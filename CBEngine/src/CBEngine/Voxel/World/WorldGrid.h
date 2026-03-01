#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Voxel/Substance/SubstanceID.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <functional>
#include <future>
#include <string>

namespace CB
{
	// =========================================================================
	// CellFlags — bitfield describing cell behavior category
	// =========================================================================
	enum CellFlags : uint8_t
	{
		CellFlag_None    = 0,
		CellFlag_Solid   = 1 << 0,
		CellFlag_Liquid  = 1 << 1,
		CellFlag_Gas     = 1 << 2,
		CellFlag_Powder  = 1 << 3,
		CellFlag_Burning = 1 << 4,
		CellFlag_Dirty   = 1 << 5,
		CellFlag_Static  = 1 << 6,  // Immovable (bedrock)
	};

	// =========================================================================
	// WorldCell — 8 bytes per voxel in the world grid
	// =========================================================================
	struct WorldCell
	{
		uint16_t SubstanceIndex = 0; // 0 = air. Index into WorldGrid::SubstanceLUT
		uint8_t  Flags = 0;          // CellFlag bitfield
		uint8_t  Health = 0;         // 0-255, normalized from substance max (0 = dead, 255 = full)
		uint8_t  Temperature = 0;    // 0-255 (10K per unit)
		uint8_t  PaletteIndex = 0;   // Slot in global palette (color + variation)
		uint8_t  Density = 0;        // Liquid level / gas concentration
		uint8_t  UpdateTick = 0;     // Last sim tick processed (prevents double-update)

		bool IsAir() const { return SubstanceIndex == 0; }
	};
	static_assert(sizeof(WorldCell) == 8, "WorldCell must be 8 bytes");

	// =========================================================================
	// WorldChunk — 32^3 cells = 256KB
	// =========================================================================
	static constexpr int CHUNK_SIZE = 32;
	static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; // 32768

	struct WorldChunk
	{
		WorldCell Cells[CHUNK_VOLUME]; // 256KB, inline
		glm::ivec3 Coord = { 0, 0, 0 };
		uint16_t FilledCount = 0;
		uint32_t Generation = 0;      // Bumped on any cell write. Triggers mesh/physics rebuild.
		bool MeshDirty = true;
		bool PhysicsDirty = true;
		uint32_t MeshGeneration = 0;  // Generation when mesh was last built
		Ref<Mesh> CachedMesh;         // nullptr until first build
		JPH::BodyID PhysicsBodyID;
		bool HasPhysicsBody = false;

		// LOD
		uint8_t CurrentLOD = 0;       // 0=full, 1=half, 2=quarter

		// Memory compression
		bool IsCompressed = false;
		uint32_t IdleFrames = 0;
		uint32_t IdleGeneration = 0;
		std::vector<uint8_t> CompressedData;

		void Compress();
		void Decompress();
		void EnsureDecompressed();

		WorldCell& At(int x, int y, int z);
		const WorldCell& At(int x, int y, int z) const;

		static int Index(int x, int y, int z)
		{
			return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
		}
	};

	// =========================================================================
	// SimulationRegion — defines active area around camera/player
	// =========================================================================
	struct SimulationRegion
	{
		glm::vec3 Center = { 0.0f, 0.0f, 0.0f };
		float SimRadius = 128.0f;     // CA simulation range (world units)
		float RenderRadius = 256.0f;  // Mesh rendering range
		float PhysicsRadius = 160.0f;
		float LOD1Distance = 160.0f;  // Beyond this: half-res mesh
		float LOD2Distance = 224.0f;  // Beyond this: quarter-res mesh

		bool ChunkInRange(glm::ivec3 coord, float voxelSize, float radius) const;
	};

	// =========================================================================
	// PaletteRange — contiguous range of palette slots for one substance
	// =========================================================================
	struct PaletteRange
	{
		uint8_t Start = 0;
		uint8_t Count = 0;
	};

	// =========================================================================
	// ChunkCoordHash — hash for glm::ivec3 used as chunk map key
	// =========================================================================
	struct ChunkCoordHash
	{
		size_t operator()(const glm::ivec3& v) const noexcept
		{
			size_t h = 0;
			h ^= std::hash<int>()(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	// =========================================================================
	// WorldGenConfig — procedural generation parameters
	// =========================================================================
	struct WorldGenConfig
	{
		enum class Type : uint8_t { None = 0, Flat = 1, Noise = 2 };
		Type GeneratorType = Type::None;
		uint32_t Seed = 42;
		int SeaLevel = 0;
		float NoiseScale = 0.02f;
		int NoiseOctaves = 4;
		float NoiseAmplitude = 32.0f;
		std::string SurfaceSubstance = "";
		std::string FillSubstance = "";
	};

	// =========================================================================
	// WorldGrid — chunked voxel world
	// =========================================================================
	class WorldGrid
	{
	public:
		float VoxelSize = 0.25f;
		SimulationRegion Region;

		// --- World bounds (chunk coordinates) ---
		// Limits chunk generation/streaming. Defaults to no bounds (max int).
		bool HasBounds = false;
		glm::ivec3 MinChunkCoord = glm::ivec3(0);
		glm::ivec3 MaxChunkCoord = glm::ivec3(0);

		bool ChunkInBounds(glm::ivec3 coord) const
		{
			if (!HasBounds) return true;
			return coord.x >= MinChunkCoord.x && coord.x <= MaxChunkCoord.x
				&& coord.y >= MinChunkCoord.y && coord.y <= MaxChunkCoord.y
				&& coord.z >= MinChunkCoord.z && coord.z <= MaxChunkCoord.z;
		}

		// Total expected chunks within bounds (for progress tracking)
		size_t GetExpectedChunkCount() const
		{
			if (!HasBounds) return 0;
			glm::ivec3 size = MaxChunkCoord - MinChunkCoord + glm::ivec3(1);
			return static_cast<size_t>(size.x) * size.y * size.z;
		}

		// --- Cell access ---
		const WorldCell& GetCell(glm::ivec3 worldVoxel) const;
		void SetCell(glm::ivec3 worldVoxel, const WorldCell& cell);
		void SetSubstance(glm::ivec3 worldVoxel, uint16_t substIdx);
		void ClearCell(glm::ivec3 worldVoxel);

		// --- Bulk operations ---
		void FillBox(glm::ivec3 min, glm::ivec3 max, uint16_t substIdx);
		void FillSphere(glm::vec3 center, float radius, uint16_t substIdx);

		// --- Coordinate math ---
		glm::ivec3 WorldToVoxel(glm::vec3 pos) const;
		glm::vec3 VoxelToWorld(glm::ivec3 coord) const;
		static glm::ivec3 VoxelToChunkCoord(glm::ivec3 voxel);
		static glm::ivec3 VoxelToLocal(glm::ivec3 voxel);

		// --- Chunk access ---
		WorldChunk* GetChunk(glm::ivec3 chunkCoord);
		const WorldChunk* GetChunk(glm::ivec3 chunkCoord) const;
		WorldChunk& GetOrCreateChunk(glm::ivec3 chunkCoord);
		void RemoveEmptyChunks();
		auto& Chunks() { return m_Chunks; }
		const auto& Chunks() const { return m_Chunks; }

		// --- Palette + substance ---
		void BuildGlobalPalette();
		void BuildSubstanceLUT();

		std::vector<SubstanceID> SubstanceLUT;
		std::unordered_map<SubstanceID, uint16_t> SubstanceToIndex;
		std::unordered_map<SubstanceID, PaletteRange> SubstancePaletteRanges;
		Ref<Texture2D> GlobalPaletteColor;
		Ref<Texture2D> GlobalPaletteMaterial;

		uint8_t CurrentTick = 0;

		// --- Serialization ---
		bool Save(const std::string& path) const;
		bool Load(const std::string& path);

		// --- Streaming ---
		void SetStoragePath(const std::string& directory);
		void StreamUpdate(const SimulationRegion& region);
		void FlushAll();
		bool HasStorage() const { return !m_StoragePath.empty(); }

		float StreamRadius = 320.0f;
		float UnloadRadius = 384.0f;

		// --- Procedural generation ---
		WorldGenConfig GenConfig;
		using GeneratorFn = std::function<void(WorldChunk& chunk, glm::ivec3 coord, WorldGrid& grid)>;
		GeneratorFn Generator;

		// --- RLE helpers (shared by compression + save/load + streaming) ---
		static std::vector<uint8_t> RLEEncode(const WorldCell* cells, int count);
		static void RLEDecode(const std::vector<uint8_t>& data, WorldCell* cells, int count, uint16_t* outFilledCount = nullptr);

		// --- Metrics ---
		size_t GetChunkCount() const { return m_Chunks.size(); }
		float GetMemoryUsageMB() const;

	private:
		std::unordered_map<glm::ivec3, Scope<WorldChunk>, ChunkCoordHash> m_Chunks;
		static const WorldCell s_Air;

		// --- Streaming internals ---
		std::string m_StoragePath;

		struct PendingLoad
		{
			glm::ivec3 Coord;
			std::future<Scope<WorldChunk>> Future;
		};
		std::vector<PendingLoad> m_PendingLoads;
		std::unordered_set<glm::ivec3, ChunkCoordHash> m_LoadingSet;

		std::string ChunkFilePath(glm::ivec3 coord) const;
		std::vector<uint8_t> ReadChunkFile(glm::ivec3 coord) const;
		void WriteChunkFile(glm::ivec3 coord, const std::vector<uint8_t>& rleData) const;
		bool ChunkFileExists(glm::ivec3 coord) const;

		void RequestLoad(glm::ivec3 coord);
		void ProcessCompletedLoads();
		void UnloadChunk(glm::ivec3 coord);
	};
}
