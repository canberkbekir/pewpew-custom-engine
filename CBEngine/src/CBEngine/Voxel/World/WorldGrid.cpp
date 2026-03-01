#include "cbpch.h"
#include "WorldGrid.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "CBEngine/Voxel/Substance/SubstanceDefinition.h"

#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>
#include <filesystem>

namespace CB
{
	const WorldCell WorldGrid::s_Air = {};

	// =========================================================================
	// WorldChunk
	// =========================================================================
	WorldCell& WorldChunk::At(int x, int y, int z)
	{
		return Cells[Index(x, y, z)];
	}

	const WorldCell& WorldChunk::At(int x, int y, int z) const
	{
		return Cells[Index(x, y, z)];
	}

	void WorldChunk::Compress()
	{
		if (IsCompressed) return;
		CompressedData = WorldGrid::RLEEncode(Cells, CHUNK_VOLUME);
		std::memset(Cells, 0, sizeof(Cells));
		IsCompressed = true;
	}

	void WorldChunk::Decompress()
	{
		if (!IsCompressed) return;
		WorldGrid::RLEDecode(CompressedData, Cells, CHUNK_VOLUME);
		CompressedData.clear();
		CompressedData.shrink_to_fit();
		IsCompressed = false;
	}

	void WorldChunk::EnsureDecompressed()
	{
		if (IsCompressed)
			Decompress();
	}

	// =========================================================================
	// SimulationRegion
	// =========================================================================
	bool SimulationRegion::ChunkInRange(glm::ivec3 coord, float voxelSize, float radius) const
	{
		float chunkWorldSize = CHUNK_SIZE * voxelSize;
		glm::vec3 chunkCenter = glm::vec3(coord) * chunkWorldSize + glm::vec3(chunkWorldSize * 0.5f);
		float dist = glm::length(chunkCenter - Center);
		float chunkHalfDiag = chunkWorldSize * 0.866f; // sqrt(3)/2
		return dist - chunkHalfDiag <= radius;
	}

	// =========================================================================
	// Coordinate math
	// =========================================================================
	glm::ivec3 WorldGrid::WorldToVoxel(glm::vec3 pos) const
	{
		return glm::ivec3(
			static_cast<int>(std::floor(pos.x / VoxelSize)),
			static_cast<int>(std::floor(pos.y / VoxelSize)),
			static_cast<int>(std::floor(pos.z / VoxelSize))
		);
	}

	glm::vec3 WorldGrid::VoxelToWorld(glm::ivec3 coord) const
	{
		return glm::vec3(coord) * VoxelSize + glm::vec3(VoxelSize * 0.5f);
	}

	glm::ivec3 WorldGrid::VoxelToChunkCoord(glm::ivec3 voxel)
	{
		return glm::ivec3(
			voxel.x >= 0 ? voxel.x / CHUNK_SIZE : (voxel.x - CHUNK_SIZE + 1) / CHUNK_SIZE,
			voxel.y >= 0 ? voxel.y / CHUNK_SIZE : (voxel.y - CHUNK_SIZE + 1) / CHUNK_SIZE,
			voxel.z >= 0 ? voxel.z / CHUNK_SIZE : (voxel.z - CHUNK_SIZE + 1) / CHUNK_SIZE
		);
	}

	glm::ivec3 WorldGrid::VoxelToLocal(glm::ivec3 voxel)
	{
		return glm::ivec3(
			((voxel.x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE,
			((voxel.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE,
			((voxel.z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE
		);
	}

	// =========================================================================
	// Cell access
	// =========================================================================
	const WorldCell& WorldGrid::GetCell(glm::ivec3 worldVoxel) const
	{
		glm::ivec3 cc = VoxelToChunkCoord(worldVoxel);
		const WorldChunk* chunk = GetChunk(cc);
		if (!chunk)
			return s_Air;

		glm::ivec3 local = VoxelToLocal(worldVoxel);
		return chunk->At(local.x, local.y, local.z);
	}

	void WorldGrid::SetCell(glm::ivec3 worldVoxel, const WorldCell& cell)
	{
		glm::ivec3 cc = VoxelToChunkCoord(worldVoxel);
		WorldChunk& chunk = GetOrCreateChunk(cc);
		chunk.EnsureDecompressed();
		glm::ivec3 local = VoxelToLocal(worldVoxel);

		WorldCell& existing = chunk.At(local.x, local.y, local.z);
		bool wasAir = existing.IsAir();
		bool isAir = cell.IsAir();

		existing = cell;

		if (wasAir && !isAir)
			chunk.FilledCount++;
		else if (!wasAir && isAir)
			chunk.FilledCount--;

		chunk.Generation++;
		chunk.MeshDirty = true;
		chunk.PhysicsDirty = true;
	}

	void WorldGrid::SetSubstance(glm::ivec3 worldVoxel, uint16_t substIdx)
	{
		if (substIdx == 0 || substIdx >= SubstanceLUT.size()) {
			ClearCell(worldVoxel);
			return;
		}

		WorldCell cell;
		cell.SubstanceIndex = substIdx;
		cell.Health = 255;

		const SubstanceID& id = SubstanceLUT[substIdx];
		const auto& props = SubstanceRegistry::Get(id);

		cell.Flags = props.CellCategory != 0 ? props.CellCategory : CellFlag_Solid;

		auto rangeIt = SubstancePaletteRanges.find(id);
		if (rangeIt != SubstancePaletteRanges.end() && rangeIt->second.Count > 0) {
			static thread_local std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<int> dist(0, rangeIt->second.Count - 1);
			cell.PaletteIndex = rangeIt->second.Start + static_cast<uint8_t>(dist(rng));
		}

		cell.Density = 128;

		SetCell(worldVoxel, cell);
	}

	void WorldGrid::ClearCell(glm::ivec3 worldVoxel)
	{
		glm::ivec3 cc = VoxelToChunkCoord(worldVoxel);
		WorldChunk* chunk = GetChunk(cc);
		if (!chunk)
			return;

		chunk->EnsureDecompressed();
		glm::ivec3 local = VoxelToLocal(worldVoxel);
		WorldCell& existing = chunk->At(local.x, local.y, local.z);

		if (existing.IsAir())
			return;

		existing = WorldCell{};
		chunk->FilledCount--;
		chunk->Generation++;
		chunk->MeshDirty = true;
		chunk->PhysicsDirty = true;
	}

	// =========================================================================
	// Bulk operations
	// =========================================================================
	void WorldGrid::FillBox(glm::ivec3 min, glm::ivec3 max, uint16_t substIdx)
	{
		for (int z = min.z; z <= max.z; z++)
			for (int y = min.y; y <= max.y; y++)
				for (int x = min.x; x <= max.x; x++)
					SetSubstance({ x, y, z }, substIdx);
	}

	void WorldGrid::FillSphere(glm::vec3 center, float radius, uint16_t substIdx)
	{
		float r2 = radius * radius;
		glm::ivec3 min = WorldToVoxel(center - glm::vec3(radius));
		glm::ivec3 max = WorldToVoxel(center + glm::vec3(radius));

		for (int z = min.z; z <= max.z; z++)
			for (int y = min.y; y <= max.y; y++)
				for (int x = min.x; x <= max.x; x++) {
					glm::vec3 voxCenter = VoxelToWorld({ x, y, z });
					if (glm::dot(voxCenter - center, voxCenter - center) <= r2)
						SetSubstance({ x, y, z }, substIdx);
				}
	}

	// =========================================================================
	// Chunk access
	// =========================================================================
	WorldChunk* WorldGrid::GetChunk(glm::ivec3 chunkCoord)
	{
		auto it = m_Chunks.find(chunkCoord);
		return it != m_Chunks.end() ? it->second.get() : nullptr;
	}

	const WorldChunk* WorldGrid::GetChunk(glm::ivec3 chunkCoord) const
	{
		auto it = m_Chunks.find(chunkCoord);
		return it != m_Chunks.end() ? it->second.get() : nullptr;
	}

	WorldChunk& WorldGrid::GetOrCreateChunk(glm::ivec3 chunkCoord)
	{
		auto it = m_Chunks.find(chunkCoord);
		if (it != m_Chunks.end())
			return *it->second;

		auto chunk = std::make_unique<WorldChunk>();
		chunk->Coord = chunkCoord;
		std::memset(chunk->Cells, 0, sizeof(chunk->Cells));
		WorldChunk& ref = *chunk;
		m_Chunks[chunkCoord] = std::move(chunk);
		return ref;
	}

	void WorldGrid::RemoveEmptyChunks()
	{
		for (auto it = m_Chunks.begin(); it != m_Chunks.end(); ) {
			if (it->second->FilledCount == 0)
				it = m_Chunks.erase(it);
			else
				++it;
		}
	}

	// =========================================================================
	// Substance LUT
	// =========================================================================
	void WorldGrid::BuildSubstanceLUT()
	{
		SubstanceLUT.clear();
		SubstanceToIndex.clear();

		SubstanceLUT.push_back(""); // Index 0 = air

		auto allIDs = SubstanceRegistry::GetAllIDs();
		for (const auto& id : allIDs) {
			uint16_t idx = static_cast<uint16_t>(SubstanceLUT.size());
			SubstanceLUT.push_back(id);
			SubstanceToIndex[id] = idx;
		}
	}

	// =========================================================================
	// Global palette
	// =========================================================================
	void WorldGrid::BuildGlobalPalette()
	{
		SubstancePaletteRanges.clear();

		auto allIDs = SubstanceRegistry::GetAllIDs();
		if (allIDs.empty())
			return;

		int slotsPerSubstance = std::max(1, 255 / static_cast<int>(allIDs.size()));
		slotsPerSubstance = std::min(slotsPerSubstance, 8);

		std::vector<uint8_t> colorData(256 * 4, 0);
		std::vector<uint8_t> materialData(256 * 4, 0);

		uint8_t currentSlot = 1;

		for (const auto& id : allIDs) {
			if (currentSlot >= 255)
				break;

			uint8_t count = static_cast<uint8_t>(std::min(slotsPerSubstance, 256 - static_cast<int>(currentSlot)));
			SubstancePaletteRanges[id] = { currentSlot, count };

			Vector3 baseColor = SubstanceRegistry::GetDisplayColor(id);
			const auto& def = SubstanceRegistry::GetDefinition(id);

			for (uint8_t i = 0; i < count; i++) {
				uint8_t slot = currentSlot + i;
				float brightness = 0.85f + 0.15f * (static_cast<float>(i) / std::max(1.0f, static_cast<float>(count - 1)));
				Vector3 color = baseColor * brightness;

				int offset = slot * 4;
				colorData[offset + 0] = static_cast<uint8_t>(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
				colorData[offset + 1] = static_cast<uint8_t>(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
				colorData[offset + 2] = static_cast<uint8_t>(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
				colorData[offset + 3] = 255;

				materialData[offset + 0] = static_cast<uint8_t>(std::clamp(def.PBRDefaults.Metallic, 0.0f, 1.0f) * 255.0f);
				materialData[offset + 1] = static_cast<uint8_t>(std::clamp(def.PBRDefaults.Roughness, 0.0f, 1.0f) * 255.0f);
				materialData[offset + 2] = static_cast<uint8_t>(std::clamp(def.PBRDefaults.Emission, 0.0f, 1.0f) * 255.0f);
				materialData[offset + 3] = 255;
			}

			currentSlot += count;
		}

		GlobalPaletteColor = Texture2D::CreateFromData(256, 1, colorData.data(), true);
		GlobalPaletteMaterial = Texture2D::CreateFromData(256, 1, materialData.data(), true);
	}

	// =========================================================================
	// RLE helpers — shared by compression, save/load, streaming
	// =========================================================================
	std::vector<uint8_t> WorldGrid::RLEEncode(const WorldCell* cells, int count)
	{
		std::vector<uint8_t> rleData;
		int i = 0;
		while (i < count) {
			const WorldCell& current = cells[i];
			uint16_t runLen = 1;
			while (i + runLen < count && runLen < 65535) {
				if (std::memcmp(&current, &cells[i + runLen], sizeof(WorldCell)) != 0)
					break;
				runLen++;
			}

			size_t off = rleData.size();
			rleData.resize(off + 2 + sizeof(WorldCell));
			std::memcpy(rleData.data() + off, &runLen, 2);
			std::memcpy(rleData.data() + off + 2, &current, sizeof(WorldCell));

			i += runLen;
		}
		return rleData;
	}

	void WorldGrid::RLEDecode(const std::vector<uint8_t>& data, WorldCell* cells, int count, uint16_t* outFilledCount)
	{
		size_t offset = 0;
		int cellIdx = 0;
		uint16_t filled = 0;

		while (offset + 2 + sizeof(WorldCell) <= data.size() && cellIdx < count) {
			uint16_t runLen;
			std::memcpy(&runLen, data.data() + offset, 2);
			WorldCell cell;
			std::memcpy(&cell, data.data() + offset + 2, sizeof(WorldCell));
			offset += 2 + sizeof(WorldCell);

			for (uint16_t r = 0; r < runLen && cellIdx < count; r++, cellIdx++) {
				cells[cellIdx] = cell;
				if (!cell.IsAir())
					filled++;
			}
		}

		if (outFilledCount)
			*outFilledCount = filled;
	}

	// =========================================================================
	// Metrics
	// =========================================================================
	float WorldGrid::GetMemoryUsageMB() const
	{
		size_t total = 0;
		for (const auto& [coord, chunk] : m_Chunks) {
			if (!chunk) continue;
			if (chunk->IsCompressed)
				total += chunk->CompressedData.size();
			else
				total += sizeof(WorldCell) * CHUNK_VOLUME;
			total += sizeof(WorldChunk) - sizeof(WorldCell) * CHUNK_VOLUME; // metadata
		}
		return static_cast<float>(total) / (1024.0f * 1024.0f);
	}

	// =========================================================================
	// Save — binary format v2: header + GenConfig + RLE-compressed chunks
	// =========================================================================
	static constexpr uint32_t CBWG_MAGIC = 0x43425747; // "CBWG"
	static constexpr uint32_t CBWG_VERSION_V1 = 1;
	static constexpr uint32_t CBWG_VERSION_V2 = 2;

	bool WorldGrid::Save(const std::string& path) const
	{
		// If streaming is enabled, flush all chunks to individual files
		if (HasStorage()) {
			// Save manifest only
			std::ofstream file(path, std::ios::binary);
			if (!file.is_open())
				return false;

			file.write(reinterpret_cast<const char*>(&CBWG_MAGIC), 4);
			file.write(reinterpret_cast<const char*>(&CBWG_VERSION_V2), 4);
			file.write(reinterpret_cast<const char*>(&VoxelSize), 4);

			// GenConfig
			uint8_t genType = static_cast<uint8_t>(GenConfig.GeneratorType);
			file.write(reinterpret_cast<const char*>(&genType), 1);
			file.write(reinterpret_cast<const char*>(&GenConfig.Seed), 4);
			file.write(reinterpret_cast<const char*>(&GenConfig.SeaLevel), 4);
			file.write(reinterpret_cast<const char*>(&GenConfig.NoiseScale), 4);
			file.write(reinterpret_cast<const char*>(&GenConfig.NoiseOctaves), 4);
			file.write(reinterpret_cast<const char*>(&GenConfig.NoiseAmplitude), 4);
			file.write(reinterpret_cast<const char*>(&StreamRadius), 4);
			file.write(reinterpret_cast<const char*>(&UnloadRadius), 4);

			// Substance strings
			auto writeString = [&](const std::string& s) {
				uint16_t len = static_cast<uint16_t>(s.size());
				file.write(reinterpret_cast<const char*>(&len), 2);
				file.write(s.data(), len);
			};
			writeString(GenConfig.SurfaceSubstance);
			writeString(GenConfig.FillSubstance);

			// Storage path (relative)
			writeString(m_StoragePath);

			// Flush chunks to individual files
			const_cast<WorldGrid*>(this)->FlushAll();

			return file.good();
		}

		// V1 monolithic format (no streaming)
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
			return false;

		file.write(reinterpret_cast<const char*>(&CBWG_MAGIC), 4);
		file.write(reinterpret_cast<const char*>(&CBWG_VERSION_V1), 4);
		file.write(reinterpret_cast<const char*>(&VoxelSize), 4);
		uint32_t chunkCount = static_cast<uint32_t>(m_Chunks.size());
		file.write(reinterpret_cast<const char*>(&chunkCount), 4);

		for (const auto& [coord, chunk] : m_Chunks) {
			if (!chunk)
				continue;

			file.write(reinterpret_cast<const char*>(&coord.x), 4);
			file.write(reinterpret_cast<const char*>(&coord.y), 4);
			file.write(reinterpret_cast<const char*>(&coord.z), 4);

			// If already compressed, write CompressedData directly
			std::vector<uint8_t> rleData;
			if (chunk->IsCompressed) {
				rleData = chunk->CompressedData;
			} else {
				rleData = RLEEncode(chunk->Cells, CHUNK_VOLUME);
			}

			uint32_t compressedSize = static_cast<uint32_t>(rleData.size());
			file.write(reinterpret_cast<const char*>(&compressedSize), 4);
			file.write(reinterpret_cast<const char*>(rleData.data()), compressedSize);
		}

		return file.good();
	}

	// =========================================================================
	// Load — supports v1 (monolithic) and v2 (streaming manifest)
	// =========================================================================
	bool WorldGrid::Load(const std::string& path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
			return false;

		uint32_t magic, version;
		file.read(reinterpret_cast<char*>(&magic), 4);
		file.read(reinterpret_cast<char*>(&version), 4);

		if (magic != CBWG_MAGIC)
			return false;

		if (version == CBWG_VERSION_V2) {
			// V2: manifest + per-chunk files
			file.read(reinterpret_cast<char*>(&VoxelSize), 4);

			uint8_t genType;
			file.read(reinterpret_cast<char*>(&genType), 1);
			GenConfig.GeneratorType = static_cast<WorldGenConfig::Type>(genType);
			file.read(reinterpret_cast<char*>(&GenConfig.Seed), 4);
			file.read(reinterpret_cast<char*>(&GenConfig.SeaLevel), 4);
			file.read(reinterpret_cast<char*>(&GenConfig.NoiseScale), 4);
			file.read(reinterpret_cast<char*>(&GenConfig.NoiseOctaves), 4);
			file.read(reinterpret_cast<char*>(&GenConfig.NoiseAmplitude), 4);
			file.read(reinterpret_cast<char*>(&StreamRadius), 4);
			file.read(reinterpret_cast<char*>(&UnloadRadius), 4);

			auto readString = [&]() -> std::string {
				uint16_t len;
				file.read(reinterpret_cast<char*>(&len), 2);
				std::string s(len, '\0');
				file.read(s.data(), len);
				return s;
			};
			GenConfig.SurfaceSubstance = readString();
			GenConfig.FillSubstance = readString();
			m_StoragePath = readString();

			m_Chunks.clear();
			// Chunks will be loaded on-demand via StreamUpdate
			return file.good();
		}

		if (version == CBWG_VERSION_V1) {
			// V1: all chunks inline
			file.read(reinterpret_cast<char*>(&VoxelSize), 4);
			uint32_t chunkCount;
			file.read(reinterpret_cast<char*>(&chunkCount), 4);

			m_Chunks.clear();

			for (uint32_t ci = 0; ci < chunkCount; ci++) {
				glm::ivec3 coord;
				file.read(reinterpret_cast<char*>(&coord.x), 4);
				file.read(reinterpret_cast<char*>(&coord.y), 4);
				file.read(reinterpret_cast<char*>(&coord.z), 4);

				uint32_t compressedSize;
				file.read(reinterpret_cast<char*>(&compressedSize), 4);

				std::vector<uint8_t> rleData(compressedSize);
				file.read(reinterpret_cast<char*>(rleData.data()), compressedSize);

				auto chunk = std::make_unique<WorldChunk>();
				chunk->Coord = coord;
				std::memset(chunk->Cells, 0, sizeof(chunk->Cells));

				uint16_t filled = 0;
				RLEDecode(rleData, chunk->Cells, CHUNK_VOLUME, &filled);
				chunk->FilledCount = filled;

				chunk->MeshDirty = true;
				chunk->PhysicsDirty = true;
				chunk->Generation = 1;
				m_Chunks[coord] = std::move(chunk);
			}

			return file.good();
		}

		return false; // unknown version
	}

	// =========================================================================
	// Streaming
	// =========================================================================
	void WorldGrid::SetStoragePath(const std::string& directory)
	{
		m_StoragePath = directory;
		if (!m_StoragePath.empty()) {
			std::filesystem::create_directories(m_StoragePath);
		}
	}

	std::string WorldGrid::ChunkFilePath(glm::ivec3 coord) const
	{
		return m_StoragePath + "/chunk_" +
			std::to_string(coord.x) + "_" +
			std::to_string(coord.y) + "_" +
			std::to_string(coord.z) + ".cbwc";
	}

	bool WorldGrid::ChunkFileExists(glm::ivec3 coord) const
	{
		return std::filesystem::exists(ChunkFilePath(coord));
	}

	std::vector<uint8_t> WorldGrid::ReadChunkFile(glm::ivec3 coord) const
	{
		std::string path = ChunkFilePath(coord);
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open())
			return {};

		auto size = file.tellg();
		file.seekg(0);
		std::vector<uint8_t> data(static_cast<size_t>(size));
		file.read(reinterpret_cast<char*>(data.data()), size);
		return data;
	}

	void WorldGrid::WriteChunkFile(glm::ivec3 coord, const std::vector<uint8_t>& rleData) const
	{
		std::string path = ChunkFilePath(coord);
		std::ofstream file(path, std::ios::binary);
		if (file.is_open()) {
			file.write(reinterpret_cast<const char*>(rleData.data()), rleData.size());
		}
	}

	void WorldGrid::RequestLoad(glm::ivec3 coord)
	{
		if (m_LoadingSet.count(coord))
			return;

		m_LoadingSet.insert(coord);

		// Capture what we need for async (no reference to this)
		std::string filePath = ChunkFilePath(coord);
		bool fileExists = ChunkFileExists(coord);
		WorldGenConfig genConfig = GenConfig;
		bool hasGenerator = (GenConfig.GeneratorType != WorldGenConfig::Type::None);

		// We need SubstanceToIndex for generation — copy it
		auto substanceToIndex = SubstanceToIndex;

		m_PendingLoads.push_back({ coord,
			std::async(std::launch::async, [coord, filePath, fileExists, genConfig, hasGenerator, substanceToIndex]() -> Scope<WorldChunk> {
				auto chunk = std::make_unique<WorldChunk>();
				chunk->Coord = coord;
				std::memset(chunk->Cells, 0, sizeof(chunk->Cells));

				if (fileExists) {
					// Load from file
					std::ifstream file(filePath, std::ios::binary | std::ios::ate);
					if (file.is_open()) {
						auto size = file.tellg();
						file.seekg(0);
						std::vector<uint8_t> rleData(static_cast<size_t>(size));
						file.read(reinterpret_cast<char*>(rleData.data()), size);

						uint16_t filled = 0;
						WorldGrid::RLEDecode(rleData, chunk->Cells, CHUNK_VOLUME, &filled);
						chunk->FilledCount = filled;
					}
				}
				// Note: generation is handled on main thread after load completes
				// to avoid threading issues with SubstanceRegistry

				chunk->MeshDirty = true;
				chunk->PhysicsDirty = true;
				chunk->Generation = 1;
				return chunk;
			})
		});
	}

	void WorldGrid::ProcessCompletedLoads()
	{
		static constexpr int MAX_INSERTS_PER_FRAME = 8;
		int inserted = 0;

		for (auto it = m_PendingLoads.begin(); it != m_PendingLoads.end() && inserted < MAX_INSERTS_PER_FRAME; ) {
			if (it->Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				auto chunk = it->Future.get();
				glm::ivec3 coord = it->Coord;
				m_LoadingSet.erase(coord);

				// If chunk is empty and we have a generator, generate content
				if (chunk->FilledCount == 0 && Generator) {
					Generator(*chunk, coord, *this);
				}

				if (chunk->FilledCount > 0 || Generator) {
					m_Chunks[coord] = std::move(chunk);
				}
				inserted++;
				it = m_PendingLoads.erase(it);
			} else {
				++it;
			}
		}
	}

	void WorldGrid::UnloadChunk(glm::ivec3 coord)
	{
		auto it = m_Chunks.find(coord);
		if (it == m_Chunks.end())
			return;

		WorldChunk* chunk = it->second.get();

		// Save dirty chunk to disk
		if (HasStorage() && chunk->Generation > 0) {
			std::vector<uint8_t> rleData;
			if (chunk->IsCompressed) {
				rleData = chunk->CompressedData;
			} else {
				rleData = RLEEncode(chunk->Cells, CHUNK_VOLUME);
			}
			WriteChunkFile(coord, rleData);
		}

		m_Chunks.erase(it);
	}

	void WorldGrid::StreamUpdate(const SimulationRegion& region)
	{
		if (m_StoragePath.empty() && GenConfig.GeneratorType == WorldGenConfig::Type::None)
			return;

		// 1. Process completed loads
		ProcessCompletedLoads();

		// 2. Determine needed chunk coords within StreamRadius
		float chunkWorldSize = CHUNK_SIZE * VoxelSize;
		int chunkRadius = static_cast<int>(std::ceil(StreamRadius / chunkWorldSize));
		glm::ivec3 center = VoxelToChunkCoord(WorldToVoxel(region.Center));

		// Cap concurrent pending loads to avoid launching thousands of async tasks at once
		static constexpr size_t MAX_PENDING_LOADS = 16;
		bool loadCapReached = false;

		for (int z = -chunkRadius; z <= chunkRadius && !loadCapReached; z++)
			for (int y = -chunkRadius; y <= chunkRadius && !loadCapReached; y++)
				for (int x = -chunkRadius; x <= chunkRadius && !loadCapReached; x++) {
					glm::ivec3 coord = center + glm::ivec3(x, y, z);
					if (!ChunkInBounds(coord))
						continue;
					if (!region.ChunkInRange(coord, VoxelSize, StreamRadius))
						continue;
					if (m_Chunks.count(coord) || m_LoadingSet.count(coord))
						continue;
					RequestLoad(coord);
					if (m_LoadingSet.size() >= MAX_PENDING_LOADS)
						loadCapReached = true;
				}

		// 3. Unload chunks beyond UnloadRadius
		std::vector<glm::ivec3> toUnload;
		for (auto& [coord, chunk] : m_Chunks) {
			if (!region.ChunkInRange(coord, VoxelSize, UnloadRadius))
				toUnload.push_back(coord);
		}
		for (auto& coord : toUnload)
			UnloadChunk(coord);
	}

	void WorldGrid::FlushAll()
	{
		if (m_StoragePath.empty())
			return;

		std::filesystem::create_directories(m_StoragePath);

		for (auto& [coord, chunk] : m_Chunks) {
			if (!chunk) continue;

			std::vector<uint8_t> rleData;
			if (chunk->IsCompressed) {
				rleData = chunk->CompressedData;
			} else {
				rleData = RLEEncode(chunk->Cells, CHUNK_VOLUME);
			}
			WriteChunkFile(coord, rleData);
		}
	}
}
