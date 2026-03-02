#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in vec3 a_Color;
layout(location = 6) in float a_PaletteIndex;

// Instanced attributes (for VAO compatibility)
layout(location = 7)  in vec4 a_InstanceTransform0;
layout(location = 8)  in vec4 a_InstanceTransform1;
layout(location = 9)  in vec4 a_InstanceTransform2;
layout(location = 10) in vec4 a_InstanceTransform3;
layout(location = 11) in int  a_InstanceEntityID;

uniform mat4 u_LightSpaceVP;
uniform mat4 u_Transform;
uniform int u_UseInstancing;

void main()
{
    mat4 transform;
    if (u_UseInstancing == 1)
    {
        transform = mat4(a_InstanceTransform0, a_InstanceTransform1,
                         a_InstanceTransform2, a_InstanceTransform3);
    }
    else
    {
        transform = u_Transform;
    }

    gl_Position = u_LightSpaceVP * transform * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core

void main()
{
    // Depth is written automatically
}
