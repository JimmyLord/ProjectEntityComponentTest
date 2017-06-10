//
// Copyright (c) 2016-2017 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "EngineCommonHeader.h"

#if MYFW_USING_WX
bool ComponentAudioPlayer::m_PanelWatchBlockVisible = true;
#endif

// Component Variable List
MYFW_COMPONENT_IMPLEMENT_VARIABLE_LIST( ComponentAudioPlayer ); //_VARIABLE_LIST

ComponentAudioPlayer::ComponentAudioPlayer()
: ComponentBase()
{
    MYFW_COMPONENT_VARIABLE_LIST_CONSTRUCTOR(); //_VARIABLE_LIST

    ClassnameSanityCheck();

    m_BaseType = BaseComponentType_Data;

    m_SoundCueName[0] = 0;
    m_pSoundCue = 0;

    m_ChannelSoundIsPlayingOn = -1;
}

ComponentAudioPlayer::~ComponentAudioPlayer()
{
    SAFE_RELEASE( m_pSoundCue );

    MYFW_COMPONENT_VARIABLE_LIST_DESTRUCTOR(); //_VARIABLE_LIST
}

void ComponentAudioPlayer::RegisterVariables(CPPListHead* pList, ComponentAudioPlayer* pThis) //_VARIABLE_LIST
{
    // just want to make sure these are the same on all compilers.  They should be since this is a simple class.
#if MYFW_IOS || MYFW_OSX || MYFW_NACL
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    MyAssert( offsetof( ComponentAudioPlayer, m_pSoundCue ) == MyOffsetOf( pThis, &pThis->m_pSoundCue ) );
#if MYFW_IOS || MYFW_OSX
#pragma GCC diagnostic default "-Winvalid-offsetof"
#endif

    AddVar( pList, "Cue", ComponentVariableType_SoundCuePtr, MyOffsetOf( pThis, &pThis->m_pSoundCue ), false, true, 0, (CVarFunc_ValueChanged)&ComponentAudioPlayer::OnValueChanged, (CVarFunc_DropTarget)&ComponentAudioPlayer::OnDrop, 0 );
}

void ComponentAudioPlayer::Reset()
{
    ComponentBase::Reset();

    //SAFE_RELEASE( m_pSoundCue );
    m_SoundCueName[0] = 0;
    m_pSoundCue = 0;

#if MYFW_USING_WX
    m_pPanelWatchBlockVisible = &m_PanelWatchBlockVisible;
#endif //MYFW_USING_WX
}

#if MYFW_USING_LUA
void ComponentAudioPlayer::LuaRegister(lua_State* luastate)
{
    luabridge::getGlobalNamespace( luastate )
        .beginClass<ComponentAudioPlayer>( "ComponentAudioPlayer" )
            //.addData( "m_SampleVector3", &ComponentAudioPlayer::m_SampleVector3 )
            .addFunction( "PlaySound", &ComponentAudioPlayer::PlaySound )
        .endClass();
}
#endif //MYFW_USING_LUA

#if MYFW_USING_WX
void ComponentAudioPlayer::AddToObjectsPanel(wxTreeItemId gameobjectid)
{
    //wxTreeItemId id =
    g_pPanelObjectList->AddObject( this, ComponentAudioPlayer::StaticOnLeftClick, ComponentBase::StaticOnRightClick, gameobjectid, "AudioPlayer", ObjectListIcon_Component );
}

void ComponentAudioPlayer::OnLeftClick(unsigned int count, bool clear)
{
    ComponentBase::OnLeftClick( count, clear );
}

void ComponentAudioPlayer::FillPropertiesWindow(bool clear, bool addcomponentvariables, bool ignoreblockvisibleflag)
{
    m_ControlID_ComponentTitleLabel = g_pPanelWatch->AddSpace( "Audio Player", this, ComponentBase::StaticOnComponentTitleLabelClicked );

    // Check for the sound cue if it's null, it might not have been set on initial load if sound cue file wasn't loaded.
    if( m_pSoundCue == 0 && m_SoundCueName[0] != 0 )
    {
        SetSoundCue( g_pGameCore->m_pSoundManager->FindCueByName( m_SoundCueName ) );
    }

    if( m_PanelWatchBlockVisible || ignoreblockvisibleflag == true )
    {
        ComponentBase::FillPropertiesWindow( clear );

        FillPropertiesWindowWithVariables(); //_VARIABLE_LIST

        g_pPanelWatch->AddButton( "Play Sound", this, -1, ComponentAudioPlayer::StaticOnButtonPlaySound );
    }
}

void* ComponentAudioPlayer::OnDrop(ComponentVariable* pVar, wxCoord x, wxCoord y)
{
    void* oldvalue = 0;

    if( g_DragAndDropStruct.m_Type == DragAndDropType_SoundCuePointer )
    {
        //SetSoundCue( (SoundCue*)g_DragAndDropStruct.m_Value );
        g_pEngineMainFrame->m_pCommandStack->Do( MyNew EditorCommand_ChangeSoundCue( this, (SoundCue*)g_DragAndDropStruct.m_Value ) );

        // update the panel so new sound cue name shows up.
        g_pPanelWatch->ChangeDescriptionForPointerWithDescription( pVar->m_ControlID, m_SoundCueName );
    }

    //if( g_DragAndDropStruct.m_Type == DragAndDropType_ComponentPointer )
    //{
    //    (ComponentBase*)g_DragAndDropStruct.m_Value;
    //}

    //if( g_DragAndDropStruct.m_Type == DragAndDropType_GameObjectPointer )
    //{
    //    (GameObject*)g_DragAndDropStruct.m_Value;
    //}

    return oldvalue;
}

void* ComponentAudioPlayer::OnValueChanged(ComponentVariable* pVar, bool changedbyinterface, bool finishedchanging, double oldvalue, ComponentVariableValue newvalue)
{
    void* oldpointer = 0;

    if( pVar->m_Offset == MyOffsetOf( this, &m_pSoundCue ) )
    {
        if( changedbyinterface )
        {
            wxString text = g_pPanelWatch->GetVariableProperties( pVar->m_ControlID )->GetTextCtrl()->GetValue();
            if( text == "" || text == "none" )
            {
                g_pPanelWatch->ChangeDescriptionForPointerWithDescription( pVar->m_ControlID, "no sound cue" );

                oldpointer = m_pSoundCue;
                
                // Set the current sound cue to null.
                g_pEngineMainFrame->m_pCommandStack->Do( MyNew EditorCommand_ChangeSoundCue( this, 0 ) );
            }
        }
        else if( newvalue.GetSoundCuePtr() != 0 )
        {
            oldpointer = m_pSoundCue;

            SetSoundCue( newvalue.GetSoundCuePtr() );
        }
    }

    return oldpointer;
}

void ComponentAudioPlayer::OnButtonPlaySound(int buttonid)
{
    // Check for the sound cue if it's null, it might not have been set on initial load if sound cue file wasn't loaded.
    if( m_pSoundCue == 0 && m_SoundCueName[0] != 0 )
    {
        SetSoundCue( g_pGameCore->m_pSoundManager->FindCueByName( m_SoundCueName ) );
        if( m_pSoundCue )
            g_pPanelWatch->SetNeedsRefresh();
    }

    if( m_pSoundCue == 0 )
        return;

    if( m_ChannelSoundIsPlayingOn != -1 )
        g_pGameCore->m_pSoundPlayer->StopSound( m_ChannelSoundIsPlayingOn );
    m_ChannelSoundIsPlayingOn = g_pGameCore->m_pSoundManager->PlayCue( m_pSoundCue );
}
#endif //MYFW_USING_WX

cJSON* ComponentAudioPlayer::ExportAsJSONObject(bool savesceneid, bool saveid)
{
    cJSON* jComponent = ComponentBase::ExportAsJSONObject( savesceneid, saveid );

    if( m_pSoundCue )
    {
        if( m_pSoundCue )
        {
            MyAssert( strcmp( m_SoundCueName, m_pSoundCue->m_Name ) == 0 );
        }

        cJSON_AddStringToObject( jComponent, "Cue", m_SoundCueName );
    }

    ExportVariablesToJSON( jComponent ); //_VARIABLE_LIST

    return jComponent;
}

void ComponentAudioPlayer::ImportFromJSONObject(cJSON* jComponent, unsigned int sceneid)
{
    ComponentBase::ImportFromJSONObject( jComponent, sceneid );

    cJSON* scriptstringobj = cJSON_GetObjectItem( jComponent, "Cue" );
    if( scriptstringobj )
    {
        SoundCue* pSoundCue = g_pGameCore->m_pSoundManager->FindCueByName( m_SoundCueName );
        if( pSoundCue )
        {
            SetSoundCue( pSoundCue );
        }
        else
        {
            // Set the name of the cue we want to load for later.
            strcpy_s( m_SoundCueName, MAX_SOUND_CUE_NAME_LEN, scriptstringobj->valuestring );
        }
    }

    ImportVariablesFromJSON( jComponent ); //_VARIABLE_LIST
}

ComponentAudioPlayer& ComponentAudioPlayer::operator=(const ComponentAudioPlayer& other)
{
    MyAssert( &other != this );

    ComponentBase::operator=( other );

    SetSoundCue( other.m_pSoundCue );

    return *this;
}

void ComponentAudioPlayer::RegisterCallbacks()
{
    if( m_Enabled && m_CallbacksRegistered == false )
    {
        m_CallbacksRegistered = true;

#if MYFW_USING_WX
        MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, Tick );
#endif
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, OnSurfaceChanged );
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, Draw );
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, OnTouch );
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, OnButtons );
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, OnKeys );
        //MYFW_REGISTER_COMPONENT_CALLBACK( ComponentAudioPlayer, OnFileRenamed );
    }
}

void ComponentAudioPlayer::UnregisterCallbacks()
{
    if( m_CallbacksRegistered == true )
    {
#if MYFW_USING_WX
        MYFW_UNREGISTER_COMPONENT_CALLBACK( Tick );
#endif
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnSurfaceChanged );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( Draw );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnTouch );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnButtons );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnKeys );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnFileRenamed );

        m_CallbacksRegistered = false;
    }
}

#if MYFW_USING_WX
void ComponentAudioPlayer::TickCallback(double TimePassed)
{
    //ComponentBase::TickCallback( TimePassed );

    // In editor mode, continually check for sound cue pointer then unregister tick callback one found.
    if( m_pSoundCue == 0 && m_SoundCueName[0] != 0 )
    {
        SoundCue* pSoundCue = g_pGameCore->m_pSoundManager->FindCueByName( m_SoundCueName );

        if( pSoundCue )
        {
            SetSoundCue( pSoundCue );
        }
    }

    if( m_pSoundCue != 0 )
    {
        MYFW_UNREGISTER_COMPONENT_CALLBACK( Tick );
    }
}
#endif //MYFW_USING_WX

void ComponentAudioPlayer::PlaySound(bool fireAndForget)
{
    // Check for the sound cue if it's null, it might not have been set on initial load if sound cue file wasn't loaded.
    if( m_pSoundCue == 0 && m_SoundCueName[0] != 0 )
    {
        SetSoundCue( g_pGameCore->m_pSoundManager->FindCueByName( m_SoundCueName ) );
    }

    if( m_pSoundCue == 0 )
        return;

    if( fireAndForget == false )
    {
        g_pGameCore->m_pSoundPlayer->StopSound( m_ChannelSoundIsPlayingOn );
        m_ChannelSoundIsPlayingOn = g_pGameCore->m_pSoundManager->PlayCue( m_pSoundCue );
    }
    else
    {
        g_pGameCore->m_pSoundManager->PlayCue( m_pSoundCue );
    }
}

void ComponentAudioPlayer::SetSoundCue(SoundCue* pCue)
{
    if( pCue )
        pCue->AddRef();
    SAFE_RELEASE( m_pSoundCue );

    if( pCue )
    {
        m_pSoundCue = pCue;

        strcpy_s( m_SoundCueName, MAX_SOUND_CUE_NAME_LEN, pCue->m_Name );
    }
    else
    {
        m_SoundCueName[0] = 0;
    }

#if MYFW_USING_WX
    g_pPanelWatch->SetNeedsRefresh();
#endif
}
