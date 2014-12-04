//
// Copyright (c) 2014 Jimmy Lord http://www.flatheadgames.com
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

#ifndef __GameObject_H__
#define __GameObject_H__

class GameObject : public CPPListNode
#if MYFW_USING_WX
, public wxEvtHandler
#endif
{
public:
    unsigned int m_ID;
    char* m_Name; // this is a pointer to the string passed in, not a copy.

    ComponentTransform* m_pComponentTransform;
    MyList<ComponentBase*> m_Components; // component system manager is responsible for deleting these components.

public:
    GameObject();
    virtual ~GameObject();

    cJSON* ExportAsJSONObject();
    void ImportFromJSONObject(cJSON* jsonobj);

    void SetName(char* name);

    ComponentBase* AddNewComponent(int componenttype, ComponentSystemManager* pComponentSystemManager = g_pComponentSystemManager);
    ComponentBase* AddExistingComponent(ComponentBase* pComponent);
    ComponentBase* RemoveComponent(ComponentBase* pComponent);

public:
#if MYFW_USING_WX
    static void StaticOnLeftClick(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnLeftClick(true); }
    static void StaticOnRightClick(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnRightClick(); }
    static void StaticOnDrag(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnDrag(); }
    static void StaticOnDrop(void* pObjectPtr) { ((GameObject*)pObjectPtr)->OnDrop(); }
    void OnLeftClick(bool clear);
    void OnRightClick();
    void OnPopupClick(wxEvent &evt); // used as callback for wxEvtHandler, can't be virtual(will crash, haven't looked into it).
    void OnDrag();
    void OnDrop();
#endif //MYFW_USING_WX
};

#endif //__GameObject_H__
