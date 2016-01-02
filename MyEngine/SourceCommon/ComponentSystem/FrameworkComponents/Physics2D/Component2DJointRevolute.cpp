//
// Copyright (c) 2016 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "EngineCommonHeader.h"

#if MYFW_USING_WX
bool Component2DJointRevolute::m_PanelWatchBlockVisible = true;
#endif

// Component Variable List
MYFW_COMPONENT_IMPLEMENT_VARIABLE_LIST( Component2DJointRevolute ); //_VARIABLE_LIST

Component2DJointRevolute::Component2DJointRevolute()
: ComponentBase()
{
    MYFW_COMPONENT_VARIABLE_LIST_CONSTRUCTOR(); //_VARIABLE_LIST

    ClassnameSanityCheck();

    m_BaseType = BaseComponentType_Data;

    m_pSecondCollisionObject = 0;
    
    m_AnchorA.Set( 0, 0 );
    m_AnchorB.Set( 0, 0 );

    m_pBody = 0;
    m_pSecondBody = 0;
}

Component2DJointRevolute::~Component2DJointRevolute()
{
    MYFW_COMPONENT_VARIABLE_LIST_DESTRUCTOR(); //_VARIABLE_LIST
}

void Component2DJointRevolute::RegisterVariables(CPPListHead* pList, Component2DJointRevolute* pThis) //_VARIABLE_LIST
{
    AddVar( pList, "SecondCollisionObject", ComponentVariableType_ComponentPtr,
        MyOffsetOf( pThis, &pThis->m_pSecondCollisionObject ), true, true, 0,
        (CVarFunc_ValueChanged)&Component2DJointRevolute::OnValueChanged, (CVarFunc_DropTarget)&Component2DJointRevolute::OnDrop, 0 );
    //AddVar( pList, "AnchorA", ComponentVariableType_Vector2, MyOffsetOf( pThis, &pThis->m_AnchorA ), true, true, 0, (CVarFunc_ValueChanged)&Component2DJointRevolute::OnValueChanged, 0, 0 );
    AddVar( pList, "AnchorB", ComponentVariableType_Vector2, MyOffsetOf( pThis, &pThis->m_AnchorB ), true, true, "Anchor Offset", (CVarFunc_ValueChanged)&Component2DJointRevolute::OnValueChanged, 0, 0 );
}

void Component2DJointRevolute::Reset()
{
    ComponentBase::Reset();

    m_pSecondCollisionObject = 0;

    m_AnchorA.Set( 0, 0 );
    m_AnchorB.Set( 0, 0 );

    m_pBody = 0;
    m_pSecondBody = 0;

#if MYFW_USING_WX
    m_pPanelWatchBlockVisible = &m_PanelWatchBlockVisible;
#endif //MYFW_USING_WX
}

#if MYFW_USING_LUA
void Component2DJointRevolute::LuaRegister(lua_State* luastate)
{
    //luabridge::getGlobalNamespace( luastate )
    //    .beginClass<Component2DJointRevolute>( "Component2DJointRevolute" )
    //        .addData( "density", &Component2DJointRevolute::m_Density )
    //        .addFunction( "GetMass", &Component2DJointRevolute::GetMass )            
    //    .endClass();
}
#endif //MYFW_USING_LUA

#if MYFW_USING_WX
void Component2DJointRevolute::AddToObjectsPanel(wxTreeItemId gameobjectid)
{
    //wxTreeItemId id =
    g_pPanelObjectList->AddObject( this, Component2DJointRevolute::StaticOnLeftClick, ComponentBase::StaticOnRightClick, gameobjectid, "2DJointRevolute" );
}

void Component2DJointRevolute::OnLeftClick(unsigned int count, bool clear)
{
    ComponentBase::OnLeftClick( count, clear );
}

void Component2DJointRevolute::FillPropertiesWindow(bool clear, bool addcomponentvariables, bool ignoreblockvisibleflag)
{
    m_ControlID_ComponentTitleLabel = g_pPanelWatch->AddSpace( "2D Revolute Joint", this, ComponentBase::StaticOnComponentTitleLabelClicked );

    if( m_PanelWatchBlockVisible || ignoreblockvisibleflag == true )
    {
        ComponentBase::FillPropertiesWindow( clear );

        FillPropertiesWindowWithVariables(); //_VARIABLE_LIST
    }
}

void* Component2DJointRevolute::OnDrop(ComponentVariable* pVar, wxCoord x, wxCoord y)
{
    void* oldvalue = m_pSecondCollisionObject;

    if( g_DragAndDropStruct.m_Type == DragAndDropType_ComponentPointer )
    {
        ComponentBase* pComponent = (ComponentBase*)g_DragAndDropStruct.m_Value;

        if( pComponent->IsA( "2DCollisionObjectComponent" ) )
        {
            m_pSecondCollisionObject = (Component2DCollisionObject*)pComponent;
        }        
    }

    if( g_DragAndDropStruct.m_Type == DragAndDropType_GameObjectPointer )
    {
        GameObject* pGameObject = (GameObject*)g_DragAndDropStruct.m_Value;

        m_pSecondCollisionObject = pGameObject->Get2DCollisionObject();
    }

    return oldvalue;
}

void* Component2DJointRevolute::OnValueChanged(ComponentVariable* pVar, bool finishedchanging, double oldvalue)
{
    void* oldpointer = 0;

    if( pVar->m_Offset == MyOffsetOf( this, &m_pSecondCollisionObject ) )
    {
        MyAssert( pVar->m_ControlID != -1 );

        wxString text = g_pPanelWatch->m_pVariables[pVar->m_ControlID].m_Handle_TextCtrl->GetValue();
        if( text == "" || text == "none" )
        {
            g_pPanelWatch->ChangeDescriptionForPointerWithDescription( pVar->m_ControlID, "none" );
            oldpointer = this->m_pSecondCollisionObject;
            m_pSecondCollisionObject = 0;
        }
    }

    //if( pVar->m_Offset == MyOffsetOf( this, &m_PrimitiveType ) )
    //{
    //    // TODO: rethink this, doesn't need refresh if panel isn't visible.
    //    g_pPanelWatch->m_NeedsRefresh = true;
    //}

    //if( pVar->m_Offset == MyOffsetOf( this, &m_SampleVector3 ) )
    //{
    //    MyAssert( pVar->m_ControlID != -1 );
    //}

    return oldpointer;
}
#endif //MYFW_USING_WX

Component2DJointRevolute& Component2DJointRevolute::operator=(const Component2DJointRevolute& other)
{
    MyAssert( &other != this );

    ComponentBase::operator=( other );

    // TODO: replace this with a CopyComponentVariablesFromOtherObject... or something similar.
    m_pSecondCollisionObject = other.m_pSecondCollisionObject;

    m_AnchorA = other.m_AnchorA;
    m_AnchorB = other.m_AnchorB;

    m_pBody = other.m_pBody;
    m_pSecondBody = other.m_pSecondBody;

    return *this;
}

void Component2DJointRevolute::RegisterCallbacks()
{
    if( m_Enabled && m_CallbacksRegistered == false )
    {
        m_CallbacksRegistered = true;

        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, Tick );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, OnSurfaceChanged );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, Draw );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, OnTouch );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, OnButtons );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, OnKeys );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DJointRevolute, OnFileRenamed );
    }
}

void Component2DJointRevolute::UnregisterCallbacks()
{
    if( m_CallbacksRegistered == true )
    {
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( Tick );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnSurfaceChanged );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( Draw );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnTouch );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnButtons );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnKeys );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnFileRenamed );

        m_CallbacksRegistered = false;
    }
}

void Component2DJointRevolute::OnPlay()
{
    ComponentBase::OnPlay();

    m_pBody = m_pGameObject->Get2DCollisionObject()->m_pBody;
    if( m_pSecondCollisionObject )
        m_pSecondBody = m_pSecondCollisionObject->m_pBody;

    if( m_pBody )
    {
        if( m_pSecondBody )
        {
            Vector3 posA = m_pGameObject->m_pComponentTransform->GetPosition();
            Vector3 posB = m_pSecondCollisionObject->m_pGameObject->m_pComponentTransform->GetPosition();
            //b2Vec2 anchorpos( posB.x - posA.x + m_AnchorA.x, posB.y - posA.y + m_AnchorA.y );
            b2Vec2 anchorpos( posB.x - posA.x, posB.y - posA.y );

            b2RevoluteJointDef jointdef;

            jointdef.bodyA = m_pBody;
            jointdef.bodyB = m_pSecondBody;
            jointdef.collideConnected = false;
            jointdef.localAnchorA = anchorpos;//m_AnchorA.x, m_AnchorA.y );
            jointdef.localAnchorB.Set( m_AnchorB.x, m_AnchorB.y );

            g_pBox2DWorld->m_pWorld->CreateJoint( &jointdef );
        }
        else
        {
            Vector3 pos = m_pGameObject->m_pComponentTransform->GetPosition();
            //b2Vec2 anchorpos( pos.x + m_AnchorA.x, pos.y + m_AnchorA.y );
            b2Vec2 anchorpos( pos.x + m_AnchorB.x, pos.y + m_AnchorB.y );

            b2RevoluteJointDef jointdef;
            jointdef.Initialize( g_pBox2DWorld->m_pGround, m_pBody, anchorpos );

            g_pBox2DWorld->m_pWorld->CreateJoint( &jointdef );
        }
    }
}

void Component2DJointRevolute::OnStop()
{
    ComponentBase::OnStop();
}
