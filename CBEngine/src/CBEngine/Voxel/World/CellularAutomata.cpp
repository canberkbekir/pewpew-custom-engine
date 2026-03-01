#include "cbpch.h"
#include "CellularAutomata.h"
#include "WorldGrid.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"

#include <random>
#include <algorithm>
#include <chrono>
#include <vector>
#include <future>
#include <thread>

namespace CB
{
	// =========================================================================
	// Internal state
	// =========================================================================
	static bool s_FlipX = false;
	static bool s_FlipZ = false;
	static std::vector<const VoxelSubstanceProperties*> s_Props;
	static thread_local std::mt19937 t_Rng(42);
	static constexpr float MAX_SIM_TIME_MS = 4.0f;

	// =========================================================================
	// Init — build substance property lookup table
	// =========================================================================
	void CellularAutomata::Init(WorldGrid& grid)
	{
		s_Props.clear();
		s_Props.resize(grid.SubstanceLUT.size(), nullptr);
		for (uint16_t i = 1; i < grid.SubstanceLUT.size(); i++) {
			s_Props[i] = &SubstanceRegistry::Get(grid.SubstanceLUT[i]);
		}
	}

	// =========================================================================
	// Simulate — one tick with checkerboard parallelism
	// =========================================================================
	void CellularAutomata::Simulate(WorldGrid& grid, float dt)
	{
		grid.CurrentTick++;

		// Flip scan direction each frame
		s_FlipX = !s_FlipX;
		s_FlipZ = !s_FlipZ;

		// Collect in-range chunks, sorted by distance
		struct ChunkEntry { WorldChunk* Chunk; float DistSq; };
		std::vector<ChunkEntry> chunks;

		for (auto& [coord, chunk] : grid.Chunks()) {
			if (!chunk || chunk->FilledCount == 0)
				continue;
			if (!grid.Region.ChunkInRange(coord, grid.VoxelSize, grid.Region.SimRadius))
				continue;

			float chunkWorldSize = CHUNK_SIZE * grid.VoxelSize;
			glm::vec3 center = glm::vec3(coord) * chunkWorldSize + glm::vec3(chunkWorldSize * 0.5f);
			float distSq = glm::dot(center - grid.Region.Center, center - grid.Region.Center);
			chunks.push_back({ chunk.get(), distSq });
		}

		std::sort(chunks.begin(), chunks.end(),
			[](const ChunkEntry& a, const ChunkEntry& b) { return a.DistSq < b.DistSq; });

		// Partition into checkerboard groups
		// Group = (cx+cy+cz) & 1. Chunks in the same group share no face-neighbors.
		std::vector<ChunkEntry> group0, group1;
		for (auto& e : chunks) {
			auto c = e.Chunk->Coord;
			if ((c.x + c.y + c.z) & 1)
				group1.push_back(e);
			else
				group0.push_back(e);
		}

		unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
		auto startTime = std::chrono::high_resolution_clock::now();

		// Process a group: parallel interior, then serial boundary
		auto processGroup = [&](std::vector<ChunkEntry>& group)
		{
			// Interior pass — parallel via std::async
			std::vector<std::future<void>> futures;
			size_t asyncCount = std::min(static_cast<size_t>(maxThreads), group.size());
			futures.reserve(asyncCount);

			for (size_t i = 0; i < asyncCount; i++) {
				futures.push_back(std::async(std::launch::async,
					[&grid, chunk = group[i].Chunk, dt] {
						// Seed thread_local RNG from thread ID on first use
						static thread_local bool seeded = false;
						if (!seeded) {
							t_Rng.seed(static_cast<unsigned int>(
								std::hash<std::thread::id>{}(std::this_thread::get_id())));
							seeded = true;
						}
						ProcessChunkInterior(grid, *chunk, dt);
					}));
			}
			// Process remaining chunks sequentially on main thread
			for (size_t i = asyncCount; i < group.size(); i++)
				ProcessChunkInterior(grid, *group[i].Chunk, dt);

			// Wait for async tasks
			for (auto& f : futures) f.get();

			// Boundary pass — serial on main thread (cross-chunk writes possible)
			for (auto& e : group)
				ProcessChunkBoundary(grid, *e.Chunk, dt);
		};

		// Group 0
		processGroup(group0);

		// Time check
		float elapsed = std::chrono::duration<float, std::milli>(
			std::chrono::high_resolution_clock::now() - startTime).count();
		if (elapsed >= MAX_SIM_TIME_MS)
			return;

		// Group 1
		processGroup(group1);
	}

	// =========================================================================
	// Cell iteration helper — iterates with scan-direction flipping
	// =========================================================================
	template<typename Fn>
	static void IterateCells(bool flipX, bool flipZ, bool interiorOnly, bool boundaryOnly, Fn&& fn)
	{
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int zRaw = 0; zRaw < CHUNK_SIZE; zRaw++) {
				int z = flipZ ? (CHUNK_SIZE - 1 - zRaw) : zRaw;
				for (int xRaw = 0; xRaw < CHUNK_SIZE; xRaw++) {
					int x = flipX ? (CHUNK_SIZE - 1 - xRaw) : xRaw;

					bool isBoundary = (x == 0 || x == CHUNK_SIZE - 1 ||
					                   y == 0 || y == CHUNK_SIZE - 1 ||
					                   z == 0 || z == CHUNK_SIZE - 1);

					if (interiorOnly && isBoundary) continue;
					if (boundaryOnly && !isBoundary) continue;

					fn(x, y, z);
				}
			}
		}
	}

	// =========================================================================
	// ProcessChunkInterior — safe for parallel execution (no cross-chunk writes)
	// =========================================================================
	void CellularAutomata::ProcessChunkInterior(WorldGrid& grid, WorldChunk& chunk, float dt)
	{
		IterateCells(s_FlipX, s_FlipZ, true, false, [&](int x, int y, int z) {
			WorldCell& cell = chunk.At(x, y, z);
			if (cell.SubstanceIndex == 0) return;
			if (cell.UpdateTick == grid.CurrentTick) return;
			if (cell.Flags & CellFlag_Static) return;

			glm::ivec3 worldCoord = chunk.Coord * CHUNK_SIZE + glm::ivec3(x, y, z);

			if (cell.Flags & CellFlag_Powder)
				SimPowder(grid, worldCoord, cell);
			else if (cell.Flags & CellFlag_Liquid)
				SimLiquid(grid, worldCoord, cell);
			else if (cell.Flags & CellFlag_Gas)
				SimGas(grid, worldCoord, cell);

			if (cell.Flags & CellFlag_Burning)
				SimFire(grid, worldCoord, cell, dt);
		});
	}

	// =========================================================================
	// ProcessChunkBoundary — must run on main thread (cross-chunk writes)
	// =========================================================================
	void CellularAutomata::ProcessChunkBoundary(WorldGrid& grid, WorldChunk& chunk, float dt)
	{
		IterateCells(s_FlipX, s_FlipZ, false, true, [&](int x, int y, int z) {
			WorldCell& cell = chunk.At(x, y, z);
			if (cell.SubstanceIndex == 0) return;
			if (cell.UpdateTick == grid.CurrentTick) return;
			if (cell.Flags & CellFlag_Static) return;

			glm::ivec3 worldCoord = chunk.Coord * CHUNK_SIZE + glm::ivec3(x, y, z);

			if (cell.Flags & CellFlag_Powder)
				SimPowder(grid, worldCoord, cell);
			else if (cell.Flags & CellFlag_Liquid)
				SimLiquid(grid, worldCoord, cell);
			else if (cell.Flags & CellFlag_Gas)
				SimGas(grid, worldCoord, cell);

			if (cell.Flags & CellFlag_Burning)
				SimFire(grid, worldCoord, cell, dt);
		});
	}

	// =========================================================================
	// SimPowder — sand/gravel: fall, sink through lighter liquids, diagonal slide
	// =========================================================================
	void CellularAutomata::SimPowder(WorldGrid& grid, glm::ivec3 coord, WorldCell& cell)
	{
		const WorldCell& below = grid.GetCell(coord + glm::ivec3(0, -1, 0));

		// Fall into air
		if (below.IsAir()) {
			SwapCells(grid, coord, coord + glm::ivec3(0, -1, 0));
			return;
		}

		// Sink through lighter liquid
		if ((below.Flags & CellFlag_Liquid) && below.SubstanceIndex < s_Props.size()) {
			const auto* belowProps = s_Props[below.SubstanceIndex];
			const auto* cellProps = cell.SubstanceIndex < s_Props.size() ? s_Props[cell.SubstanceIndex] : nullptr;
			if (belowProps && cellProps && belowProps->CellDensity < cellProps->CellDensity) {
				SwapCells(grid, coord, coord + glm::ivec3(0, -1, 0));
				return;
			}
		}

		// Try diagonal-down (random one of 4 directions)
		static const glm::ivec3 diags[4] = {
			{1, -1, 0}, {-1, -1, 0}, {0, -1, 1}, {0, -1, -1}
		};
		int start = t_Rng() % 4;
		for (int i = 0; i < 4; i++) {
			const glm::ivec3& diag = diags[(start + i) % 4];
			const WorldCell& diagCell = grid.GetCell(coord + diag);
			if (diagCell.IsAir()) {
				SwapCells(grid, coord, coord + diag);
				return;
			}
		}
	}

	// =========================================================================
	// SimLiquid — water/lava: fall, spread laterally
	// =========================================================================
	void CellularAutomata::SimLiquid(WorldGrid& grid, glm::ivec3 coord, WorldCell& cell)
	{
		const WorldCell& below = grid.GetCell(coord + glm::ivec3(0, -1, 0));

		// Fall into air
		if (below.IsAir()) {
			SwapCells(grid, coord, coord + glm::ivec3(0, -1, 0));
			return;
		}

		// Spread laterally (random order)
		static const glm::ivec3 horizontal[4] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
		};
		int start = t_Rng() % 4;
		for (int i = 0; i < 4; i++) {
			const glm::ivec3& dir = horizontal[(start + i) % 4];
			const WorldCell& neighbor = grid.GetCell(coord + dir);
			if (neighbor.IsAir()) {
				SwapCells(grid, coord, coord + dir);
				return;
			}
		}
	}

	// =========================================================================
	// SimGas — rise, diffuse laterally, decay
	// =========================================================================
	void CellularAutomata::SimGas(WorldGrid& grid, glm::ivec3 coord, WorldCell& cell)
	{
		const WorldCell& above = grid.GetCell(coord + glm::ivec3(0, 1, 0));

		// Rise into air or lighter gas
		if (above.IsAir()) {
			SwapCells(grid, coord, coord + glm::ivec3(0, 1, 0));
			return;
		}
		if ((above.Flags & CellFlag_Gas) && above.SubstanceIndex < s_Props.size() && cell.SubstanceIndex < s_Props.size()) {
			const auto* aboveProps = s_Props[above.SubstanceIndex];
			const auto* cellProps = s_Props[cell.SubstanceIndex];
			if (aboveProps && cellProps && aboveProps->CellDensity < cellProps->CellDensity) {
				SwapCells(grid, coord, coord + glm::ivec3(0, 1, 0));
				return;
			}
		}

		// Diffuse laterally
		static const glm::ivec3 horizontal[4] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
		};
		int dir = t_Rng() % 4;
		const WorldCell& neighbor = grid.GetCell(coord + horizontal[dir]);
		if (neighbor.IsAir()) {
			SwapCells(grid, coord, coord + horizontal[dir]);
			return;
		}

		// Decay
		if (cell.Density > 0) {
			cell.Density--;
			if (cell.Density == 0)
				grid.ClearCell(coord);
		}
	}

	// =========================================================================
	// SimFire — burn, reduce health, spread to neighbors
	// =========================================================================
	void CellularAutomata::SimFire(WorldGrid& grid, glm::ivec3 coord, WorldCell& cell, float dt)
	{
		if (cell.SubstanceIndex == 0 || cell.SubstanceIndex >= s_Props.size())
			return;
		const auto* props = s_Props[cell.SubstanceIndex];
		if (!props)
			return;

		// Reduce health
		float healthLoss = props->Health > 0.0f ? (dt * 255.0f / std::max(props->BurnDuration, 0.1f)) : 0.0f;
		uint8_t delta = static_cast<uint8_t>(std::min(healthLoss, 255.0f));
		if (delta < 1) delta = 1;

		if (cell.Health <= delta) {
			grid.ClearCell(coord);
			return;
		}
		cell.Health -= delta;

		// Increase temperature
		float tempInc = props->BurnHeatOutput * dt * 0.1f;
		cell.Temperature = static_cast<uint8_t>(std::min(255.0f, static_cast<float>(cell.Temperature) + tempInc));

		// Spread to 6 face neighbors
		static const glm::ivec3 faceNeighbors[6] = {
			{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
		};
		for (const auto& offset : faceNeighbors) {
			glm::ivec3 nCoord = coord + offset;
			WorldCell neighbor = grid.GetCell(nCoord); // copy
			if (neighbor.IsAir() || (neighbor.Flags & CellFlag_Burning))
				continue;
			if (neighbor.SubstanceIndex == 0 || neighbor.SubstanceIndex >= s_Props.size())
				continue;

			const auto* nProps = s_Props[neighbor.SubstanceIndex];
			if (!nProps || !nProps->Flammable)
				continue;

			// Check ignition temperature
			float ignitionTemp = nProps->IgnitionTemperature * 0.1f;
			if (static_cast<float>(cell.Temperature) >= ignitionTemp) {
				neighbor.Flags |= CellFlag_Burning;
				grid.SetCell(nCoord, neighbor);
			}
		}
	}

	// =========================================================================
	// SwapCells — swap two cells and mark both as updated
	// =========================================================================
	void CellularAutomata::SwapCells(WorldGrid& grid, glm::ivec3 a, glm::ivec3 b)
	{
		WorldCell cellA = grid.GetCell(a);
		WorldCell cellB = grid.GetCell(b);

		cellA.UpdateTick = grid.CurrentTick;
		cellB.UpdateTick = grid.CurrentTick;

		grid.SetCell(a, cellB);
		grid.SetCell(b, cellA);
	}
}
