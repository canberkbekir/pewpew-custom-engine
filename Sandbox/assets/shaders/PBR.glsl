#type vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in vec3 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

// Shader mode: 0=Normal, 1=Stylized, 2=Voxel, 3=Pixel
uniform int u_ShaderMode;
uniform float u_VoxelSize;

out vec3 v_WorldPos;
out vec3 v_LocalPos;
out vec2 v_TexCoord;
out vec3 v_Color;
out mat3 v_TBN;

void main()
{
    vec3 localPos = a_Position;

    // Voxel mode: snap vertices to grid
    if (u_ShaderMode == 2)
    {
        float gridSize = max(u_VoxelSize, 0.01);
        localPos = floor(localPos / gridSize + 0.5) * gridSize;
    }

    vec4 worldPos = u_Transform * vec4(localPos, 1.0);
    v_WorldPos = worldPos.xyz;
    v_LocalPos = localPos;
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;

    // TBN matrix for normal mapping
    mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));
    vec3 N = normalize(normalMatrix * a_Normal);
    vec3 T = normalize(normalMatrix * a_Tangent);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T));
    v_TBN = mat3(T, B, N);

    gl_Position = u_ViewProjection * worldPos;
}

#type fragment
#version 330 core

const float PI = 3.14159265359;

in vec3 v_WorldPos;
in vec3 v_LocalPos;
in vec2 v_TexCoord;
in vec3 v_Color;
in mat3 v_TBN;

// Textures
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;

uniform int u_UseAlbedoMap;
uniform int u_UseNormalMap;
uniform int u_UseMetallicMap;
uniform int u_UseRoughnessMap;

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

// Shading controls
uniform float u_SmoothAmount;
uniform float u_StylizedAmount;

// Shader mode: 0=Normal, 1=Stylized, 2=Voxel, 3=Pixel
uniform int u_ShaderMode;
uniform float u_PixelSize;
uniform float u_VoxelSize;

out vec4 FragColor;

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

void main()
{
    // --- UV calculation (pixelate for pixel mode) ---
    vec2 uv = v_TexCoord;
    if (u_ShaderMode == 3) // Pixel mode
    {
        float pixelRes = max(u_PixelSize, 1.0);
        uv = floor(uv * pixelRes) / pixelRes;
    }

    // --- Material sampling ---
    vec3 albedo = (u_UseAlbedoMap == 1) ? texture(u_AlbedoMap, uv).rgb : u_Albedo;
    albedo *= v_Color;  // Apply vertex color
    float metallic = (u_UseMetallicMap == 1) ? texture(u_MetallicMap, uv).r : u_Metallic;
    float roughness = (u_UseRoughnessMap == 1) ? texture(u_RoughnessMap, uv).r : u_Roughness;

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.02, 1.0);

    // --- Normal calculation ---
    vec3 flatNormal = normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)));
    vec3 smoothNormal = normalize(v_TBN[2]);

    if (u_UseNormalMap == 1 && u_ShaderMode == 0) // Normal map only in PBR mode
    {
        vec3 ns = texture(u_NormalMap, uv).rgb;
        if (length(ns) > 0.01)
        {
            vec3 nTS = ns * 2.0 - 1.0;
            smoothNormal = normalize(v_TBN * nTS);
        }
    }

    vec3 N;
    if (u_ShaderMode == 2 || u_ShaderMode == 3) // Voxel or Pixel: always flat
    {
        N = flatNormal;
    }
    else
    {
        N = normalize(mix(flatNormal, smoothNormal, u_SmoothAmount));
    }

    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 L = normalize(-u_LightDirection);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 color;

    // ========== NORMAL PBR MODE ==========
    if (u_ShaderMode == 0)
    {
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

        vec3 sky = max(u_AmbientColor, vec3(0.02));
        vec3 ground = sky * 0.35;
        float hemi = N.y * 0.5 + 0.5;
        vec3 ambientLight = mix(ground, sky, hemi);
        float fres = pow(1.0 - NdotV, 5.0);
        vec3 ambientSpec = (F0 * (0.15 + 0.85 * fres)) * (1.0 - roughAA);
        vec3 ambient = ambientLight * albedo * (0.6 + 0.4 * (1.0 - metallic)) + ambientSpec * ambientLight;

        vec3 pbrColor = ambient + direct;

        // Blend with stylized
        float softShade = NdotL * 0.15 + 0.85;
        vec3 stylizedColor = albedo * softShade;
        color = mix(pbrColor, stylizedColor, u_StylizedAmount);

        float exposure = mix(1.2, 1.0, u_StylizedAmount);
        color *= exposure;
        vec3 tonemapped = ACESFilm(color);
        color = mix(tonemapped, clamp(color, 0.0, 1.0), u_StylizedAmount * 0.7);
        color = LinearToSRGB(color);
    }
    // ========== STYLIZED/TOON MODE ==========
    else if (u_ShaderMode == 1)
    {
        // Cel shading with color bands
        float shade = NdotL;

        // Create 3-4 discrete shading bands
        if (shade > 0.66) shade = 1.0;
        else if (shade > 0.33) shade = 0.66;
        else if (shade > 0.0) shade = 0.4;
        else shade = 0.25;

        vec3 lightCol = u_LightColor * u_LightIntensity;
        vec3 ambient = albedo * max(u_AmbientColor, vec3(0.1));
        vec3 diffuse = albedo * shade * lightCol;

        // Simple rim light
        float rim = 1.0 - NdotV;
        rim = smoothstep(0.6, 1.0, rim);
        vec3 rimColor = albedo * rim * 0.3 * lightCol;

        color = diffuse + ambient + rimColor;
        color = clamp(color, 0.0, 1.0);
    }
    // ========== VOXEL MODE ==========
    else if (u_ShaderMode == 2)
    {
        // Hard-edged voxel look with flat shading
        float shade = NdotL * 0.6 + 0.4;

        // Add subtle edge highlighting based on face direction
        vec3 absN = abs(N);
        float edgeFactor = max(absN.x, max(absN.y, absN.z));
        edgeFactor = pow(edgeFactor, 2.0);

        vec3 lightCol = u_LightColor * u_LightIntensity;
        vec3 ambient = albedo * max(u_AmbientColor, vec3(0.1));
        vec3 diffuse = albedo * shade * lightCol;

        color = diffuse + ambient * (1.0 - edgeFactor * 0.3);

        // Posterize colors slightly for more voxel feel
        color = floor(color * 8.0) / 8.0;
        color = clamp(color, 0.0, 1.0);
    }
    // ========== PIXEL/RETRO MODE ==========
    else if (u_ShaderMode == 3)
    {
        // Retro pixel art style
        float shade = NdotL * 0.5 + 0.5;

        // Quantize shading to fewer levels
        shade = floor(shade * 4.0) / 4.0;

        vec3 lightCol = u_LightColor * u_LightIntensity;
        vec3 diffuse = albedo * shade * lightCol;
        vec3 ambient = albedo * max(u_AmbientColor, vec3(0.1));

        color = diffuse + ambient;

        // Color quantization (reduce color palette)
        float colorLevels = 16.0;
        color = floor(color * colorLevels) / colorLevels;
        color = clamp(color, 0.0, 1.0);
    }

    FragColor = vec4(color, 1.0);
}
