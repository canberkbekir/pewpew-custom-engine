#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"

#include <vector>
#include <cstdint>

namespace CB
{
	class Scene;
	class WorldGrid;

	struct WorldDamageEvent
	{
		glm::ivec3 WorldVoxelCoord;
		float Amount = 0.0f;
	};

	class RigidBodyBridge
	{
	public:
		static void Update(Scene* scene, WorldGrid& grid, float dt);
		static void QueueWorldDamage(const WorldDamageEvent& event);

	private:
		// Stamp: at-rest dynamic entity voxels into world grid
		static void ProcessStamps(Scene* scene, WorldGrid& grid);

		// Extract: disconnected world grid clusters become entities
		static void ProcessExtractions(Scene* scene, WorldGrid& grid);

		// Connectivity check on damaged chunks
		static void CheckConnectivity(Scene* scene, WorldGrid& grid, const std::vector<glm::ivec3>& damagedChunks);
	};
}
