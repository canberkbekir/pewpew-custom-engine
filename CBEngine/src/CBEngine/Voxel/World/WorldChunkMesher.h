#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Renderer/Resources/Mesh.h"

namespace CB
{
	struct WorldChunk;
	class WorldGrid;

	class WorldChunkMesher
	{
	public:
		// Build a greedy-meshed mesh for a single chunk.
		// Uses grid for cross-chunk boundary neighbor lookups.
		// Must be called on the main thread (creates GL resources).
		// Returns nullptr if chunk has no exposed faces.
		// step=1 full detail, step=2 half-res (LOD1), step=4 quarter-res (LOD2)
		static Ref<Mesh> BuildMesh(WorldChunk& chunk, const WorldGrid& grid, int step = 1);
	};
}
