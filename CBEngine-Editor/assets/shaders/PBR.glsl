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
uniform mat4 u_LightSpaceMatrix;

out vec3 v_WorldPos;
out vec2 v_TexCoord;
out vec3 v_Color;
flat out float v_PaletteIndex;
out mat3 v_TBN;
flat out int v_EntityID;
out vec4 v_LightSpacePos;

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
        v_EntityID = -1; // Will use u_EntityID in fragment shader
    }

    vec4 worldPos = transform * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    v_PaletteIndex = a_PaletteIndex;

    // TBN matrix for normal mapping
    mat3 normalMatrix = transpose(inverse(mat3(transform)));
    vec3 N = normalize(normalMatrix * a_Normal);
    vec3 T = normalize(normalMatrix * a_Tangent);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T));
    v_TBN = mat3(T, B, N);

    v_LightSpacePos = u_LightSpaceMatrix * worldPos;
    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

const float PI = 3.14159265359;

in vec3 v_WorldPos;
in vec2 v_TexCoord;
in vec3 v_Color;
flat in float v_PaletteIndex;
in mat3 v_TBN;
flat in int v_EntityID;
in vec4 v_LightSpacePos;

// Textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;

uniform int u_UseAlbedoMap;
uniform int u_UseNormalMap;
uniform int u_UseMetallicMap;
uniform int u_UseRoughnessMap;

// When 1, vertex colors are the albedo (skip texture maps)
uniform int u_UseVertexColor;

// Palette mode (voxel palette coloring)
uniform int u_UsePalette;
uniform sampler2D u_PaletteColorMap;
uniform sampler2D u_PaletteMaterialMap;

// Material properties
uniform vec3  u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;

// Lighting
uniform vec3  u_LightDirection;
uniform vec3  u_LightColor;
uniform float u_LightIntensity;
uniform vec3  u_CameraPosition;
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

// Shadow mapping
uniform sampler2DShadow u_ShadowMap;
uniform int u_ShadowsEnabled;

// Shading controls
uniform float u_SmoothAmount;
uniform float u_Opacity;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int o_EntityID;

// Entity picking
uniform int u_EntityID;
uniform int u_UseInstancing;

// ---------- Color helpers ----------
vec3 LinearToSRGB(vec3 c)
{
    return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

// ---------- PBR helpers ----------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 1e-6);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float SpecularAA(vec3 N, float roughness)
{
    vec3 dndx = dFdx(N);
    vec3 dndy = dFdy(N);
    float variance = max(dot(dndx, dndx) + dot(dndy, dndy), 0.0);
    float r2 = roughness * roughness;
    r2 = clamp(r2 + 0.5 * variance, 0.0, 1.0);
    return sqrt(r2);
}

float DistanceAttenuation(float dist, float range)
{
    float ratio = dist / max(range, 0.001);
    float f = clamp(1.0 - ratio * ratio, 0.0, 1.0);
    return f * f;
}

vec3 CalcPointLight(int i, vec3 N, vec3 V, vec3 worldPos, vec3 F0, vec3 albedo, float metallic, float roughAA)
{
    vec3 lightVec = u_PointLightPositions[i] - worldPos;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 0.001);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float atten = DistanceAttenuation(dist, u_PointLightRanges[i]);

    vec3 radiance = u_PointLightColors[i] * atten;

    float NDF = DistributionGGX(N, H, roughAA);
    float G = GeometrySmith(N, V, L, roughAA);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalcSpotLight(int i, vec3 N, vec3 V, vec3 worldPos, vec3 F0, vec3 albedo, float metallic, float roughAA)
{
    vec3 lightVec = u_SpotLightPositions[i] - worldPos;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 0.001);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float atten = DistanceAttenuation(dist, u_SpotLightRanges[i]);

    // Angular falloff
    float theta = dot(-L, normalize(u_SpotLightDirections[i]));
    float spotFalloff = smoothstep(u_SpotLightOuterCos[i], u_SpotLightInnerCos[i], theta);

    vec3 radiance = u_SpotLightColors[i] * atten * spotFalloff;

    float NDF = DistributionGGX(N, H, roughAA);
    float G = GeometrySmith(N, V, L, roughAA);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float CalcShadow(vec4 lsPos, vec3 N, vec3 L)
{
    vec3 proj = lsPos.xyz / lsPos.w;
    proj = proj * 0.5 + 0.5;

    // Outside shadow map bounds
    if (proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0 || proj.z > 1.0)
        return 0.0;

    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);
    float depthRef = proj.z - bias;

    // 3x3 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(u_ShadowMap, vec3(proj.xy + vec2(x, y) * texelSize, depthRef));
        }
    }
    return 1.0 - (shadow / 9.0);
}

void main()
{
    vec2 uv = v_TexCoord;

    // --- Material sampling ---
    vec3 albedo;
    float metallic;
    float roughness;
    float emission = 0.0;

    if (u_UsePalette == 1)
    {
        // Palette mode: lookup color and material from 256x1 textures
        float palU = (v_PaletteIndex + 0.5) / 256.0;
        vec4 palColor = texture(u_PaletteColorMap, vec2(palU, 0.5));
        vec4 palMat = texture(u_PaletteMaterialMap, vec2(palU, 0.5));
        albedo = palColor.rgb * v_Color;
        metallic = palMat.r;
        roughness = palMat.g;
        emission = palMat.b;
    }
    else if (u_UseVertexColor == 1)
    {
        // Vertex colors are the albedo (voxel meshes with baked colors)
        // Skip texture maps since UVs don't match
        albedo = v_Color * u_Albedo;
        metallic = u_Metallic;
        roughness = u_Roughness;
    }
    else
    {
        albedo = (u_UseAlbedoMap == 1) ? texture(u_AlbedoMap, uv).rgb : u_Albedo;
        albedo *= v_Color;
        metallic = (u_UseMetallicMap == 1) ? texture(u_MetallicMap, uv).r : u_Metallic;
        roughness = (u_UseRoughnessMap == 1) ? texture(u_RoughnessMap, uv).r : u_Roughness;
    }

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.02, 1.0);

    // --- Normal calculation ---
    vec3 flatNormal = normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)));
    vec3 smoothNormal = normalize(v_TBN[2]);

    if (u_UseNormalMap == 1 && u_UseVertexColor == 0)
    {
        vec3 ns = texture(u_NormalMap, uv).rgb;
        if (length(ns) > 0.01)
        {
            vec3 nTS = ns * 2.0 - 1.0;
            smoothNormal = normalize(v_TBN * nTS);
        }
    }

    vec3 N = normalize(mix(flatNormal, smoothNormal, u_SmoothAmount));

    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 L = normalize(-u_LightDirection);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // --- PBR Lighting ---
    float roughAA = SpecularAA(N, roughness);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float wrap = 0.25;
    float NdotL_wrap = clamp((NdotL + wrap) / (1.0 + wrap), 0.0, 1.0);
    vec3 radiance = u_LightColor * u_LightIntensity;

    float NDF = DistributionGGX(N, H, roughAA);
    float G = GeometrySmith(N, V, L, roughAA);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 direct = (kD * albedo / PI + specular) * radiance * NdotL_wrap;

    // Apply shadow to directional light
    if (u_ShadowsEnabled == 1)
    {
        float shadow = CalcShadow(v_LightSpacePos, N, L);
        direct *= (1.0 - shadow);
    }

    // Point lights
    for (int i = 0; i < u_NumPointLights; i++)
        direct += CalcPointLight(i, N, V, v_WorldPos, F0, albedo, metallic, roughAA);

    // Spot lights
    for (int i = 0; i < u_NumSpotLights; i++)
        direct += CalcSpotLight(i, N, V, v_WorldPos, F0, albedo, metallic, roughAA);

    vec3 sky = max(u_AmbientColor, vec3(0.02));
    vec3 ground = sky * 0.35;
    float hemi = N.y * 0.5 + 0.5;
    vec3 ambientLight = mix(ground, sky, hemi);
    float fres = pow(1.0 - NdotV, 5.0);
    vec3 ambientSpec = (F0 * (0.15 + 0.85 * fres)) * (1.0 - roughAA);
    vec3 ambient = ambientLight * albedo * (0.6 + 0.4 * (1.0 - metallic)) + ambientSpec * ambientLight;

    vec3 color = ambient + direct;

    // Emission contribution (from palette)
    color += albedo * emission;

    color *= 1.2;
    color = ACESFilm(color);
    color = LinearToSRGB(color);

    FragColor = vec4(color, 1.0);
    o_EntityID = (u_UseInstancing == 1) ? v_EntityID : u_EntityID;
}
