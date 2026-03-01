#pragma once

namespace CB
{
	struct WorldChunk;
	struct WorldGenConfig;
	class WorldGrid;

	class WorldGenerator
	{
	public:
		static void Generate(WorldChunk& chunk, glm::ivec3 coord,
		                     const WorldGenConfig& config, WorldGrid& grid);

	private:
		static void GenerateFlat(WorldChunk& chunk, glm::ivec3 coord,
		                         const WorldGenConfig& config, WorldGrid& grid);
		static void GenerateNoise(WorldChunk& chunk, glm::ivec3 coord,
		                          const WorldGenConfig& config, WorldGrid& grid);
	};
}
