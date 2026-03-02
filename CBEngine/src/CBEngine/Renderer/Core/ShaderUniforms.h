#pragma once

namespace CB::ShaderUniforms
{
	// Transform uniforms
	inline constexpr auto ViewProjection = "u_ViewProjection";
	inline constexpr auto Transform = "u_Transform";
	inline constexpr auto CameraPosition = "u_CameraPosition";

	// Lighting uniforms
	inline constexpr auto LightDirection = "u_LightDirection";
	inline constexpr auto LightColor = "u_LightColor";
	inline constexpr auto LightIntensity = "u_LightIntensity";
	inline constexpr auto AmbientColor = "u_AmbientColor";

	// Material texture flags
	inline constexpr auto UseAlbedoMap = "u_UseAlbedoMap";
	inline constexpr auto UseNormalMap = "u_UseNormalMap";
	inline constexpr auto UseMetallicMap = "u_UseMetallicMap";
	inline constexpr auto UseRoughnessMap = "u_UseRoughnessMap";

	// Material texture samplers
	inline constexpr auto AlbedoMap = "u_AlbedoMap";
	inline constexpr auto NormalMap = "u_NormalMap";
	inline constexpr auto MetallicMap = "u_MetallicMap";
	inline constexpr auto RoughnessMap = "u_RoughnessMap";

	// Entity picking
	inline constexpr auto EntityID = "u_EntityID";

	// Vertex color mode (voxel meshes with baked colors)
	inline constexpr auto UseVertexColor = "u_UseVertexColor";

	// Palette mode (voxel palette coloring)
	inline constexpr auto UsePalette = "u_UsePalette";
	inline constexpr auto PaletteColorMap = "u_PaletteColorMap";
	inline constexpr auto PaletteMaterialMap = "u_PaletteMaterialMap";

	// Material properties
	inline constexpr auto Albedo = "u_Albedo";
	inline constexpr auto Metallic = "u_Metallic";
	inline constexpr auto Roughness = "u_Roughness";
	inline constexpr auto SmoothAmount = "u_SmoothAmount";
	inline constexpr auto Opacity = "u_Opacity";

	// Instancing
	inline constexpr auto UseInstancing = "u_UseInstancing";

	// Point lights
	inline constexpr auto NumPointLights = "u_NumPointLights";
	inline constexpr auto PointLightPositions = "u_PointLightPositions";
	inline constexpr auto PointLightColors = "u_PointLightColors";
	inline constexpr auto PointLightRanges = "u_PointLightRanges";

	// Spot lights
	inline constexpr auto NumSpotLights = "u_NumSpotLights";
	inline constexpr auto SpotLightPositions = "u_SpotLightPositions";
	inline constexpr auto SpotLightDirections = "u_SpotLightDirections";
	inline constexpr auto SpotLightColors = "u_SpotLightColors";
	inline constexpr auto SpotLightRanges = "u_SpotLightRanges";
	inline constexpr auto SpotLightInnerCos = "u_SpotLightInnerCos";
	inline constexpr auto SpotLightOuterCos = "u_SpotLightOuterCos";

	// Shadow mapping
	inline constexpr auto LightSpaceMatrix = "u_LightSpaceMatrix";
	inline constexpr auto ShadowMap = "u_ShadowMap";
	inline constexpr auto ShadowsEnabled = "u_ShadowsEnabled";
}