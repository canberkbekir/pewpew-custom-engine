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

	// Instancing
	inline constexpr auto UseInstancing = "u_UseInstancing";
}