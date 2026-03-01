#include "cbpch.h"
#include "WorldGenerator.h"
#include "WorldGrid.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"

#include <glm/gtc/noise.hpp>

namespace CB
{
	void WorldGenerator::Generate(WorldChunk& chunk, glm::ivec3 coord,
	                              const WorldGenConfig& config, WorldGrid& grid)
	{
		switch (config.GeneratorType)
		{
		case WorldGenConfig::Type::Flat:
			GenerateFlat(chunk, coord, config, grid);
			break;
		case WorldGenConfig::Type::Noise:
			GenerateNoise(chunk, coord, config, grid);
			break;
		default:
			break;
		}
	}

	// Helper to fill a cell with a substance
	static void FillCell(WorldCell& cell, uint16_t substIdx, uint8_t flags, uint8_t paletteStart, uint8_t paletteCount)
	{
		cell.SubstanceIndex = substIdx;
		cell.Flags = flags;
		cell.Health = 255;
		cell.Density = 128;
		if (paletteCount > 0) {
			// Simple deterministic variation based on position hash
			cell.PaletteIndex = paletteStart + (cell.Temperature % paletteCount);
		}
	}

	void WorldGenerator::GenerateFlat(WorldChunk& chunk, glm::ivec3 coord,
	                                   const WorldGenConfig& config, WorldGrid& grid)
	{
		auto surfaceIt = grid.SubstanceToIndex.find(config.SurfaceSubstance);
		auto fillIt = grid.SubstanceToIndex.find(config.FillSubstance);

		uint16_t surfaceIdx = (surfaceIt != grid.SubstanceToIndex.end()) ? surfaceIt->second : 0;
		uint16_t fillIdx = (fillIt != grid.SubstanceToIndex.end()) ? fillIt->second : 0;

		if (surfaceIdx == 0 && fillIdx == 0)
			return;

		// Get palette ranges
		auto getSurfaceInfo = [&](uint16_t idx) -> std::pair<uint8_t, uint8_t> {
			if (idx == 0 || idx >= grid.SubstanceLUT.size()) return {0, 0};
			const auto& id = grid.SubstanceLUT[idx];
			auto rangeIt = grid.SubstancePaletteRanges.find(id);
			if (rangeIt != grid.SubstancePaletteRanges.end())
				return {rangeIt->second.Start, rangeIt->second.Count};
			return {0, 0};
		};
		auto [surfPalStart, surfPalCount] = getSurfaceInfo(surfaceIdx);
		auto [fillPalStart, fillPalCount] = getSurfaceInfo(fillIdx);

		uint8_t surfFlags = CellFlag_Solid | CellFlag_Static;
		uint8_t fillFlags = CellFlag_Solid | CellFlag_Static;

		// Check substance cell categories
		if (surfaceIdx > 0 && surfaceIdx < grid.SubstanceLUT.size()) {
			const auto& props = SubstanceRegistry::Get(grid.SubstanceLUT[surfaceIdx]);
			if (props.CellCategory != 0) surfFlags = props.CellCategory | CellFlag_Static;
		}
		if (fillIdx > 0 && fillIdx < grid.SubstanceLUT.size()) {
			const auto& props = SubstanceRegistry::Get(grid.SubstanceLUT[fillIdx]);
			if (props.CellCategory != 0) fillFlags = props.CellCategory | CellFlag_Static;
		}

		int baseY = coord.y * CHUNK_SIZE;

		for (int y = 0; y < CHUNK_SIZE; y++) {
			int worldY = baseY + y;

			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int x = 0; x < CHUNK_SIZE; x++) {
					int idx = WorldChunk::Index(x, y, z);

					if (worldY < config.SeaLevel - 1) {
						// Below surface: fill substance
						if (fillIdx > 0) {
							WorldCell& cell = chunk.Cells[idx];
							cell.Temperature = static_cast<uint8_t>((x * 7 + z * 13 + y * 3) & 0xFF);
							FillCell(cell, fillIdx, fillFlags, fillPalStart, fillPalCount);
							chunk.FilledCount++;
						}
					} else if (worldY == config.SeaLevel - 1 || worldY == config.SeaLevel) {
						// Surface layer
						uint16_t idx2 = surfaceIdx > 0 ? surfaceIdx : fillIdx;
						uint8_t palS = surfaceIdx > 0 ? surfPalStart : fillPalStart;
						uint8_t palC = surfaceIdx > 0 ? surfPalCount : fillPalCount;
						uint8_t flg = surfaceIdx > 0 ? surfFlags : fillFlags;
						if (idx2 > 0) {
							WorldCell& cell = chunk.Cells[idx];
							cell.Temperature = static_cast<uint8_t>((x * 7 + z * 13 + y * 3) & 0xFF);
							FillCell(cell, idx2, flg, palS, palC);
							chunk.FilledCount++;
						}
					}
					// Above SeaLevel: leave as air
				}
			}
		}

		if (chunk.FilledCount > 0) {
			chunk.MeshDirty = true;
			chunk.PhysicsDirty = true;
			chunk.Generation = 1;
		}
	}

	void WorldGenerator::GenerateNoise(WorldChunk& chunk, glm::ivec3 coord,
	                                    const WorldGenConfig& config, WorldGrid& grid)
	{
		auto surfaceIt = grid.SubstanceToIndex.find(config.SurfaceSubstance);
		auto fillIt = grid.SubstanceToIndex.find(config.FillSubstance);

		uint16_t surfaceIdx = (surfaceIt != grid.SubstanceToIndex.end()) ? surfaceIt->second : 0;
		uint16_t fillIdx = (fillIt != grid.SubstanceToIndex.end()) ? fillIt->second : 0;

		if (surfaceIdx == 0 && fillIdx == 0)
			return;

		auto getSurfaceInfo = [&](uint16_t idx) -> std::pair<uint8_t, uint8_t> {
			if (idx == 0 || idx >= grid.SubstanceLUT.size()) return {0, 0};
			const auto& id = grid.SubstanceLUT[idx];
			auto rangeIt = grid.SubstancePaletteRanges.find(id);
			if (rangeIt != grid.SubstancePaletteRanges.end())
				return {rangeIt->second.Start, rangeIt->second.Count};
			return {0, 0};
		};
		auto [surfPalStart, surfPalCount] = getSurfaceInfo(surfaceIdx);
		auto [fillPalStart, fillPalCount] = getSurfaceInfo(fillIdx);

		uint8_t surfFlags = CellFlag_Solid | CellFlag_Static;
		uint8_t fillFlags = CellFlag_Solid | CellFlag_Static;

		if (surfaceIdx > 0 && surfaceIdx < grid.SubstanceLUT.size()) {
			const auto& props = SubstanceRegistry::Get(grid.SubstanceLUT[surfaceIdx]);
			if (props.CellCategory != 0) surfFlags = props.CellCategory | CellFlag_Static;
		}
		if (fillIdx > 0 && fillIdx < grid.SubstanceLUT.size()) {
			const auto& props = SubstanceRegistry::Get(grid.SubstanceLUT[fillIdx]);
			if (props.CellCategory != 0) fillFlags = props.CellCategory | CellFlag_Static;
		}

		int baseX = coord.x * CHUNK_SIZE;
		int baseY = coord.y * CHUNK_SIZE;
		int baseZ = coord.z * CHUNK_SIZE;

		// Seed offset from config
		float seedOffsetX = static_cast<float>(config.Seed) * 0.37f;
		float seedOffsetZ = static_cast<float>(config.Seed) * 0.73f;

		// Precompute height map for this chunk's XZ footprint
		int heightMap[CHUNK_SIZE][CHUNK_SIZE];
		for (int z = 0; z < CHUNK_SIZE; z++) {
			for (int x = 0; x < CHUNK_SIZE; x++) {
				float worldX = static_cast<float>(baseX + x) + seedOffsetX;
				float worldZ = static_cast<float>(baseZ + z) + seedOffsetZ;

				// Multi-octave fractal noise
				float height = 0.0f;
				float amplitude = config.NoiseAmplitude;
				float frequency = config.NoiseScale;
				for (int oct = 0; oct < config.NoiseOctaves; oct++) {
					height += glm::simplex(glm::vec2(worldX * frequency, worldZ * frequency)) * amplitude;
					amplitude *= 0.5f;
					frequency *= 2.0f;
				}
				heightMap[z][x] = config.SeaLevel + static_cast<int>(std::round(height));
			}
		}

		for (int y = 0; y < CHUNK_SIZE; y++) {
			int worldY = baseY + y;

			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int x = 0; x < CHUNK_SIZE; x++) {
					int surfaceHeight = heightMap[z][x];
					int idx = WorldChunk::Index(x, y, z);

					if (worldY > surfaceHeight)
						continue; // air

					if (worldY == surfaceHeight) {
						// Surface layer
						uint16_t sIdx = surfaceIdx > 0 ? surfaceIdx : fillIdx;
						uint8_t palS = surfaceIdx > 0 ? surfPalStart : fillPalStart;
						uint8_t palC = surfaceIdx > 0 ? surfPalCount : fillPalCount;
						uint8_t flg = surfaceIdx > 0 ? surfFlags : fillFlags;
						if (sIdx > 0) {
							WorldCell& cell = chunk.Cells[idx];
							cell.Temperature = static_cast<uint8_t>((x * 7 + z * 13 + y * 3) & 0xFF);
							FillCell(cell, sIdx, flg, palS, palC);
							chunk.FilledCount++;
						}
					} else {
						// Below surface: fill
						if (fillIdx > 0) {
							WorldCell& cell = chunk.Cells[idx];
							cell.Temperature = static_cast<uint8_t>((x * 7 + z * 13 + y * 3) & 0xFF);
							FillCell(cell, fillIdx, fillFlags, fillPalStart, fillPalCount);
							chunk.FilledCount++;
						}
					}
				}
			}
		}

		if (chunk.FilledCount > 0) {
			chunk.MeshDirty = true;
			chunk.PhysicsDirty = true;
			chunk.Generation = 1;
		}
	}
}
