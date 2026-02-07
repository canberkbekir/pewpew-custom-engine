#pragma once

#include "Asset.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Utils/VoxelPalette.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace CB
{
	class VoxelMeshAsset;

	struct CustomBrush
	{
		VoxelPaletteEntry Entry;
		VoxelMaterialType MaterialType = VoxelMaterialType::Stone;
	};

	class VoxelTextureAsset : public Asset
	{
	public:
		static constexpr uint32_t s_VtexVersion = 2;

		VoxelTextureAsset() = default;
		~VoxelTextureAsset() override = default;

		// Source vmesh reference
		UUID SourceVmeshUUID;

		// Grid dimensions (should match vmesh)
		glm::ivec3 GridSize = { 0, 0, 0 };
		uint64_t VoxelCount = 0;

		// Palette index -> material type mapping (covers the bulk of voxels)
		std::unordered_map<uint8_t, VoxelMaterialType> PaletteMapping;

		// Sparse per-voxel overrides (filled voxel index -> material type)
		std::unordered_map<uint32_t, VoxelMaterialType> VoxelOverrides;

		// Custom brushes (added by user, persisted in vtex)
		std::vector<CustomBrush> CustomBrushes;

		// Sparse per-voxel palette index overrides from painting
		// filledVoxelIndex -> new paletteIndex
		std::unordered_map<uint32_t, uint8_t> PaletteIndexOverrides;

		// Query: checks overrides first, then palette mapping
		VoxelMaterialType GetMaterialType(uint32_t filledVoxelIndex, uint8_t paletteIndex) const;

		// Set per-voxel override
		void SetMaterialType(uint32_t filledVoxelIndex, VoxelMaterialType type);

		// Set palette-level mapping (affects all voxels with that palette index)
		void SetPaletteType(uint8_t paletteIndex, VoxelMaterialType type);

		// Generate from an existing vmesh asset (auto-detects material types from palette)
		static Ref<VoxelTextureAsset> GenerateFromVmesh(const Ref<VoxelMeshAsset>& vmesh);

		// Regenerate palette mapping by sampling a texture via vmesh UVs
		// Each palette entry's color is replaced by the average sampled color from the texture
		void GenerateMappingFromTexture(const Ref<VoxelMeshAsset>& vmesh,
		                                const std::filesystem::path& texturePath);

		// YAML Serialization (.vtex files)
		bool Save(const std::filesystem::path& filePath);
		static Ref<VoxelTextureAsset> Load(const std::filesystem::path& filePath);

		bool Reload() override;

		static AssetType GetStaticType() { return AssetType::VoxelTexture; }
	};
}
