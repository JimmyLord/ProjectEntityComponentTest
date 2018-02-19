//
// Copyright (c) 2018 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "EngineCommonHeader.h"

EditorPrefs* g_pEditorPrefs = 0;

EditorPrefs::EditorPrefs()
{
    g_pEditorPrefs = this;

    m_pSaveFile = 0;

    m_jEditorPrefs = 0;

    // Set default values for prefs.
    m_WindowX = 0;
    m_WindowY = 0;
    m_WindowWidth = 1280;
    m_WindowHeight = 600;
    m_IsWindowMaximized = false;

    m_View_ShowEditorIcons = true;
    m_Debug_DrawPhysicsDebugShapes = true;
    m_SelectedObjects_ShowWireframe = true;
    m_SelectedObjects_ShowEffect = true;
    m_Mode_SwitchFocusOnPlayStop = true;

    m_GridSettings.visible = true;
    m_GridSettings.snapenabled = false;
    m_GridSettings.stepsize.Set( 1, 1, 1 );
}

EditorPrefs::~EditorPrefs()
{
    MyAssert( m_pSaveFile == 0 );

    cJSON_Delete( m_jEditorPrefs );
}

void EditorPrefs::Init()
{
    FILE* file = 0;
#if MYFW_USING_WX
    const char* iniFilename = "wxEditorPrefs.ini";
#else
    const char* iniFilename = "EditorPrefs.ini";
#endif

#if MYFW_WINDOWS
    fopen_s( &file, iniFilename, "rb" );
#else
    file = fopen( iniFilename, "rb" );
#endif

    if( file )
    {
        char* string = MyNew char[10000];
        size_t len = fread( string, 1, 10000, file );
        string[len] = 0;
        fclose( file );

        m_jEditorPrefs = cJSON_Parse( string );
        delete[] string;
    }
}

void EditorPrefs::LoadWindowSizePrefs()
{
    if( m_jEditorPrefs == 0 )
        return;

#if MYFW_USING_IMGUI
    // Load in some prefs.
    cJSONExt_GetInt( m_jEditorPrefs, "WindowX", &m_WindowX );
    cJSONExt_GetInt( m_jEditorPrefs, "WindowY", &m_WindowY );
    cJSONExt_GetInt( m_jEditorPrefs, "WindowWidth", &m_WindowWidth );
    cJSONExt_GetInt( m_jEditorPrefs, "WindowHeight", &m_WindowHeight );
    cJSONExt_GetBool( m_jEditorPrefs, "IsMaximized", &m_IsWindowMaximized );

    // Resize window.
    SetWindowPos( g_hWnd, 0, m_WindowX, m_WindowY, m_WindowWidth, m_WindowHeight, 0 );
    if( m_IsWindowMaximized )
    {
        ShowWindow( g_hWnd, SW_MAXIMIZE );
    }
#endif //MYFW_USING_IMGUI
}

void EditorPrefs::LoadPrefs()
{
    if( m_jEditorPrefs == 0 )
        return;

    cJSON* jObject;

    if( g_pEngineCore )
    {
        jObject = cJSON_GetObjectItem( m_jEditorPrefs, "EditorCam" );
        if( jObject )
            g_pEngineCore->GetEditorState()->GetEditorCamera()->m_pComponentTransform->ImportFromJSONObject( jObject, SCENEID_EngineObjects );
    }

    //extern GLViewTypes g_CurrentGLViewType;
    //cJSONExt_GetInt( jEditorPrefs, "GameAspectRatio", (int*)&g_CurrentGLViewType );

    cJSONExt_GetBool( m_jEditorPrefs, "View_ShowEditorIcons", &m_View_ShowEditorIcons );
    cJSONExt_GetBool( m_jEditorPrefs, "Debug_DrawPhysicsDebugShapes", &m_Debug_DrawPhysicsDebugShapes );
    cJSONExt_GetBool( m_jEditorPrefs, "SelectedObjects_ShowWireframe", &m_SelectedObjects_ShowWireframe );
    cJSONExt_GetBool( m_jEditorPrefs, "SelectedObjects_ShowEffect", &m_SelectedObjects_ShowEffect );
    cJSONExt_GetBool( m_jEditorPrefs, "Mode_SwitchFocusOnPlayStop", &m_Mode_SwitchFocusOnPlayStop );

    cJSONExt_GetBool( m_jEditorPrefs, "Grid_Visible", &m_GridSettings.visible );
    if( g_pEngineCore )
    {
        g_pEngineCore->SetGridVisible( m_GridSettings.visible );
    }
    cJSONExt_GetBool( m_jEditorPrefs, "Grid_SnapEnabled", &m_GridSettings.snapenabled );
    cJSONExt_GetFloatArray( m_jEditorPrefs, "Grid_StepSize", &m_GridSettings.stepsize.x, 3 );
}

void EditorPrefs::LoadLastSceneLoaded()
{
    bool scenewasloaded = false;

    // Load the scene at the end.
    if( m_jEditorPrefs != 0 )
    {
        cJSON* jObject = cJSON_GetObjectItem( m_jEditorPrefs, "LastSceneLoaded" );
        if( jObject && jObject->valuestring[0] != 0 )
        {
            char* scenename = jObject->valuestring;

            if( g_pEngineCore == 0 )
                return;

            // Load the scene from file.
            // This might cause some "undo" actions, so wipe them out once the load is complete.
            unsigned int numItemsInUndoStack = g_pEngineCore->GetCommandStack()->GetUndoStackSize();

            char fullpath[MAX_PATH];
            GetFullPath( scenename, fullpath, MAX_PATH );
            SceneID sceneid = g_pEngineCore->LoadSceneFromFile( fullpath );

            scenewasloaded = true;

            g_pEngineCore->GetCommandStack()->ClearUndoStack( numItemsInUndoStack );

            //this->SetTitle( scenename ); //g_pComponentSystemManager->GetSceneInfo( sceneid )->m_FullPath );
        }
    }

    if( scenewasloaded == false )
    {
        g_pEngineCore->GetEditorState()->ClearKeyAndActionStates();
        g_pEngineCore->GetEditorState()->ClearSelectedObjectsAndComponents();
        g_pComponentSystemManager->CreateNewScene( "Unsaved.scene", SCENEID_MainScene );
        g_pEngineCore->CreateDefaultSceneObjects();
    }
}

cJSON* EditorPrefs::SaveStart()
{
    MyAssert( m_pSaveFile == 0 );

#if MYFW_WINDOWS
#if MYFW_USING_WX
    fopen_s( &m_pSaveFile, "wxEditorPrefs.ini", "wb" );
#else
    fopen_s( &m_pSaveFile, "EditorPrefs.ini", "wb" );
#endif
#else
    m_pSaveFile = fopen( "EditorPrefs.ini", "wb" );
#endif
    if( m_pSaveFile )
    {
        cJSON* jPrefs = cJSON_CreateObject();

#if MYFW_USING_IMGUI
        WINDOWINFO info;
        if( GetWindowInfo( g_hWnd, &info ) )
        {
            m_WindowX = info.rcWindow.left;
            m_WindowY = info.rcWindow.top;

            m_WindowWidth = info.rcWindow.right - info.rcWindow.left;
            m_WindowHeight = info.rcWindow.bottom - info.rcWindow.top;

            if( info.dwStyle & WS_MAXIMIZE )
                m_IsWindowMaximized = true;
            else
                m_IsWindowMaximized = false;
        }
#endif //MYFW_USING_IMGUI

        cJSON_AddNumberToObject( jPrefs, "WindowX", m_WindowX );
        cJSON_AddNumberToObject( jPrefs, "WindowY", m_WindowY );
        cJSON_AddNumberToObject( jPrefs, "WindowWidth", m_WindowWidth );
        cJSON_AddNumberToObject( jPrefs, "WindowHeight", m_WindowHeight );
        cJSON_AddNumberToObject( jPrefs, "IsMaximized", m_IsWindowMaximized );

        const char* relativepath = GetRelativePath( g_pComponentSystemManager->GetSceneInfo( SCENEID_MainScene )->m_FullPath );
        if( relativepath )
            cJSON_AddStringToObject( jPrefs, "LastSceneLoaded", relativepath );
        else
            cJSON_AddStringToObject( jPrefs, "LastSceneLoaded", g_pComponentSystemManager->GetSceneInfo( SCENEID_MainScene )->m_FullPath );

        cJSON_AddItemToObject( jPrefs, "EditorCam", g_pEngineCore->GetEditorState()->GetEditorCamera()->m_pComponentTransform->ExportAsJSONObject( false, true ) );

        //cJSON* jGameObjectFlagsArray = cJSON_CreateStringArray( g_pEngineCore->GetGameObjectFlagStringArray(), 32 );
        //cJSON_AddItemToObject( pPrefs, "GameObjectFlags", jGameObjectFlagsArray );

        //// View menu options
        //cJSON_AddNumberToObject( pPrefs, "EditorLayout", GetDefaultEditorPerspectiveIndex() );
        //cJSON_AddNumberToObject( pPrefs, "GameplayLayout", GetDefaultGameplayPerspectiveIndex() );
        //extern GLViewTypes g_CurrentGLViewType;
        //cJSON_AddNumberToObject( pPrefs, "GameAspectRatio", g_CurrentGLViewType );

        cJSON_AddNumberToObject( jPrefs, "View_ShowEditorIcons", m_View_ShowEditorIcons );
        cJSON_AddNumberToObject( jPrefs, "Debug_DrawPhysicsDebugShapes", m_Debug_DrawPhysicsDebugShapes );
        cJSON_AddNumberToObject( jPrefs, "SelectedObjects_ShowWireframe", m_SelectedObjects_ShowWireframe );
        cJSON_AddNumberToObject( jPrefs, "SelectedObjects_ShowEffect", m_SelectedObjects_ShowEffect );

        //// Grid menu options
        cJSON_AddNumberToObject( jPrefs, "Grid_Visible", m_GridSettings.visible );
        cJSON_AddNumberToObject( jPrefs, "Grid_SnapEnabled", m_GridSettings.snapenabled );
        cJSONExt_AddFloatArrayToObject( jPrefs, "Grid_StepSize", &m_GridSettings.stepsize.x, 3 );

        //// Mode menu options
        cJSON_AddNumberToObject( jPrefs, "Mode_SwitchFocusOnPlayStop", m_Mode_SwitchFocusOnPlayStop );
        //cJSON_AddNumberToObject( jPrefs, "LaunchPlatform", GetLaunchPlatformIndex() );

        return jPrefs;
    }

    return 0;
}

void EditorPrefs::SaveFinish(cJSON* jPrefs)
{
    MyAssert( m_pSaveFile != 0 );
    MyAssert( jPrefs != 0 );

    char* string = cJSON_Print( jPrefs );
    cJSON_Delete( jPrefs );

    fprintf( m_pSaveFile, "%s", string );
    fclose( m_pSaveFile );
    m_pSaveFile = 0;

    cJSONExt_free( string );
}

void EditorPrefs::ToggleGrid_Visible()
{
    m_GridSettings.visible = !m_GridSettings.visible;
    g_pEngineCore->SetGridVisible( m_GridSettings.visible );
}

void EditorPrefs::ToggleGrid_SnapEnabled()
{
    m_GridSettings.snapenabled = !m_GridSettings.snapenabled;
}
