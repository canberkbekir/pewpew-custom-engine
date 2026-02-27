#pragma once

#include "Asset.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Utils/VoxelMaterialType.h"
#include "CBEngine/Utils/VoxelPalette.h"
#include "CBEngine/Voxel/Destruction/SubstanceID.h"

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
		SubstanceID MaterialSubstance = "Stone";
	};

	class VoxelTextureAsset : public Asset
	{
	public:
		static constexpr uint32_t s_VtexVersion = 4;

		VoxelTextureAsset() = default;
		~VoxelTextureAsset() override = default;

		// Source vmesh reference
		UUID SourceVmeshUUID;

		// Grid dimensions (should match vmesh)
		glm::ivec3 GridSize = {0, 0, 0};
		uint64_t VoxelCount = 0;

		// Palette index -> substance ID mapping (covers the bulk of voxels)
		std::unordered_map<uint8_t, SubstanceID> PaletteMapping;

		// Sparse per-voxel overrides (filled voxel index -> substance ID)
		std::unordered_map<uint32_t, SubstanceID> VoxelOverrides;

		// Custom brushes (added by user, persisted in vtex)
		std::vector<CustomBrush> CustomBrushes;

		// Sparse per-voxel palette index overrides from painting
		// filledVoxelIndex -> new paletteIndex
		std::unordered_map<uint32_t, uint8_t> PaletteIndexOverrides;

		// PBR override flags (checkboxes in editor)
		bool HasMetallicOverrides = false;
		bool HasRoughnessOverrides = false;
		bool HasEmissionOverrides = false;
		bool HasAlbedoOverrides = false;

		// Per-palette PBR overrides (palette index -> value)
		std::unordered_map<uint8_t, float> MetallicOverrides; // 0.0-1.0
		std::unordered_map<uint8_t, float> RoughnessOverrides; // 0.0-1.0
		std::unordered_map<uint8_t, float> EmissionOverrides; // 0.0-1.0
		std::unordered_map<uint8_t, Vector3> AlbedoOverrides; // RGB

		// Apply PBR overrides to a palette, returns modified copy
		VoxelPalette ApplyOverrides(const VoxelPalette& basePalette) const;

		// Query: checks overrides first, then palette mapping
		SubstanceID GetSubstanceID(uint32_t filledVoxelIndex, uint8_t paletteIndex) const;

		// Legacy compatibility: returns VoxelMaterialType from string
		VoxelMaterialType GetMaterialType(uint32_t filledVoxelIndex, uint8_t paletteIndex) const;

		// Set per-voxel override
		void SetSubstanceID(uint32_t filledVoxelIndex, const SubstanceID& id);

		// Legacy
		void SetMaterialType(uint32_t filledVoxelIndex, VoxelMaterialType type);

		// Set palette-level mapping (affects all voxels with that palette index)
		void SetPaletteSubstance(uint8_t paletteIndex, const SubstanceID& id);

		// Legacy
		void SetPaletteType(uint8_t paletteIndex, VoxelMaterialType type);

		// Generate from an existing vmesh asset (auto-detects material types from palette)
		static Ref<VoxelTextureAsset> GenerateFromVmesh(const Ref<VoxelMeshAsset>& vmesh);

		// Regenerate palette mapping by sampling a texture via vmesh UVs
		void GenerateMappingFromTexture(const Ref<VoxelMeshAsset>& vmesh,
		                                const std::filesystem::path& texturePath);

		// Apply PBR properties from a material to all palette entries as overrides
		void GenerateFromMaterial(const Ref<class Material>& material,
		                          const Ref<class VoxelMeshAsset>& vmesh);

		// YAML Serialization (.vtex files)
		bool Save(const std::filesystem::path& filePath);
		static Ref<VoxelTextureAsset> Load(const std::filesystem::path& filePath);

		bool Reload() override;

		static AssetType GetStaticType() { return AssetType::VoxelTexture; }
	};
}
