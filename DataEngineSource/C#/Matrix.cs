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
using System.Runtime.InteropServices;

namespace MyEngine
{ 
    [StructLayout(LayoutKind.Sequential)]
    public class mat4
    {
        // Values are stored column major.
        // m11 m21 m31 m41       Sx  0  0 Tx
        // m12 m22 m32 m42  --\   0 Sy  0 Ty
        // m13 m23 m33 m43  --/   0  0 Sz Tz
        // m14 m24 m34 m44        0  0  0  1
        public float m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44;

        public mat4()
        {
            m11 = 11;
            m12 = 12;
            m13 = 13;
            m14 = 14;
        }

        public void SetIdentity() { GCHandle h = GCHandle.Alloc(this, GCHandleType.Pinned); SetIdentity( h.AddrOfPinnedObject() ); h.Free(); }
        public void CreateSRT(vec3 scale, vec3 rot, vec3 pos)
        {
            GCHandle hThis = GCHandle.Alloc(this, GCHandleType.Pinned);
            GCHandle hScale = GCHandle.Alloc(scale, GCHandleType.Pinned);
            GCHandle hRot = GCHandle.Alloc(rot, GCHandleType.Pinned);
            GCHandle hPos = GCHandle.Alloc(pos, GCHandleType.Pinned);
            CreateSRT( hThis.AddrOfPinnedObject(), hScale.AddrOfPinnedObject(), hRot.AddrOfPinnedObject(), hPos.AddrOfPinnedObject() );
            hThis.Free();
            hScale.Free();
            hRot.Free();
            hPos.Free();
        }

        [MethodImpl(MethodImplOptions.InternalCall)] public extern static void SetIdentity(IntPtr pThis);
        [MethodImpl(MethodImplOptions.InternalCall)] public extern static void CreateSRT(IntPtr pThis, IntPtr scale, IntPtr rot, IntPtr pos);
    }
}
