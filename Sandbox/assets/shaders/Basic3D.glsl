#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec2 v_TexCoord;
out vec3 v_NormalWS;

void main()
{
    v_TexCoord = a_TexCoord;

    // World-space normal (correct for non-uniform scaling)
    mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));
    v_NormalWS = normalize(normalMatrix * a_Normal);

    vec4 worldPos = u_Transform * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

in vec2 v_TexCoord;
in vec3 v_NormalWS; // currently unused, kept for future lighting

uniform sampler2D u_Texture;

out vec4 FragColor;

void main()
{
    FragColor = texture(u_Texture, v_TexCoord);
}
