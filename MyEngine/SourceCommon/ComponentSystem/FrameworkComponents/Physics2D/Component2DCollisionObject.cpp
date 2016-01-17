//
// Copyright (c) 2015-2016 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "EngineCommonHeader.h"

#if MYFW_USING_WX
bool Component2DCollisionObject::m_PanelWatchBlockVisible = true;
#endif

const char* Physics2DPrimitiveTypeStrings[Physics2DPrimitive_NumTypes] = //ADDING_NEW_Physics2DPrimitiveType
{
    "Box",
    "Circle",
    "Edge",
    "Chain",
};

// Component Variable List
MYFW_COMPONENT_IMPLEMENT_VARIABLE_LIST( Component2DCollisionObject ); //_VARIABLE_LIST

Component2DCollisionObject::Component2DCollisionObject()
: ComponentBase()
{
    MYFW_COMPONENT_VARIABLE_LIST_CONSTRUCTOR(); //_VARIABLE_LIST

    ClassnameSanityCheck();

    m_BaseType = BaseComponentType_Data;

    //m_Type = ComponentType_2DCollisionObject;

    m_pComponentLuaScript = 0;

    m_pBox2DWorld = 0;
    m_pBody = 0;

    m_PrimitiveType = Physics2DPrimitiveType_Box;

    m_Scale.Set( 1,1,1 );

    m_Static = false;
    m_Density = 0;
    m_IsSensor = false;
    m_Friction = 0.2f;
    m_Restitution = 0;
    //m_pMesh = 0;
}

Component2DCollisionObject::~Component2DCollisionObject()
{
    MYFW_COMPONENT_VARIABLE_LIST_DESTRUCTOR(); //_VARIABLE_LIST

    if( m_pBody )
    {
        MyAssert( m_pBox2DWorld );
        m_pBox2DWorld->m_pWorld->DestroyBody( m_pBody );
    }

    //SAFE_RELEASE( m_pMesh );

#if !MYFW_USING_WX
    m_Vertices.FreeAllInList();
#endif
}

void Component2DCollisionObject::RegisterVariables(CPPListHead* pList, Component2DCollisionObject* pThis) //_VARIABLE_LIST
{
    ComponentVariable* pVars[6];

    pVars[0] = AddVarEnum( pList, "PrimitiveType", MyOffsetOf( pThis, &pThis->m_PrimitiveType ),   true, true, "Primitive Type", Physics2DPrimitive_NumTypes, Physics2DPrimitiveTypeStrings, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );

    pVars[1] = AddVar( pList, "Static",        ComponentVariableType_Bool,  MyOffsetOf( pThis, &pThis->m_Static ),          true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );
    pVars[2] = AddVar( pList, "Density",       ComponentVariableType_Float, MyOffsetOf( pThis, &pThis->m_Density ),         true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );
    pVars[3] = AddVar( pList, "IsSensor",      ComponentVariableType_Bool,  MyOffsetOf( pThis, &pThis->m_IsSensor ),        true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );
    pVars[4] = AddVar( pList, "Friction",      ComponentVariableType_Float, MyOffsetOf( pThis, &pThis->m_Friction ),        true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );
    pVars[5] = AddVar( pList, "Restitution",   ComponentVariableType_Float, MyOffsetOf( pThis, &pThis->m_Restitution ),     true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );

    //AddVar( pList, "Scale",         ComponentVariableType_Float, MyOffsetOf( pThis, &pThis->m_Scale ),           true, true, 0, (CVarFunc_ValueChanged)&Component2DCollisionObject::OnValueChanged, 0, 0 );
#if MYFW_USING_WX
    //for( int i=0; i<6; i++ )
    //    pVars[i]->AddCallback_ShouldVariableBeAdded( (CVarFunc_ShouldVariableBeAdded)(&Component2DCollisionObject::ShouldVariableBeAddedToWatchPanel) );
#endif //MYFW_USING_WX
}

void Component2DCollisionObject::Reset()
{
    ComponentBase::Reset();

    m_PrimitiveType = Physics2DPrimitiveType_Box;

    if( m_pBody )
    {
        MyAssert( m_pBox2DWorld );
        m_pBox2DWorld->m_pWorld->DestroyBody( m_pBody );
    }
    m_pBox2DWorld = 0;

    m_pComponentLuaScript = 0;

    m_Scale.Set( 1,1,1 );

    m_Static = false;
    m_Density = 0;
    m_IsSensor = false;
    m_Friction = 0.2f;
    m_Restitution = 0;
    //SAFE_RELEASE( m_pMesh );

#if MYFW_USING_WX
    m_pPanelWatchBlockVisible = &m_PanelWatchBlockVisible;
#endif //MYFW_USING_WX
}

#if MYFW_USING_LUA
void Component2DCollisionObject::LuaRegister(lua_State* luastate)
{
    luabridge::getGlobalNamespace( luastate )
        .beginClass<Component2DCollisionObject>( "Component2DCollisionObject" )
            .addData( "density", &Component2DCollisionObject::m_Density )
            .addFunction( "ApplyForce", &Component2DCollisionObject::ApplyForce )
            .addFunction( "ApplyLinearImpulse", &Component2DCollisionObject::ApplyLinearImpulse )
            .addFunction( "GetLinearVelocity", &Component2DCollisionObject::GetLinearVelocity )
            .addFunction( "GetMass", &Component2DCollisionObject::GetMass )            
        .endClass();
}
#endif //MYFW_USING_LUA

#if MYFW_USING_WX
void Component2DCollisionObject::AddToObjectsPanel(wxTreeItemId gameobjectid)
{
    //wxTreeItemId id =
    g_pPanelObjectList->AddObject( this, Component2DCollisionObject::StaticOnLeftClick, ComponentBase::StaticOnRightClick, gameobjectid, "2DCollisionObject" );
}

void Component2DCollisionObject::OnLeftClick(unsigned int count, bool clear)
{
    ComponentBase::OnLeftClick( count, clear );
}

void Component2DCollisionObject::FillPropertiesWindow(bool clear, bool addcomponentvariables, bool ignoreblockvisibleflag)
{
    m_ControlID_ComponentTitleLabel = g_pPanelWatch->AddSpace( "2D Collision Object", this, ComponentBase::StaticOnComponentTitleLabelClicked );

    if( m_PanelWatchBlockVisible || ignoreblockvisibleflag == true )
    {
        ComponentBase::FillPropertiesWindow( clear );

        FillPropertiesWindowWithVariables(); //_VARIABLE_LIST

        if( m_PrimitiveType == Physics2DPrimitiveType_Chain )
        {
            g_pPanelWatch->AddButton( "Edit Chain", 0, 0 );
        }
    }
}

bool Component2DCollisionObject::ShouldVariableBeAddedToWatchPanel(ComponentVariable* pVar)
{
    return true;
}

void* Component2DCollisionObject::OnDrop(ComponentVariable* pVar, wxCoord x, wxCoord y)
{
    void* oldvalue = 0;

    if( g_DragAndDropStruct.m_Type == DragAndDropType_ComponentPointer )
    {
        (ComponentBase*)g_DragAndDropStruct.m_Value;
    }

    if( g_DragAndDropStruct.m_Type == DragAndDropType_GameObjectPointer )
    {
        (GameObject*)g_DragAndDropStruct.m_Value;
    }

    return oldvalue;
}

void* Component2DCollisionObject::OnValueChanged(ComponentVariable* pVar, bool finishedchanging, double oldvalue)
{
    void* oldpointer = 0;

    if( pVar->m_Offset == MyOffsetOf( this, &m_PrimitiveType ) )
    {
        // TODO: rethink this, doesn't need refresh if panel isn't visible.
        g_pPanelWatch->m_NeedsRefresh = true;
    }

    // limit some properties
    if( pVar->m_Offset == MyOffsetOf( this, &m_Density ) )
    {
        if( m_Density < 0 )
            m_Density = 0;
    }

    if( pVar->m_Offset == MyOffsetOf( this, &m_Friction ) )
    {
        MyClamp( m_Friction, 0.0f, 1.0f );
    }

    if( pVar->m_Offset == MyOffsetOf( this, &m_Restitution ) )
    {
        MyClamp( m_Restitution, 0.0f, 1.0f );
    }

    // if a body exists, game is running, change the already existing body or fixture.
    if( m_pBody )
    {
        if( pVar->m_Offset == MyOffsetOf( this, &m_Static ) )
        {
            if( m_Static )
                m_pBody->SetType( b2_staticBody );
            else
                m_pBody->SetType( b2_dynamicBody );
        }

        if( pVar->m_Offset == MyOffsetOf( this, &m_Density ) )
        {
            for( b2Fixture* pFixture = m_pBody->GetFixtureList(); pFixture != 0; pFixture = pFixture->GetNext() )
            {
                pFixture->SetDensity( m_Density );
            }
            m_pBody->ResetMassData();
        }

        if( pVar->m_Offset == MyOffsetOf( this, &m_IsSensor ) )
        {
            for( b2Fixture* pFixture = m_pBody->GetFixtureList(); pFixture != 0; pFixture = pFixture->GetNext() )
            {
                pFixture->SetSensor( m_IsSensor );
            }
        }

        if( pVar->m_Offset == MyOffsetOf( this, &m_Friction ) )
        {
            for( b2Fixture* pFixture = m_pBody->GetFixtureList(); pFixture != 0; pFixture = pFixture->GetNext() )
            {
                pFixture->SetFriction( m_Friction );
            }
        }

        if( pVar->m_Offset == MyOffsetOf( this, &m_Restitution ) )
        {
            for( b2Fixture* pFixture = m_pBody->GetFixtureList(); pFixture != 0; pFixture = pFixture->GetNext() )
            {
                pFixture->SetRestitution( m_Restitution );
            }
        }
    }

    return oldpointer;
}

void Component2DCollisionObject::OnTransformPositionChanged(Vector3& newpos, bool changedbyeditor)
{
    if( changedbyeditor )
        SyncRigidBodyToTransform();
}
#endif //MYFW_USING_WX

cJSON* Component2DCollisionObject::ExportAsJSONObject(bool savesceneid)
{
    cJSON* jComponent = ComponentBase::ExportAsJSONObject( savesceneid );

#if MYFW_USING_WX
    int count = m_Vertices.size();
#else
    int count = m_Vertices.Count();
#endif

    if( count > 0 )
        cJSONExt_AddFloatArrayToObject( jComponent, "Vertices", &m_Vertices[0].x, count * 2 );

    return jComponent;
}

void Component2DCollisionObject::ImportFromJSONObject(cJSON* jsonobj, unsigned int sceneid)
{
    ComponentBase::ImportFromJSONObject( jsonobj, sceneid );

    unsigned int chasingid = 0;
    cJSON* jVertexArray = cJSON_GetObjectItem( jsonobj, "Vertices" );
    if( jVertexArray )
    {
        int arraysize = cJSON_GetArraySize( jVertexArray );

#if MYFW_USING_WX
        m_Vertices.resize( arraysize / 2 );
#else
        m_Vertices.FreeAllInList();
        m_Vertices.AllocateObjects( arraysize / 2 );
#endif

        cJSONExt_GetFloatArray( jsonobj, "Vertices", &m_Vertices[0].x, arraysize );
    }
}

Component2DCollisionObject& Component2DCollisionObject::operator=(const Component2DCollisionObject& other)
{
    MyAssert( &other != this );

    ComponentBase::operator=( other );

    // TODO: replace this with a CopyComponentVariablesFromOtherObject... or something similar.
    m_PrimitiveType = other.m_PrimitiveType;

    //m_Scale = other.m_Scale;

    m_Static = other.m_Static;
    m_Density = other.m_Density;
    m_IsSensor = other.m_IsSensor;
    m_Friction = other.m_Friction;
    m_Restitution = other.m_Restitution;

    //m_pMesh
    
    // TODO: fix copying chains
#if MYFW_USING_WX
    //m_Vertices = other.m_Vertices;
#else
    //m_Vertices = other.m_Vertices;
#endif

    return *this;
}

void Component2DCollisionObject::RegisterCallbacks()
{
    if( m_Enabled && m_CallbacksRegistered == false )
    {
        m_CallbacksRegistered = true;

        MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, Tick );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, OnSurfaceChanged );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, Draw );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, OnTouch );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, OnButtons );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, OnKeys );
        //MYFW_REGISTER_COMPONENT_CALLBACK( Component2DCollisionObject, OnFileRenamed );
    }
}

void Component2DCollisionObject::UnregisterCallbacks()
{
    if( m_CallbacksRegistered == true )
    {
        MYFW_UNREGISTER_COMPONENT_CALLBACK( Tick );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnSurfaceChanged );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( Draw );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnTouch );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnButtons );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnKeys );
        //MYFW_UNREGISTER_COMPONENT_CALLBACK( OnFileRenamed );

        m_CallbacksRegistered = false;
    }
}

//void Component2DCollisionObject::SetMesh(MyMesh* pMesh)
//{
//    if( pMesh )
//        pMesh->AddRef();
//
//    SAFE_RELEASE( m_pMesh );
//    m_pMesh = pMesh;
//}

void Component2DCollisionObject::OnPlay()
{
    ComponentBase::OnPlay();

    if( m_pComponentLuaScript == 0 )
    {
        m_pComponentLuaScript = (ComponentLuaScript*)m_pGameObject->GetFirstComponentOfType( "LuaScriptComponent" );
    }

    MyAssert( m_pGameObject->GetPhysicsSceneID() != 0 );
    SceneInfo* pSceneInfo = g_pComponentSystemManager->GetSceneInfo( m_pGameObject->GetPhysicsSceneID() );
    if( pSceneInfo )
    {
        m_pBox2DWorld = pSceneInfo->m_pBox2DWorld;
        MyAssert( m_pBox2DWorld );
    }

    MyAssert( m_pBody == 0 );
    if( m_pBody != 0 )
    {
        if( m_pBox2DWorld )
        {
            m_pBox2DWorld->m_pWorld->DestroyBody( m_pBody );
            m_pBody = 0;
        }
    }

    CreateBody();
}

void Component2DCollisionObject::OnStop()
{
    ComponentBase::OnStop();

    // shouldn't get hit, all objects are deleted/recreated when gameplay is stopped.
    if( m_pBody )
    {
        m_pBox2DWorld->m_pWorld->DestroyBody( m_pBody );
        m_pBody = 0;
        m_pBox2DWorld = 0;
    }
}

void Component2DCollisionObject::CreateBody()
{
    MyAssert( m_pBody == 0 );
    MyAssert( m_pBox2DWorld );

    if( m_pBody != 0 )
        return;

    if( m_pBox2DWorld == 0 )
        return;

    // create a body on start
    if( m_pBody == 0 )
    {
        Vector3 pos = m_pGameObject->m_pComponentTransform->GetPosition();
        Vector3 rot = m_pGameObject->m_pComponentTransform->GetLocalRotation();

        b2BodyDef bodydef;
        
        bodydef.position = b2Vec2( pos.x, pos.y );
        bodydef.angle = -rot.z / 180 * PI;
        if( m_Static )
            bodydef.type = b2_staticBody;
        else
            bodydef.type = b2_dynamicBody;

        m_pBody = m_pBox2DWorld->m_pWorld->CreateBody( &bodydef );
        m_pBody->SetUserData( this );

        m_Scale = m_pGameObject->m_pComponentTransform->GetLocalScale();

        switch( m_PrimitiveType )
        {
        case Physics2DPrimitiveType_Box:
            {
                b2PolygonShape boxshape;
                boxshape.SetAsBox( 0.5f * m_Scale.x, 0.5f * m_Scale.y );

                b2FixtureDef fixturedef;
                fixturedef.shape = &boxshape;
                fixturedef.density = m_Density;
                fixturedef.isSensor = m_IsSensor;
                fixturedef.friction = m_Friction;
                fixturedef.restitution = m_Restitution;

                m_pBody->CreateFixture( &fixturedef );
            }
            break;

        case Physics2DPrimitiveType_Circle:
            {
                b2CircleShape circleshape;
                circleshape.m_p.Set( 0, 0 );
                circleshape.m_radius = m_Scale.x;

                b2FixtureDef fixturedef;
                fixturedef.shape = &circleshape;
                fixturedef.density = m_Density;
                fixturedef.isSensor = m_IsSensor;
                fixturedef.friction = m_Friction;
                fixturedef.restitution = m_Restitution;

                m_pBody->CreateFixture( &fixturedef );
            }
            break;

        case Physics2DPrimitiveType_Edge:
            {
                b2EdgeShape edgeshape;

                // TODO: define edges so they're not limited to x/y axes.
                if( m_Scale.x > m_Scale.y )
                    edgeshape.Set( b2Vec2( -0.5f * m_Scale.x, 0 ), b2Vec2( 0.5f * m_Scale.x, 0 ) );
                else
                    edgeshape.Set( b2Vec2( 0, -0.5f * m_Scale.y ), b2Vec2( 0, 0.5f * m_Scale.y ) );

                b2FixtureDef fixturedef;
                fixturedef.shape = &edgeshape;
                fixturedef.density = m_Density;
                fixturedef.isSensor = m_IsSensor;
                fixturedef.friction = m_Friction;
                fixturedef.restitution = m_Restitution;

                m_pBody->CreateFixture( &fixturedef );
            }
            break;

        case Physics2DPrimitiveType_Chain:
            {
                b2ChainShape chainshape;

#if MYFW_USING_WX
                int count = m_Vertices.size();
#else
                int count = m_Vertices.Count();
#endif
                if( count == 0 )
                {
                    m_Vertices.push_back( b2Vec2( -5, 0 ) );
                    m_Vertices.push_back( b2Vec2( 5, 0 ) );
                    count = 2;
                }

                if( count > 0 )
                {
                    chainshape.CreateChain( &m_Vertices[0], count );

                    b2FixtureDef fixturedef;
                    fixturedef.shape = &chainshape;
                    fixturedef.density = m_Density;
                    fixturedef.isSensor = m_IsSensor;
                    fixturedef.friction = m_Friction;
                    fixturedef.restitution = m_Restitution;

                    m_pBody->CreateFixture( &fixturedef );
                }
            }
        }
    }
}

void Component2DCollisionObject::TickCallback(double TimePassed)
{
    //ComponentBase::Tick( TimePassed );

    if( TimePassed == 0 )
        return;

    if( m_pBody == 0 )
        return;

    b2Vec2 pos = m_pBody->GetPosition();
    float32 angle = -m_pBody->GetAngle() / PI * 180.0f;

    MyMatrix* matLocal = m_pGameObject->m_pComponentTransform->GetLocalTransform();

    Vector3 oldpos = m_pGameObject->m_pComponentTransform->GetPosition();
    Vector3 oldrot = m_pGameObject->m_pComponentTransform->GetLocalRotation();

    matLocal->SetIdentity();
    matLocal->CreateSRT( m_Scale, Vector3( 0, 0, angle ), Vector3( pos.x, pos.y, oldpos.z ) );

#if MYFW_USING_WX
    m_pGameObject->m_pComponentTransform->UpdatePosAndRotFromLocalMatrix();
#endif
}

void Component2DCollisionObject::SyncRigidBodyToTransform()
{
    if( m_pBody == 0 )
        return;
}

void Component2DCollisionObject::ApplyForce(Vector2 force, Vector2 point)
{
    b2Vec2 b2force = b2Vec2( force.x, force.y );
    m_pBody->ApplyForce( b2force, m_pBody->GetWorldCenter(), true );
}

void Component2DCollisionObject::ApplyLinearImpulse(Vector2 impulse, Vector2 point)
{
    b2Vec2 b2impulse = b2Vec2( impulse.x, impulse.y );
    m_pBody->ApplyLinearImpulse( b2impulse, m_pBody->GetWorldCenter(), true );
}

Vector2 Component2DCollisionObject::GetLinearVelocity()
{
    b2Vec2 b2velocity = m_pBody->GetLinearVelocity();
    return Vector2( b2velocity.x, b2velocity.y );
}

float Component2DCollisionObject::GetMass()
{
    return m_pBody->GetMass();
}
