//
// Copyright (c) 2017 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "EngineCommonHeader.h"
#include "PrefabManager.h"

PrefabManager::PrefabManager()
{
}

PrefabManager::~PrefabManager()
{
}

unsigned int PrefabManager::GetNumberOfFiles()
{
#if MYFW_USING_WX
    return m_pPrefabFiles.size();
#else
    return m_pPrefabFiles.Count();
#endif
}

void PrefabManager::SetNumberOfFiles(unsigned int numfiles)
{
#if !MYFW_USING_WX
    m_pPrefabFiles.AllocateObjects( numfiles );
#endif
}

MyFileObject* PrefabManager::GetFile(unsigned int fileindex)
{
#if MYFW_USING_WX
    MyAssert( fileindex < m_pPrefabFiles.size() );
#else
    MyAssert( fileindex < m_pPrefabFiles.Count() );
#endif

    return m_pPrefabFiles[fileindex];
}

void PrefabManager::RequestFile(const char* prefabfilename)
{
    MyFileObject* pFile = g_pFileManager->RequestFile( prefabfilename );

#if MYFW_USING_WX
    m_pPrefabFiles.push_back( pFile );
#else
    MyAssert( m_pPrefabFiles.Count() < m_pPrefabFiles.Length() );
    m_pPrefabFiles.Add( pFile );
#endif
}

void PrefabManager::CreatePrefabInFile(unsigned int fileindex, const char* prefabname, GameObject* pGameObject)
{
}

#if MYFW_USING_WX
void PrefabManager::CreateFile(const char* relativepath)
{
    // create an empty stub of a file so it can be properly requested by our code
    FILE* pFile = 0;
#if MYFW_WINDOWS
    fopen_s( &pFile, relativepath, "wb" );
#else
    pFile = fopen( relativepath, "wb" );
#endif
    if( pFile )
    {
        fclose( pFile );
    }

    // if the file managed to save, request it.
    if( pFile != 0 )
    {
        RequestFile( relativepath );
    }
}

bool PrefabManager::CreateOrLoadFile()
{
    // Pick an existing file to load or create a new file
    {
        wxFileDialog FileDialog( g_pEngineMainFrame, _("Load or Create Prefab file"), "./Data/Prefabs", "", "Prefab files (*.myprefabs)|*.myprefabs", wxFD_OPEN );
    
        if( FileDialog.ShowModal() != wxID_CANCEL )
        {
            wxString wxpath = FileDialog.GetPath();
            char fullpath[MAX_PATH];
            sprintf_s( fullpath, MAX_PATH, "%s", (const char*)wxpath );
            const char* relativepath = GetRelativePath( fullpath );

            if( relativepath != 0 )
            {
                if( g_pFileManager->DoesFileExist( relativepath ) )
                {
                    RequestFile( relativepath );
                    return true;
                }
                else
                {
                    CreateFile( relativepath );
                    return true;
                }
            }
        }
    }

    return false;
}
#endif
