//
// Copyright (c) 2014-2015 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef __GameObject_H__
#define __GameObject_H__

class GameObject : public CPPListNode
#if MYFW_USING_WX
, public wxEvtHandler
#endif
{
    static const int MAX_COMPONENTS = 4; // TODO: fix this hardcodedness

protected:
    unsigned int m_SceneID; // 0 for runtime generated.
    unsigned int m_ID;
    char* m_Name; // this a copy of the string passed in.
    bool m_Managed;

public:
    ComponentTransform* m_pComponentTransform;
    MyList<ComponentBase*> m_Components; // component system manager is responsible for deleting these components.

public:
    GameObject(bool managed, int sceneid);
    virtual ~GameObject();

    static void LuaRegister(lua_State* luastate);

    cJSON* ExportAsJSONObject(bool savesceneid);
    void ImportFromJSONObject(cJSON* jsonobj, unsigned int sceneid);

    void SetSceneID(unsigned int sceneid);
    void SetID(unsigned int id);
    void SetName(const char* name);
    void SetManaged(bool managed);
    bool IsManaged() { return m_Managed; }

    unsigned int GetSceneID() { return m_SceneID; }
    unsigned int GetID() { return m_ID; }
    const char* GetName() { return m_Name; }

    ComponentBase* AddNewComponent(int componenttype, unsigned int sceneid, ComponentSystemManager* pComponentSystemManager = g_pComponentSystemManager);
    ComponentBase* AddExistingComponent(ComponentBase* pComponent, bool resetcomponent);
    ComponentBase* RemoveComponent(ComponentBase* pComponent);

    ComponentBase* GetFirstComponentOfBaseType(BaseComponentTypes basetype);
    ComponentBase* GetNextComponentOfBaseType(ComponentBase* pLastComponent);

    ComponentBase* GetFirstComponentOfType(const char* type);
    ComponentBase* GetNextComponentOfType(ComponentBase* pLastComponent);

    ComponentTransform* GetTransform() { return m_pComponentTransform; }
    ComponentCollisionObject* GetCollisionObject();

    // TODO: find a way to find an arbitrary component type that would be accessible from lua script.
    ComponentAnimationPlayer* GetAnimationPlayer();

    MaterialDefinition* GetMaterial();
    void SetMaterial(MaterialDefinition* pMaterial);

    void SetScriptFile(MyFileObject* pFile);

public:
#if MYFW_USING_WX
    static void StaticOnLeftClick(void* pObjectPtr, unsigned int count) { ((GameObject*)pObjectPtr)->OnLeftClick( count, true ); }
    static void StaticOnRightClick(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnRightClick(); }
    static void StaticOnDrag(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnDrag(); }
    static void StaticOnDrop(void* pObjectPtr, int controlid, wxCoord x, wxCoord y) { ((GameObject*)pObjectPtr)->OnDrop(controlid, x, y); }
    static void StaticOnLabelEdit(void* pObjectPtr, wxString newlabel) { ((GameObject*)pObjectPtr)->OnLabelEdit( newlabel ); }
    void OnLeftClick(unsigned int count, bool clear);
    void OnRightClick();
    void OnPopupClick(wxEvent &evt); // used as callback for wxEvtHandler, can't be virtual(will crash, haven't looked into it).
    void OnDrag();
    void OnDrop(int controlid, wxCoord x, wxCoord y);
    void OnLabelEdit(wxString newlabel);
#endif //MYFW_USING_WX
};

#endif //__GameObject_H__
