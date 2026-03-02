#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in vec3 a_Color;
layout(location = 6) in float a_PaletteIndex;

// Instanced attributes (only active when u_UseInstancing == 1)
layout(location = 7)  in vec4 a_InstanceTransform0;
layout(location = 8)  in vec4 a_InstanceTransform1;
layout(location = 9)  in vec4 a_InstanceTransform2;
layout(location = 10) in vec4 a_InstanceTransform3;
layout(location = 11) in int  a_InstanceEntityID;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;
uniform int u_UseInstancing;

out vec3 v_WorldPos;
out vec2 v_TexCoord;
out vec3 v_Color;
flat out float v_PaletteIndex;
out vec3 v_Normal;
flat out int v_EntityID;

void main()
{
    mat4 transform;
    if (u_UseInstancing == 1)
    {
        transform = mat4(a_InstanceTransform0, a_InstanceTransform1,
                         a_InstanceTransform2, a_InstanceTransform3);
        v_EntityID = a_InstanceEntityID;
    }
    else
    {
        transform = u_Transform;
        v_EntityID = -1;
    }

    vec4 worldPos = transform * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    v_PaletteIndex = a_PaletteIndex;

    mat3 normalMatrix = transpose(inverse(mat3(transform)));
    v_Normal = normalize(normalMatrix * a_Normal);

    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

in vec3 v_WorldPos;
in vec2 v_TexCoord;
in vec3 v_Color;
flat in float v_PaletteIndex;
in vec3 v_Normal;
flat in int v_EntityID;

// Textures
uniform sampler2D u_AlbedoMap;
uniform int u_UseAlbedoMap;

// Vertex color / palette modes
uniform int u_UseVertexColor;
uniform int u_UsePalette;
uniform sampler2D u_PaletteColorMap;
uniform sampler2D u_PaletteMaterialMap;

// Material properties
uniform vec3  u_Albedo;

// Lighting
uniform vec3  u_LightDirection;
uniform vec3  u_LightColor;
uniform float u_LightIntensity;
uniform vec3  u_AmbientColor;

// Point lights
uniform int u_NumPointLights;
uniform vec3 u_PointLightPositions[32];
uniform vec3 u_PointLightColors[32];
uniform float u_PointLightRanges[32];

// Spot lights
uniform int u_NumSpotLights;
uniform vec3 u_SpotLightPositions[32];
uniform vec3 u_SpotLightDirections[32];
uniform vec3 u_SpotLightColors[32];
uniform float u_SpotLightRanges[32];
uniform float u_SpotLightInnerCos[32];
uniform float u_SpotLightOuterCos[32];

// Shadow (accepted but unused for flat)
uniform sampler2DShadow u_ShadowMap;
uniform int u_ShadowsEnabled;
uniform mat4 u_LightSpaceMatrix;

// Unused uniforms (accepted to avoid GL warnings)
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform int u_UseNormalMap;
uniform int u_UseMetallicMap;
uniform int u_UseRoughnessMap;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_SmoothAmount;
uniform float u_Opacity;
uniform vec3  u_CameraPosition;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int o_EntityID;

// Entity picking
uniform int u_EntityID;
uniform int u_UseInstancing;

vec3 LinearToSRGB(vec3 c)
{
    return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

float DistanceAttenuation(float dist, float range)
{
    float ratio = dist / max(range, 0.001);
    float f = clamp(1.0 - ratio * ratio, 0.0, 1.0);
    return f * f;
}

void main()
{
    // --- Material sampling ---
    vec3 albedo;

    if (u_UsePalette == 1)
    {
        float palU = (v_PaletteIndex + 0.5) / 256.0;
        vec4 palColor = texture(u_PaletteColorMap, vec2(palU, 0.5));
        albedo = palColor.rgb * v_Color;
    }
    else if (u_UseVertexColor == 1)
    {
        albedo = v_Color * u_Albedo;
    }
    else
    {
        albedo = (u_UseAlbedoMap == 1) ? texture(u_AlbedoMap, v_TexCoord).rgb : u_Albedo;
        albedo *= v_Color;
    }

    // Use flat normal from derivatives for consistent flat look
    vec3 N = normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)));
    vec3 L = normalize(-u_LightDirection);
    float NdotL = max(dot(N, L), 0.0);

    // Simple Lambert + ambient
    vec3 ambient = u_AmbientColor * albedo;
    vec3 diffuse = albedo * u_LightColor * u_LightIntensity * NdotL;

    vec3 color = ambient + diffuse;

    // Point lights (simple diffuse)
    for (int i = 0; i < u_NumPointLights; i++)
    {
        vec3 lightVec = u_PointLightPositions[i] - v_WorldPos;
        float dist = length(lightVec);
        vec3 pL = lightVec / max(dist, 0.001);
        float pNdotL = max(dot(N, pL), 0.0);
        float atten = DistanceAttenuation(dist, u_PointLightRanges[i]);
        color += albedo * u_PointLightColors[i] * atten * pNdotL;
    }

    // Spot lights (simple diffuse)
    for (int i = 0; i < u_NumSpotLights; i++)
    {
        vec3 lightVec = u_SpotLightPositions[i] - v_WorldPos;
        float dist = length(lightVec);
        vec3 sL = lightVec / max(dist, 0.001);
        float sNdotL = max(dot(N, sL), 0.0);
        float atten = DistanceAttenuation(dist, u_SpotLightRanges[i]);
        float theta = dot(-sL, normalize(u_SpotLightDirections[i]));
        float spotFalloff = smoothstep(u_SpotLightOuterCos[i], u_SpotLightInnerCos[i], theta);
        color += albedo * u_SpotLightColors[i] * atten * spotFalloff * sNdotL;
    }

    color *= 1.2;
    color = ACESFilm(color);
    color = LinearToSRGB(color);

    FragColor = vec4(color, 1.0);
    o_EntityID = (u_UseInstancing == 1) ? v_EntityID : u_EntityID;
}
