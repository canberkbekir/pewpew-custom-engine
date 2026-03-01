#include "cbpch.h"
#include "WorldSimSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Voxel/World/WorldGrid.h"
#include "CBEngine/Voxel/World/CellularAutomata.h"
#include "CBEngine/Voxel/World/RigidBodyBridge.h"

namespace CB
{
	void WorldSimSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		auto** wgPtr = scene->GetRegistry().ctx().find<WorldGrid*>();
		if (!wgPtr || !*wgPtr)
			return;

		WorldGrid& grid = **wgPtr;

		// Process world damage + stamps + extractions
		RigidBodyBridge::Update(scene, grid, ts);

		// Run cellular automata
		CellularAutomata::Simulate(grid, ts);
	}
}
