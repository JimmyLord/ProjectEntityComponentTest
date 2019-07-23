//
// Copyright (c) 2019 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "MyEnginePCH.h"

#include "EditorInterface_HeightmapEditor.h"
#include "Camera/Camera3D.h"
#include "ComponentSystem/BaseComponents/ComponentCamera.h"
#include "ComponentSystem/BaseComponents/ComponentTransform.h"
#include "ComponentSystem/Core/GameObject.h"
#include "ComponentSystem/FrameworkComponents/ComponentMesh.h"
#include "ComponentSystem/EngineComponents/ComponentHeightmap.h"
#include "Core/EngineComponentTypeManager.h"
#include "Core/EngineCore.h"
#include "GUI/EditorIcons.h"
#include "GUI/ImGuiExtensions.h"
#include "../SourceEditor/EditorState.h"
#include "../SourceEditor/Commands/EngineEditorCommands.h"
#include "../SourceEditor/Commands/HeightmapEditorCommands.h"
#include "../SourceEditor/Editor_ImGui/EditorMainFrame_ImGui.h"
#include "../SourceEditor/PlatformSpecific/FileOpenDialog.h"
#include "../SourceEditor/Prefs/EditorPrefs.h"
#include "../../../Framework/MyFramework/SourceCommon/Renderers/BaseClasses/Shader_Base.h"

class Job_CalculateNormals : public MyJob
{
protected:
    EditorInterface_HeightmapEditor* m_pHeightmapEditor;

public:
    Job_CalculateNormals()
    {
        m_pHeightmapEditor = nullptr;
    }
    virtual ~Job_CalculateNormals() {}

    void SetEditor(EditorInterface_HeightmapEditor* pHeightmapEditor)
    {
        m_pHeightmapEditor = pHeightmapEditor;
    }

    virtual void DoWork()
    {
        m_pHeightmapEditor->GetHeightmapBeingEdited()->RecalculateNormals();
    }
};

EditorInterface_HeightmapEditor::EditorInterface_HeightmapEditor(EngineCore* pEngineCore)
//: EditorInterface( pEngineCore )
: EditorDocument( pEngineCore )
{
    m_pCamera = MyNew ComponentCamera( nullptr );
    m_pCamera->Reset();
    m_pCameraTransform = MyNew ComponentTransform( nullptr );
    m_pCameraTransform->Reset();
    m_pCamera->m_pComponentTransform = m_pCameraTransform;

    m_pCameraTransform->SetWorldPosition( Vector3( 5, 5, -5 ) );
    m_pCamera->m_Camera3D.LookAt( m_pCameraTransform->GetWorldPosition(), Vector3(0,1,0), Vector3( 5, 0, 5 ) );

    m_pFBO = nullptr;
    m_WindowPos.Set( -1, -1 );
    m_WindowSize.Set( 0, 0 );
    m_WindowHovered = false;
    m_WindowFocused = false;
    m_WindowVisible = false;

    m_pHeightmap = nullptr;

    m_CurrentTool = Tool::Raise;
    m_CurrentToolState = ToolState::Idle;

    m_pPoint = nullptr;

    m_PositionMouseWentDown.Set( 0, 0 );

    m_IndexOfPointBeingDragged = -1;
    m_NewMousePress = false;

    m_WorldSpaceMousePosition.Set( 0, 0, 0 );
    m_WorldSpaceMousePositionWhenToolStarted.Set( 0, 0, 0 );

    for( int i=0; i<Mat_NumMaterials; i++ )
    {
        m_pMaterials[i] = nullptr;
    }

    m_HeightmapNormalsNeedRebuilding = false;
    m_pJob_CalculateNormals = nullptr;

    // Warnings.
    m_ShowWarning_CloseEditor = false;

    // Editor settings.
    m_BrushSoftness = 0.1f;
    m_BrushRadius = 1.0f;
    m_RaiseAmount = 0.1f;
    m_LevelUseBrushHeight = true;
    m_LevelHeight = 0.0f;

    m_AlwaysRecalculateNormals = false;
}

EditorInterface_HeightmapEditor::~EditorInterface_HeightmapEditor()
{
    SAFE_DELETE( m_pJob_CalculateNormals );

    SAFE_DELETE( m_pPoint );

    for( int i=0; i<Mat_NumMaterials; i++ )
    {
        SAFE_RELEASE( m_pMaterials[i] );
    }

    SAFE_RELEASE( m_pFBO );

    m_pCameraTransform->SetEnabled( false );
    m_pCamera->SetEnabled( false );
    SAFE_DELETE( m_pCameraTransform );
    SAFE_DELETE( m_pCamera );
}

void EditorInterface_HeightmapEditor::Initialize()
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;
    MaterialManager* pMaterialManager = pEngineCore->GetManagers()->GetMaterialManager();

    if( m_pMaterials[Mat_Point] == nullptr )
        m_pMaterials[Mat_Point] = MyNew MaterialDefinition( pMaterialManager, pEngineCore->GetShader_TintColor(), ColorByte(255,255,0,255) );
    if( m_pMaterials[Mat_BrushOverlay] == nullptr )
        m_pMaterials[Mat_BrushOverlay] = pMaterialManager->LoadMaterial( "Data/DataEngine/Materials/HeightmapBrush.mymaterial" );
}

bool EditorInterface_HeightmapEditor::IsBusy()
{
    if( m_pJob_CalculateNormals )
    {
        if( m_pJob_CalculateNormals->IsQueued() )
            return true;
    }

    return false;
}

void EditorInterface_HeightmapEditor::OnActivated()
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    // Prevent any overlays on selected items.
    pEngineCore->GetEditorPrefs()->Set_Internal_ShowSpecialEffectsForSelectedItems( false );

    // Create a gameobject for the point that we'll draw.
    if( m_pPoint == nullptr )
    {
        GameObject* pGameObject;
        ComponentMesh* pComponentMesh;

        pGameObject = g_pComponentSystemManager->CreateGameObject( false, SCENEID_EngineObjects ); // Not managed.
        pGameObject->SetName( "Heightmap editor - point" );
        pGameObject->GetTransform()->SetLocalRotation( Vector3( -90, 0, 0 ) );

        pComponentMesh = (ComponentMesh*)pGameObject->AddNewComponent( ComponentType_Mesh, SCENEID_EngineObjects, g_pComponentSystemManager );
        if( pComponentMesh )
        {
            pComponentMesh->SetVisible( true );
            pComponentMesh->SetMaterial( m_pMaterials[Mat_Point], 0 );
            pComponentMesh->SetLayersThisExistsOn( Layer_EditorFG );
            pComponentMesh->m_pMesh = MyNew MyMesh( g_pComponentSystemManager->GetEngineCore() );
            pComponentMesh->m_pMesh->Create2DCircle( 0.25f, 20 );
            pComponentMesh->m_GLPrimitiveType = pComponentMesh->m_pMesh->GetSubmesh( 0 )->m_PrimitiveType;

            pComponentMesh->OnLoad();
        }

        m_pPoint = pGameObject;
    }
}

void EditorInterface_HeightmapEditor::OnDeactivated()
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    // Allow overlays on selected items.
    pEngineCore->GetEditorPrefs()->Set_Internal_ShowSpecialEffectsForSelectedItems( true );

    ComponentRenderable* pRenderable;

    pRenderable = (ComponentRenderable*)m_pPoint->GetFirstComponentOfBaseType( BaseComponentType_Renderable );
    pRenderable->SetVisible( false );
}

void EditorInterface_HeightmapEditor::OnDrawFrame() //unsigned int canvasID)
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    // EditorInterface class will draw the main editor view.
    //EditorInterface::OnDrawFrame( canvasID );

    MyAssert( m_pHeightmap != nullptr );
    if( m_pHeightmap == nullptr )
        return;

    MyAssert( m_pPoint != nullptr );
    if( m_pPoint == nullptr )
        return;

    ComponentRenderable* pRenderable;

    if( m_WindowVisible && m_WindowSize.LengthSquared() != 0 )
    {
        if( m_pFBO == nullptr )
        {
            m_pFBO = pEngineCore->GetManagers()->GetTextureManager()->CreateFBO( 1024, 1024, MyRE::MinFilter_Nearest, MyRE::MagFilter_Nearest, FBODefinition::FBOColorFormat_RGBA_UByte, 32, true );
            m_pFBO->MemoryPanel_Hide();
        }

        m_pCamera->Tick( 0.0f );

        if( m_pFBO->GetColorTexture( 0 ) )
        {
            uint32 previousFBO = g_GLStats.m_CurrentFramebuffer;

            m_pFBO->Bind( false );
            MyViewport viewport( 0, 0, (uint32)m_WindowSize.x, (uint32)m_WindowSize.y );
            pEngineCore->GetRenderer()->EnableViewport( &viewport, true );
        
            pEngineCore->GetRenderer()->SetClearColor( ColorFloat( 0.0f,0.1f,0.1f,1.0f ) );
            pEngineCore->GetRenderer()->ClearBuffers( true, true, true );

            // Draw the heightmap.
            {
                MyMatrix* pWorldMat = m_pHeightmap->GetGameObject()->GetTransform()->GetWorldTransform();
                Vector3 localSpacePoint = pWorldMat->GetInverse() * m_WorldSpaceMousePosition;

                bool wasVisible = m_pHeightmap->IsVisible();
                m_pHeightmap->SetVisible( true );

                MyMatrix* pEditorMatProj = &m_pCamera->m_Camera3D.m_matProj;
                MyMatrix* pEditorMatView = &m_pCamera->m_Camera3D.m_matView;

                g_pComponentSystemManager->DrawSingleComponent( pEditorMatProj, pEditorMatView, m_pHeightmap, nullptr, 0 );

                m_pHeightmap->SetVisible( wasVisible );
            }

            // Draw the brush circle projected on the heightmap.
            {
                MyMatrix* pWorldMat = m_pHeightmap->GetGameObject()->GetTransform()->GetWorldTransform();
                Vector3 localSpacePoint = pWorldMat->GetInverse() * m_WorldSpaceMousePosition;

                bool wasVisible = m_pHeightmap->IsVisible();
                m_pHeightmap->SetVisible( true );

                MyMatrix* pEditorMatProj = &m_pCamera->m_Camera3D.m_matProj;
                MyMatrix* pEditorMatView = &m_pCamera->m_Camera3D.m_matView;

                MaterialDefinition* pMaterial = m_pMaterials[Mat_BrushOverlay];
                Vector2 size = m_pHeightmap->m_Size;
                Vector2 scale = Vector2( m_BrushRadius, m_BrushRadius ) * 2;
                pMaterial->SetUVScale( 1.0f/scale );
                pMaterial->SetUVOffset( Vector2( -localSpacePoint.x + scale.x/2.0f, -localSpacePoint.z + scale.y/2.0f ) );
                pEngineCore->GetRenderer()->SetTextureWrapModes( pMaterial->GetTextureColor(), MyRE::WrapMode_Clamp, MyRE::WrapMode_Clamp );
                g_pComponentSystemManager->DrawSingleComponent( pEditorMatProj, pEditorMatView, m_pHeightmap, &pMaterial, 1 );

                m_pHeightmap->SetVisible( wasVisible );
            }

            // Draw a circle at the mouse position for the height desired by the level tool.
            pRenderable = (ComponentRenderable*)m_pPoint->GetFirstComponentOfBaseType( BaseComponentType_Renderable );
            if( m_CurrentTool == Tool::Level && m_LevelUseBrushHeight == false )
            {
                pRenderable->SetVisible( true );

                Vector3 worldPos = m_WorldSpaceMousePositionAtDesiredHeight;

                m_pPoint->GetTransform()->SetLocalPosition( worldPos );

                MyMatrix* pEditorMatProj = &m_pCamera->m_Camera3D.m_matProj;
                MyMatrix* pEditorMatView = &m_pCamera->m_Camera3D.m_matView;

                pEngineCore->GetRenderer()->SetDepthFunction( MyRE::DepthFunc_Always );
                g_pComponentSystemManager->DrawSingleObject( pEditorMatProj, pEditorMatView, m_pPoint, nullptr );
                pEngineCore->GetRenderer()->SetDepthFunction( MyRE::DepthFunc_LEqual );
            }
            else
            {
                pRenderable->SetVisible( false );
            }

            g_pRenderer->BindFramebuffer( previousFBO );
        }
    }

    // Show some heightmap editor controls.
    ImGui::SetNextWindowSize( ImVec2(150,200), ImGuiCond_FirstUseEver );
    ImGui::SetNextWindowBgAlpha( 1.0f );
    ImGui::Begin( "Heightmap Editor", nullptr, ImGuiWindowFlags_NoFocusOnAppearing );

    // Icon bar to select tools.
    {
        // TODO: Make new icons.
        ImVec2 normalSize = ImVec2( 20, 20 );
        ImVec2 selectedSize = ImVec2( 30, 30 );
        if( ImGuiExt::ButtonWithTooltip( EditorIcon_Prefab, m_CurrentTool == Tool::Raise ? selectedSize : normalSize, "Raise" ) )
        {
            m_CurrentTool = Tool::Raise;
        }
        ImGui::SameLine();
        if( ImGuiExt::ButtonWithTooltip( EditorIcon_Folder, m_CurrentTool == Tool::Lower ? selectedSize : normalSize, "Lower" ) )
        {
            m_CurrentTool = Tool::Lower;
        }
        ImGui::SameLine();
        if( ImGuiExt::ButtonWithTooltip( EditorIcon_GameObject, m_CurrentTool == Tool::Level ? selectedSize : normalSize, "Level" ) )
        {
            m_CurrentTool = Tool::Level;
        }
    }

    // General options.
    if( ImGui::CollapsingHeader( "General", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
        ImGui::Checkbox( "Always recalculate normals", &m_AlwaysRecalculateNormals );
    }

    // Tool specific values.
    if( ImGui::CollapsingHeader( "Brush", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
        ImGui::DragFloat( "Softness", &m_BrushSoftness, 0.01f, 0.0f, 1.0f );
        ImGui::DragFloat( "Radius", &m_BrushRadius, 0.01f, 0.0f, FLT_MAX );
    }

    if( ImGui::CollapsingHeader( "Raise/Lower", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
        ImGui::DragFloat( "Amount", &m_RaiseAmount, 0.01f, 0.0f, FLT_MAX );
    }

    if( ImGui::CollapsingHeader( "Level", ImGuiTreeNodeFlags_DefaultOpen ) )
    {
        ImGui::Checkbox( "Use brush height", &m_LevelUseBrushHeight );
        ImGui::DragFloat( "Height", &m_LevelHeight, 0.01f, 0.0f, FLT_MAX );
    }

    // Other.
    ImGui::Separator();

    if( ImGui::Button( "Export as MyMesh" ) )
    {
        m_pHeightmap->SaveAsMyMesh( "Data/Meshes/TestHeightmap.mymesh" );
    }

    if( ImGui::Button( "Save" ) )
    {
        Save();
    }

    ImGui::End();

    if( m_ShowWarning_CloseEditor )
    {
        m_ShowWarning_CloseEditor = false;
        ImGui::OpenPopup( "Close Heightmap Editor Warning" );
    }
    if( ImGui::BeginPopupModal( "Close Heightmap Editor Warning" ) )
    {
        ImGui::Text( "Some changes aren't saved." );
        ImGui::Dummy( ImVec2( 0, 10 ) );

        if( ImGui::Button( "Revert changes" ) )
        {
            while( pEngineCore->GetCommandStack()->GetUndoStackSize() > 0 )
                pEngineCore->GetCommandStack()->Undo( 1 );
            ImGui::CloseCurrentPopup();
            //pEngineCore->SetEditorInterface( EditorInterfaceType::SceneManagement );
        }

        if( ImGui::Button( "Save" ) )
        {
            ImGui::CloseCurrentPopup();
            Save();
            //pEngineCore->SetEditorInterface( EditorInterfaceType::SceneManagement );
        }

        if( ImGui::Button( "Keep changes without saving" ) )
        {
            ImGui::CloseCurrentPopup();
            //pEngineCore->SetEditorInterface( EditorInterfaceType::SceneManagement );
        }

        ImGui::EndPopup();
    }
}

void EditorInterface_HeightmapEditor::AddImGuiOverlayItems()
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    ImGui::Text( "Editing Heightmap: %s", m_pHeightmap->GetGameObject()->GetName() );

    if( pEngineCore->GetCommandStack()->GetUndoStackSize() > 0 )
    {
        ImGui::Text( "Unsaved changes" );
    }

    if( m_pJob_CalculateNormals && m_pJob_CalculateNormals->IsQueued() )
    {
        ImGui::Text( "Recalculating normals" );
    }
}

void EditorInterface_HeightmapEditor::CancelCurrentOperation()
{
    m_CurrentToolState = ToolState::Idle;
    g_pGameCore->GetCommandStack()->Undo( 1 );
}

void EditorInterface_HeightmapEditor::GetMouseRay(Vector2 mousepos, Vector3* start, Vector3* end)
{
    // Convert mouse coord into clip space.
    Vector2 mouseClip;
    mouseClip.x = (mousepos.x / m_WindowSize.x) * 2.0f - 1.0f;
    mouseClip.y = (mousepos.y / m_WindowSize.y) * 2.0f - 1.0f;

    // Compute the inverse view projection matrix.
    MyMatrix invVP = ( m_pCamera->m_Camera3D.m_matProj * m_pCamera->m_Camera3D.m_matView ).GetInverse();

    // Store the camera position as the near world point.
    Vector3 nearWorldPoint = m_pCameraTransform->GetWorldPosition();

    // Calculate the world position of the far clip plane where the mouse is pointing.
    Vector4 farClipPoint4 = Vector4( mouseClip, 1, 1 );
    Vector4 farWorldPoint4 = invVP * farClipPoint4;
    Vector3 farWorldPoint = farWorldPoint4.XYZ() / farWorldPoint4.w;

    *start = nearWorldPoint;
    *end = farWorldPoint;
}

bool EditorInterface_HeightmapEditor::HandleInput(int keyAction, int keyCode, int mouseAction, int id, float x, float y, float pressure)
{
    EditorDocument::HandleInput( keyAction, keyCode, mouseAction, id, x, y, pressure );

    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;
    EditorState* pEditorState = pEngineCore->GetEditorState();

    // If nothing in this method directly uses the input, then we'll pass it to the camera.
    bool inputHandled = false;

    // Deal with keys.
    if( keyAction != -1 )
    {
        // Escape to cancel current tool or exit heightmap editor.
        if( keyAction == GCBA_Up && keyCode == MYKEYCODE_ESC )
        {
            if( m_CurrentToolState == ToolState::Active )
            {
                CancelCurrentOperation();
            }
            else
            {
                // Don't allow users leave this editor mode if there are jobs pending.
                if( IsBusy() == false )
                {
                    if( pEngineCore->GetCommandStack()->GetUndoStackSize() > 0 )
                    {
                        m_ShowWarning_CloseEditor = true;
                    }
                    else
                    {
                        //pEngineCore->SetEditorInterface( EditorInterfaceType::SceneManagement );
                    }
                }
            }
        }
    }

    // Deal with mouse.
    if( mouseAction != -1 )
    {
        // Find the mouse intersection point on the heightmap.
        Vector3 start, end;
        GetMouseRay( Vector2( x, y ), &start, &end );

        Vector3 mouseIntersectionPoint;
        bool mouseRayIntersected = m_pHeightmap->RayCast( start, end, &mouseIntersectionPoint );

        // Show a 2d circle at that point.
        MyMatrix* pWorldMat = m_pHeightmap->GetGameObject()->GetTransform()->GetWorldTransform();
        m_WorldSpaceMousePosition = *pWorldMat * mouseIntersectionPoint;
        //LOGInfo( LOGTag, "RayCast result is (%0.2f, %0.2f, %0.2f)\n", mouseIntersectionPoint.x, mouseIntersectionPoint.y, mouseIntersectionPoint.z );

        if( m_CurrentTool == Tool::Level && m_LevelUseBrushHeight == false )
        {
            Vector3 mouseIntersectPointAtDesiredHeight;

            m_pHeightmap->RayCastAtLocalHeight( start, end, m_LevelHeight, &mouseIntersectPointAtDesiredHeight, &mouseIntersectionPoint );
            m_WorldSpaceMousePositionAtDesiredHeight = *pWorldMat * mouseIntersectPointAtDesiredHeight;

            m_WorldSpaceMousePosition = *pWorldMat * mouseIntersectionPoint;
        }

        if( pEditorState->m_ModifierKeyStates & MODIFIERKEY_LeftMouse )
        {
            // Right mouse button to cancel current operation.
            if( mouseAction == GCBA_Down && id == 1 && m_CurrentTool != Tool::None )
            {
                CancelCurrentOperation();
                inputHandled = true;
            }

            // Left mouse button down.
            if( mouseAction == GCBA_Down && id == 0 )
            {
                m_PositionMouseWentDown.Set( x, y );

                if( mouseRayIntersected )
                {
                    m_CurrentToolState = ToolState::Active;
                    m_WorldSpaceMousePositionWhenToolStarted = m_WorldSpaceMousePosition;
                    ApplyCurrentTool( mouseIntersectionPoint, mouseAction );
                }
            }

            if( mouseAction == GCBA_Up && id == 0 ) // Left mouse button up.
            {
                MyMatrix* pWorldMat = m_pHeightmap->GetGameObject()->GetTransform()->GetWorldTransform();
                Vector3 localSpacePoint = pWorldMat->GetInverse() * m_WorldSpaceMousePosition;
                ApplyCurrentTool( localSpacePoint, mouseAction );

                // Wait for any outstanding jobs to complete.
                pEngineCore->GetManagers()->GetJobManager()->WaitForJobToComplete( m_pJob_CalculateNormals );

                m_CurrentToolState = ToolState::Idle;

                // Rebuild heightmap normals when mouse is lifted after 'raise' command.
                if( m_HeightmapNormalsNeedRebuilding )
                {
                    m_HeightmapNormalsNeedRebuilding = false;
                    m_pHeightmap->RecalculateNormals();
                }
            }

            if( mouseAction == GCBA_Held && id == 0 ) // Left mouse button moved.
            {
                if( m_CurrentToolState == ToolState::Active )
                {
                    // Once the tool is active, intersect with the y=height plane of the initial ray.
                    // This will prevent the raise tool from moving around as the height changes.
                    Plane plane;
                    float heightWanted = m_WorldSpaceMousePositionWhenToolStarted.y;
                    plane.Set( Vector3( 0, 1, 0 ), Vector3( 0, heightWanted, 0 ) );

                    Vector3 flatIntersectPoint;
                    if( plane.IntersectRay( start, end, &flatIntersectPoint ) )
                    {
                        Vector3 localSpacePoint = pWorldMat->GetInverse() * flatIntersectPoint;
                        ApplyCurrentTool( localSpacePoint, mouseAction );

                        m_WorldSpaceMousePosition = flatIntersectPoint;
                    }
                }
            }
        }
    }

    // Handle camera movement, with both mouse and keyboard.
    if( inputHandled == false )
    {
        if( m_pCamera->HandleInputForEditorCamera( keyAction, keyCode, mouseAction, id, x, y, pressure ) )
        {
            return true;
        }
    }

    return false;
}

bool EditorInterface_HeightmapEditor::ExecuteHotkeyAction(HotKeyAction action)
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;
    EditorPrefs* pEditorPrefs = pEngineCore->GetEditorPrefs();

#pragma warning( push )
#pragma warning( disable : 4062 )
    switch( action )
    {
    case HotKeyAction::HeightmapEditor_Tool_Raise:    { m_CurrentTool = Tool::Raise; return true; }
    case HotKeyAction::HeightmapEditor_Tool_Lower:    { m_CurrentTool = Tool::Lower; return true; }
    case HotKeyAction::HeightmapEditor_Tool_Level:    { m_CurrentTool = Tool::Level; return true; }
    }
#pragma warning( pop )

    return false;
}

void EditorInterface_HeightmapEditor::Update()
{
    EditorDocument::Update();

    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    m_WindowVisible = true;

    ImVec2 min = ImGui::GetWindowContentRegionMin();
    ImVec2 max = ImGui::GetWindowContentRegionMax();
    float w = max.x - min.x;
    float h = max.y - min.y;

    ImVec2 pos = ImGui::GetWindowPos();
    m_WindowPos.Set( pos.x + min.x, pos.y + min.y );
    m_WindowSize.Set( w, h );

    m_pCamera->m_Camera3D.SetupProjection( w/h, w/h, 45, 0.01f, 100.0f );
    m_pCamera->m_Camera3D.UpdateMatrices();

    //ImGui::Text( "Testing Heightmap EditorDocument." );

    if( m_pFBO )
    {
        // This will resize our FBO if the window is larger than it ever was.
        pEngineCore->GetManagers()->GetTextureManager()->ReSetupFBO( m_pFBO, (unsigned int)w, (unsigned int)h, MyRE::MinFilter_Nearest, MyRE::MagFilter_Nearest, FBODefinition::FBOColorFormat_RGBA_UByte, 32, true );

        if( m_pFBO->GetColorTexture( 0 ) )
        {
            TextureDefinition* tex = m_pFBO->GetColorTexture( 0 );
            ImGui::ImageButton( (void*)tex, ImVec2( w, h ), ImVec2(0,h/m_pFBO->GetTextureHeight()), ImVec2(w/m_pFBO->GetTextureWidth(),0), 0 );
        }
    }
}

void EditorInterface_HeightmapEditor::SetHeightmap(ComponentHeightmap* pHeightmap)
{
    m_pHeightmap = pHeightmap;

    if( m_pJob_CalculateNormals == nullptr )
    {
        m_pJob_CalculateNormals = MyNew Job_CalculateNormals();
        m_pJob_CalculateNormals->SetEditor( this );
    }

    if( m_pHeightmap )
    {
        // TODO: If no heightmap is created, then make a default flat one.
    }
}

MaterialDefinition* EditorInterface_HeightmapEditor::GetMaterial(MaterialTypes type)
{
    MyAssert( type >= 0 && type < Mat_NumMaterials );

    return m_pMaterials[type];
}

void EditorInterface_HeightmapEditor::ApplyCurrentTool(Vector3 mouseIntersectionPoint, int mouseAction)
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    if( m_CurrentToolState != ToolState::Active )
        return;

    bool valuesChanged = false;

    switch( m_CurrentTool )
    {
    case Tool::Raise:
    case Tool::Lower:
        {
            // Ignore mouse up's.
            if( mouseAction == GCBA_Up )
                break;

            float amount = m_RaiseAmount;
            if( m_CurrentTool == Tool::Lower )
                amount *= -1;

            // Raise the terrain and add to command stack.
            if( m_pHeightmap->Tool_Raise( mouseIntersectionPoint, amount, m_BrushRadius, m_BrushSoftness, true ) )
            {
                EditorCommand_Heightmap_Raise* pCommand = MyNew EditorCommand_Heightmap_Raise( m_pHeightmap, mouseIntersectionPoint, amount, m_BrushRadius, m_BrushSoftness );

                // Attach to previous 'raise' command if this is a held message,
                //   otherwise it's a mouse down so create a new message.
                bool attachToPrevious = mouseAction == GCBA_Held ? true : false;
                pEngineCore->GetCommandStack()->Add( pCommand, attachToPrevious );

                valuesChanged = true;
            }
        }
        break;

    case Tool::Level:
        {
            if( mouseAction == GCBA_Down )
            {
                EditorCommand_Heightmap_FullBackup* pCommand = MyNew EditorCommand_Heightmap_FullBackup( m_pHeightmap );
                pEngineCore->GetCommandStack()->Add( pCommand, false );
            }
            if( mouseAction == GCBA_Up )
            {
                EditorCommand_Heightmap_FullBackup* pCommand = (EditorCommand_Heightmap_FullBackup*)pEngineCore->GetCommandStack()->GetUndoCommandAtIndex( pEngineCore->GetCommandStack()->GetUndoStackSize() - 1 );
                pCommand->CopyInFinalHeights();
                break;
            }

            float height = m_LevelUseBrushHeight ? mouseIntersectionPoint.y : m_LevelHeight;
            Vector3 point = m_LevelUseBrushHeight ? mouseIntersectionPoint : m_WorldSpaceMousePositionAtDesiredHeight;

            if( m_pHeightmap->Tool_Level( point, height, m_BrushRadius, m_BrushSoftness, true ) )
            {
                // TODO: Undo/Redo.

                valuesChanged = true;
            }
        }
        break;

    case Tool::None:
        break;
    }

    // Deal with normals.
    if( valuesChanged )
    {
        if( m_AlwaysRecalculateNormals )
        {
            m_pHeightmap->RecalculateNormals();
        }
        else
        {
            // Add a job to regenerate the normals on another thread.
            if( m_pJob_CalculateNormals->IsQueued() == false )
            {
                pEngineCore->GetManagers()->GetJobManager()->AddJob( m_pJob_CalculateNormals );
            }
            else
            {
                m_HeightmapNormalsNeedRebuilding = true;
            }
        }
    }
}

//====================================================================================================
// Protected Methods.
//====================================================================================================
void EditorInterface_HeightmapEditor::Save()
{
    EngineCore* pEngineCore = EditorDocument::m_pEngineCore;

    if( m_pHeightmap->m_pHeightmapFile )
    {
        m_pHeightmap->SaveAsHeightmap( m_pHeightmap->m_pHeightmapFile->GetFullPath() );
    }
    else
    {
        const char* path = FileSaveDialog( "Data\\Meshes\\", "Heightmap Files\0*.myheightmap\0All\0*.*\0" );
        if( path[0] != 0 )
        {
            int len = (int)strlen( path );

            // Append '.myheightmap' to end of filename if it wasn't already there.
            char fullPath[MAX_PATH];
            if( strcmp( &path[len-12], ".myheightmap" ) == 0 )
            {
                strcpy_s( fullPath, MAX_PATH, path );
            }
            else
            {
                sprintf_s( fullPath, MAX_PATH, "%s.myheightmap", path );
            }

            // Only set the filename and save if the path is relative.
            const char* relativePath = ::GetRelativePath( path );
            if( relativePath )
            {
                m_pHeightmap->SaveAsHeightmap( relativePath );
                MyFileInfo* pFileInfo = pEngineCore->GetComponentSystemManager()->LoadDataFile( relativePath, m_pHeightmap->GetSceneID(), nullptr, false );
                m_pHeightmap->SetHeightmapFile( pFileInfo->GetFile() );
            }
            else
            {
                LOGError( LOGTag, "Document not saved, path must be relative to the editor." );
            }
        }
    }
}
