#ifdef WIN32
#define lowp
#define mediump
#else
precision mediump float;
#endif

#ifdef VertexShader

attribute vec4 a_Position;

uniform mat4 u_WorldViewProj;

#define NO_NORMALS 1
#include "Include/Bone_AttribsAndUniforms.glsl"
#include "Include/Bone_Functions.glsl"

void main()
{
    vec4 pos;
    ApplyBoneInfluencesToPositionAttribute( pos );

    gl_Position = u_WorldViewProj * pos;
}

#endif

#ifdef FragmentShader

uniform vec4 u_TextureTintColor;

void main()
{
    gl_FragColor = u_TextureTintColor;
}

#endif
