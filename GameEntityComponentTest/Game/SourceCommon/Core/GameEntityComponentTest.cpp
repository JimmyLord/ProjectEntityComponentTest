//
// Copyright (c) 2012-2014 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "GameCommonHeader.h"

GameEntityComponentTest::GameEntityComponentTest()
{
    m_pComponentSystemManager = 0;

    m_EditorMode = true;

    m_TimeSinceLastPhysicsStep = 0;

    m_pShader_TintColor = 0;
    m_pShader_TestNormals = 0;
    m_pShader_Texture = 0;

    m_GameWidth = 0;
    m_GameHeight = 0;

    for( int i=0; i<4; i++ )
        m_pOBJTestFiles[i] = 0;

    for( int i=0; i<4; i++ )
        m_pTextures[i] = 0;

    m_pBulletWorld = MyNew BulletWorld();
}

GameEntityComponentTest::~GameEntityComponentTest()
{
    SAFE_DELETE( m_pShader_TintColor );
    SAFE_DELETE( m_pShader_TestNormals );
    SAFE_DELETE( m_pShader_Texture );

    SAFE_DELETE( m_pComponentSystemManager );

    for( int i=0; i<4; i++ )
    {
        if( m_pOBJTestFiles[i] )
            g_pFileManager->FreeFile( m_pOBJTestFiles[i] );
    }

    for( int i=0; i<4; i++ )
    {
        if( m_pTextures[i] )
            SAFE_RELEASE( m_pTextures[i] );
    }

    SAFE_DELETE( m_pBulletWorld );
}

void GameEntityComponentTest::OneTimeInit()
{
    GameCore::OneTimeInit();

    GameObject* pPlayer = 0;
    GameObject* pGameObject;
    ComponentCamera* pComponentCamera;
    ComponentSprite* pComponentSprite;
    ComponentMesh* pComponentMesh;
    ComponentAIChasePlayer* pComponentAIChasePlayer;
    ComponentCollisionObject* pComponentCollisionObject;

    m_pOBJTestFiles[0] = g_pFileManager->RequestFile( "Data/OBJs/cube.obj" );
    //m_pOBJTestFiles[1] = g_pFileManager->RequestFile( "Data/OBJs/Teapot.obj" );
    //m_pOBJTestFiles[2] = g_pFileManager->RequestFile( "Data/OBJs/humanoid_tri.obj" );
    //m_pOBJTestFiles[3] = g_pFileManager->RequestFile( "Data/OBJs/teapot2.obj" );

    //m_pTextures[0] = g_pTextureManager->CreateTexture( "Data/Textures/test1.png", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT );
    //m_pTextures[1] = g_pTextureManager->CreateTexture( "Data/Textures/test2.png", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT );
    //m_pTextures[2] = g_pTextureManager->CreateTexture( "Data/Textures/test3.png", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT );
    //m_pTextures[3] = g_pTextureManager->CreateTexture( "Data/Textures/test4.png", GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT );

    glEnable( GL_CULL_FACE );

    // setup our shaders
    m_pShader_TintColor = MyNew ShaderGroup( MyNew Shader_Base(ShaderPass_Main), 0, 0, "Tint Color" );
    m_pShader_TestNormals = MyNew ShaderGroup( MyNew Shader_Base(ShaderPass_Main), 0, 0, "Test-Normals" );
    m_pShader_Texture = MyNew ShaderGroup( MyNew Shader_Base(ShaderPass_Main), 0, 0, "Texture" );

    m_pShader_TintColor->SetFileForAllPasses( "Data/Shaders/Shader_TintColor" );
    m_pShader_TestNormals->SetFileForAllPasses( "Data/Shaders/Shader_TestNormals" );
    m_pShader_Texture->SetFileForAllPasses( "Data/Shaders/Shader_Texture" );

    // Initialize our component system.
    m_pComponentSystemManager = MyNew ComponentSystemManager( MyNew GameComponentTypeManager );

    // create a 3D camera, renders first.
    {
        pGameObject = m_pComponentSystemManager->CreateGameObject();
        pGameObject->SetName( "Main Camera" );
        pGameObject->m_pComponentTransform->SetPosition( Vector3( 0, 0, 10 ) );
        pComponentCamera = (ComponentCamera*)pGameObject->AddNewComponent( ComponentType_Camera );
        pComponentCamera->SetDesiredAspectRatio( 640, 960 );
        pComponentCamera->m_Orthographic = false;
        pComponentCamera->m_LayersToRender = Layer_MainScene;
    }

    // create a 2D camera, renders after 3d, for hud.
    {
        pGameObject = m_pComponentSystemManager->CreateGameObject();
        pGameObject->SetName( "Hud Camera" );
        pComponentCamera = (ComponentCamera*)pGameObject->AddNewComponent( ComponentType_Camera );
        pComponentCamera->SetDesiredAspectRatio( 640, 960 );
        pComponentCamera->m_Orthographic = true;
        pComponentCamera->m_LayersToRender = Layer_HUD;
    }

    // create a player game object and attach a mesh(sprite) component to it.
    {
        pGameObject = m_pComponentSystemManager->CreateGameObject();
        pGameObject->SetName( "Player Object" );
        pComponentSprite = (ComponentSprite*)pGameObject->AddNewComponent( ComponentType_Sprite );
        if( pComponentSprite )
        {
            pComponentSprite->SetShader( m_pShader_TintColor );
            pComponentSprite->m_Size.Set( 50.0f, 50.0f );
            pComponentSprite->m_Tint.Set( 255, 0, 0, 255 );
            pComponentSprite->m_LayersThisExistsOn = Layer_HUD;
        }
        pGameObject->AddNewComponent( ComponentType_InputTrackMousePos );
        pGameObject->m_pComponentTransform->SetPosition( Vector3( 0, 0, 0 ) );//m_GameWidth/2, m_GameHeight/2, 0 ) );

        pPlayer = pGameObject;
    }

    // create an enemy game object and attach a mesh(sprite) and AI component to it.
    {
        pGameObject = m_pComponentSystemManager->CreateGameObject();
        pGameObject->SetName( "Second Object" );
        pComponentSprite = (ComponentSprite*)pGameObject->AddNewComponent( ComponentType_Sprite );
        if( pComponentSprite )
        {
            pComponentSprite->SetShader( m_pShader_TintColor );
            pComponentSprite->m_Size.Set( 50.0f, 50.0f );
            pComponentSprite->m_Tint.Set( 0, 255, 0, 255 );
            pComponentSprite->m_LayersThisExistsOn = Layer_HUD;
        }
        pComponentAIChasePlayer = (ComponentAIChasePlayer*)pGameObject->AddNewComponent( ComponentType_AIChasePlayer );
        if( pComponentAIChasePlayer )
        {
            pComponentAIChasePlayer->m_pPlayerComponentTransform = pPlayer->m_pComponentTransform;
        }
    }

    // create a cube in the 3d scene.
    {
        pGameObject = m_pComponentSystemManager->CreateGameObject();
        pGameObject->SetName( "Cube" );
        pComponentMesh = (ComponentMesh*)pGameObject->AddNewComponent( ComponentType_MeshOBJ );
        if( pComponentMesh )
        {
            pComponentMesh->SetShader( m_pShader_TestNormals );
            pComponentMesh->m_pOBJFile = m_pOBJTestFiles[0];
            pComponentMesh->m_LayersThisExistsOn = Layer_MainScene;
        }
        pComponentCollisionObject = (ComponentCollisionObject*)pGameObject->AddNewComponent( ComponentType_CollisionObject );
    }
}

static float totaltimepassed;

void GameEntityComponentTest::Tick(double TimePassed)
{
    GameCore::Tick( TimePassed );

    totaltimepassed += (float)TimePassed;

    // tick all components.
    if( m_EditorMode )
        m_pComponentSystemManager->Tick( 0 );
    else
    {
        if( TimePassed != 0 )
            m_pComponentSystemManager->Tick( TimePassed );
    }

    if( m_EditorMode == false )
    {
        m_TimeSinceLastPhysicsStep += TimePassed;
        
        while( m_TimeSinceLastPhysicsStep > 1/60.0f )
        {
            m_TimeSinceLastPhysicsStep -= 1/60.0f;
            m_pBulletWorld->PhysicsStep();
        }
    }
}

void GameEntityComponentTest::OnDrawFrame()
{
    GameCore::OnDrawFrame();

    glClearColor( 0.0f, 0.0f, 0.2f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // draw all components.
    m_pComponentSystemManager->OnDrawFrame();
}

void GameEntityComponentTest::OnTouch(int action, int id, float x, float y, float pressure, float size)
{
    GameCore::OnTouch( action, id, x, y, pressure, size );

    // prefer 0,0 at bottom left.
    y = m_WindowHeight - y;

    if( m_EditorMode )
    {
        HandleEditorInput( -1, -1, action, x, y );
        return;
    }

    // TODO: get the camera properly.
    ComponentCamera* pCamera = m_pComponentSystemManager->GetFirstCamera();

    // convert mouse to x/y in Camera2D space. TODO: put this in camera component.
    x = (x - pCamera->m_Camera2D.m_ScreenOffsetX - m_WindowStartX) / pCamera->m_Camera2D.m_ScreenWidth * m_GameWidth;
    y = (y - pCamera->m_Camera2D.m_ScreenOffsetY + m_WindowStartY) / pCamera->m_Camera2D.m_ScreenHeight * m_GameHeight;

    m_pComponentSystemManager->OnTouch( action, id, x, y, pressure, size );
}

void GameEntityComponentTest::OnButtons(GameCoreButtonActions action, GameCoreButtonIDs id)
{
    GameCore::OnButtons( action, id );
}

void GameEntityComponentTest::OnKeyDown(int keycode, int unicodechar)
{
    if( m_EditorMode )
    {
        if( keycode == 'P' ) // if press "Play"
        {
            SaveScene();
            m_EditorMode = false;
            m_pComponentSystemManager->OnPlay();
            return;
        }

        HandleEditorInput( 1, keycode, -1, -1, -1 );
        return;
    }

    if( keycode == 'P' ) // Play/Stop
    {
        if( m_EditorMode ) // if press "Play"
        {
        }
        else // if press "Stop"
        {
            LoadScene();
            m_EditorMode = true;
            m_pComponentSystemManager->OnStop();

            m_pComponentSystemManager->SyncAllRigidBodiesToObjectTransforms();
        }
    }
}

void GameEntityComponentTest::OnKeyUp(int keycode, int unicodechar)
{
    if( m_EditorMode )
    {
        HandleEditorInput( 0, keycode, -1, -1, -1 );
        return;
    }
}

void GameEntityComponentTest::HandleEditorInput(int keydown, int keycode, int action, float x, float y)
{
#if MYFW_USING_WX
    if( keycode == MYKEYCODE_LCTRL )
    {
        if( keydown == 1 ) m_EditorState.m_ModifierKeyStates |= MODIFIERKEY_Control;
        if( keydown == 0 ) m_EditorState.m_ModifierKeyStates &= ~MODIFIERKEY_Control;
    }
    //if( keycode == MYKEYCODE_LALT )
    //{
    //    if( keydown == 1 ) m_EditorState.m_ModifierKeyStates |= MODIFIERKEY_Control;
    //    if( keydown == 0 ) m_EditorState.m_ModifierKeyStates &= ~MODIFIERKEY_Control;
    //}
    if( keycode == MYKEYCODE_LSHIFT )
    {
        if( keydown == 1 ) m_EditorState.m_ModifierKeyStates |= MODIFIERKEY_Shift;
        if( keydown == 0 ) m_EditorState.m_ModifierKeyStates &= ~MODIFIERKEY_Shift;
    }
    if( keycode == ' ' )
    {
        if( keydown == 1 ) m_EditorState.m_ModifierKeyStates |= MODIFIERKEY_Space;
        if( keydown == 0 ) m_EditorState.m_ModifierKeyStates &= ~MODIFIERKEY_Space;
    }
    if( action != -1 )
    {
        if( action == GCBA_Down )
        {
            m_EditorState.m_CurrentMousePosition.Set( x, y );
            m_EditorState.m_LastMousePosition.Set( x, y );
            m_EditorState.m_ModifierKeyStates |= MODIFIERKEY_LeftMouse;
        }
        if( action == GCBA_Up )
        {
            m_EditorState.m_CurrentMousePosition.Set( -1, -1 );
            m_EditorState.m_LastMousePosition.Set( -1, -1 );
            m_EditorState.m_ModifierKeyStates &= ~MODIFIERKEY_LeftMouse;
        }
        if( action == GCBA_Held )
        {
            m_EditorState.m_CurrentMousePosition.Set( x, y );
        }
    }
   
    if( m_EditorState.m_ModifierKeyStates == 0 ) // no modifiers held.
    {
        if( action == GCBA_Down )
        {
            // find the object we clicked on.
        }
    }

    // if control is held, pan the camera around.
    if( m_EditorState.m_ModifierKeyStates & MODIFIERKEY_LeftMouse )
    {
        // TODO: get the camera properly.
        ComponentCamera* pCamera = m_pComponentSystemManager->GetFirstCamera();
        MyMatrix* matLocalCamera = pCamera->m_pComponentTransform->GetLocalTransform();

        if( m_EditorState.m_ModifierKeyStates & MODIFIERKEY_Space )
        {
            Vector2 dir = m_EditorState.m_CurrentMousePosition - m_EditorState.m_LastMousePosition;

            if( dir.LengthSquared() > 0 )
                matLocalCamera->TranslatePreRotScale( dir * 0.05f );
        }
        else
        {
            // rotate the camera around a point 10 units in front of the camera.
            Vector2 dir = m_EditorState.m_CurrentMousePosition - m_EditorState.m_LastMousePosition;

            if( dir.LengthSquared() > 0 )
            {
                matLocalCamera->TranslatePreRotScale( 0, 0, -10 );

                MyMatrix matRotation;
                matRotation.SetIdentity();
                matRotation.Rotate( dir.y, 1, 0, 0 );
                matRotation.Rotate( dir.x, 0, 1, 0 );

                *matLocalCamera = *matLocalCamera * matRotation;

                matLocalCamera->TranslatePreRotScale( 0, 0, 10 );
            }
        }
    }

    m_EditorState.m_LastMousePosition = m_EditorState.m_CurrentMousePosition;
#endif //MYFW_USING_WX
}

void GameEntityComponentTest::SaveScene()
{
    char* savestring = g_pComponentSystemManager->SaveSceneToJSON();

    FILE* filehandle;
    errno_t error = fopen_s( &filehandle, "test.scene", "w" );
    if( filehandle )
    {
        fprintf( filehandle, "%s", savestring );
        fclose( filehandle );
    }

    free( savestring );
}

void GameEntityComponentTest::LoadScene()
{
    FILE* filehandle;
    errno_t err = fopen_s( &filehandle, "test.scene", "rb" );

    char* jsonstr;

    if( filehandle )
    {
        fseek( filehandle, 0, SEEK_END );
        long length = ftell( filehandle );
        if( length > 0 )
        {
            fseek( filehandle, 0, SEEK_SET );

            jsonstr = MyNew char[length+1];
            fread( jsonstr, length, 1, filehandle );
            jsonstr[length] = 0;
        }

        fclose( filehandle );

        g_pComponentSystemManager->LoadSceneFromJSON( jsonstr );

        delete[] jsonstr;
    }

    m_EditorMode = true;

    OnSurfaceChanged( m_WindowStartX, m_WindowStartY, m_WindowWidth, m_WindowHeight );
}

void GameEntityComponentTest::OnSurfaceChanged(unsigned int startx, unsigned int starty, unsigned int width, unsigned int height)
{
    GameCore::OnSurfaceChanged( startx, starty, width, height );

    if( height == 0 || width == 0 )
        return;

    float devicewidth = m_WindowWidth;
    float deviceheight = m_WindowHeight;
    float deviceratio = devicewidth / deviceheight;

    //if( width > height )
    //{
    //    m_GameWidth = 960.0f;
    //    m_GameHeight = 640.0f;
    //}
    //else if( height > width )
    //{
    //    m_GameWidth = 640.0f;
    //    m_GameHeight = 960.0f;
    //}
    //else
    //{
    //    m_GameWidth = 640.0f;
    //    m_GameHeight = 640.0f;
    //}

    m_GameWidth = 640.0f;
    m_GameHeight = 960.0f;

    if( m_pComponentSystemManager )
        m_pComponentSystemManager->OnSurfaceChanged( startx, starty, width, height, (unsigned int)m_GameWidth, (unsigned int)m_GameHeight );
}
