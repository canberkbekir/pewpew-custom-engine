#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"

#include <array>
#include <vector>
#include <cstdint>

namespace CB
{
	class BinaryWriter;
	class BinaryReader;

	struct VoxelPaletteEntry
	{
		Vector3 Color = Vector3(1.0f);
		float Metallic = 0.0f;
		float Roughness = 0.5f;
		float Emission = 0.0f;
	};

	class VoxelPalette
	{
	public:
		static constexpr uint32_t MaxEntries = 256;

		VoxelPalette();

		uint8_t FindClosestEntry(const Vector3& color) const;
		uint8_t AddEntry(const VoxelPaletteEntry& entry);

		const VoxelPaletteEntry& GetEntry(uint8_t index) const { return m_Entries[index]; }
		void SetEntry(uint8_t index,const VoxelPaletteEntry& entry);

		uint32_t GetUsedCount() const { return m_UsedCount; }

		void GenerateColorTextureData(std::vector<uint8_t>& outRGBA) const;
		void GenerateMaterialTextureData(std::vector<uint8_t>& outRGBA) const;

		void Serialize(BinaryWriter& writer) const;
		void Deserialize(BinaryReader& reader);
	private:
		std::array<VoxelPaletteEntry, MaxEntries> m_Entries;
		uint32_t m_UsedCount = 1; // entry 0 = default white
	};
}