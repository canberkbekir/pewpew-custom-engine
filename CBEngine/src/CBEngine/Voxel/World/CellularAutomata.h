#pragma once

namespace CB
{
	class WorldGrid;

	class CellularAutomata
	{
	public:
		static void Init(WorldGrid& grid);
		static void Simulate(WorldGrid& grid, float dt);

	private:
		static void ProcessChunkInterior(WorldGrid& grid, struct WorldChunk& chunk, float dt);
		static void ProcessChunkBoundary(WorldGrid& grid, struct WorldChunk& chunk, float dt);
		static void SimPowder(WorldGrid& grid, glm::ivec3 coord, struct WorldCell& cell);
		static void SimLiquid(WorldGrid& grid, glm::ivec3 coord, struct WorldCell& cell);
		static void SimGas(WorldGrid& grid, glm::ivec3 coord, struct WorldCell& cell);
		static void SimFire(WorldGrid& grid, glm::ivec3 coord, struct WorldCell& cell, float dt);
		static void SwapCells(WorldGrid& grid, glm::ivec3 a, glm::ivec3 b);
	};
}
