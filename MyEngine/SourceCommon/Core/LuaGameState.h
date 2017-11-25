//
// Copyright (c) 2014-2017 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef __LuaGameState_H__
#define __LuaGameState_H__

class LuaGameState;

extern LuaGameState* g_pLuaGameState;

void LuaBridgeExt_LogExceptionFormattedForVisualStudioOutputWindow(const char* userdata, const char* fullpath, const char* what);

class LuaGameState
{
public:
    struct Breakpoint
    {
        char file[MAX_PATH];
        int line;
    };

    lua_State* m_pLuaState;

    int m_ListenSocket;
    int m_DebugSocket;

    // Used by step-in/out/over.
    int m_NextLineToBreakOn;
    char m_NextSourceFileToBreakOn[MAX_PATH];

    // For breakpoints.
    std::vector<Breakpoint> m_Breakpoints;

public:
    LuaGameState();
    virtual ~LuaGameState();

    virtual void Rebuild();
    virtual void RegisterClasses();

    void Tick();

    void CheckForDebugNetworkMessages(bool block);
    bool DealWithDebugNetworkMessages(char* message, bool wasblocking);

    void SendStoppedMessage();
    int AddStackToJSONMessage(cJSON* jMessage);
    void AddUserDataToJSONArray(cJSON* jArray, cJSON* jObject, const char* name);
    void AddValueAtTopOfStackToJSONArray(cJSON* jArray, const char* name);
    int AddLocalVarsToStackInJSONMessage(cJSON* jStack, lua_Debug* ar);

    void ClearAllBreakpoints();
    void ClearAllBreakpointsFromFile(char* fullpath);
    void AddBreakpoint(char* fullpath, int line);
};

#endif //__LuaGameState_H__
