//
// Copyright (c) 2019 Jimmy Lord http://www.flatheadgames.com
//
// This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

using System;
using System.Runtime.CompilerServices;

namespace MyEngine
{ 
    public class GameObject
    {
        private IntPtr m_pNativeObject = (IntPtr)null;

        public vec3 GetPosition() { return GameObject.GetPosition( m_pNativeObject ); }
        [MethodImpl(MethodImplOptions.InternalCall)] private extern static vec3 GetPosition(IntPtr pNativeObject);

        public void SetPosition(vec3 pos) { GameObject.SetPosition( m_pNativeObject, pos.x, pos.y, pos.z ); }
        [MethodImpl(MethodImplOptions.InternalCall)] private extern static void SetPosition(IntPtr pNativeObject, float x, float y, float z);

        public void SetLocalTransform(mat4 transform) { GameObject.SetLocalTransform( m_pNativeObject, transform ); }
        [MethodImpl(MethodImplOptions.InternalCall)] private extern static void SetLocalTransform(IntPtr pNativeObject, mat4 transform);
    }
}
