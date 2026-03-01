#include "cbpch.h"
#include "WorldRenderSystem.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Voxel/World/WorldGrid.h"
#include "CBEngine/Voxel/World/WorldChunkMesher.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Voxelizer.h>

#include <algorithm>
#include <vector>

namespace CB
{
	static constexpr int MAX_MESH_REBUILDS_PER_FRAME = 8;
	static constexpr int MAX_PHYSICS_REBUILDS_PER_FRAME = 8;
	static constexpr uint32_t FREEZE_FRAMES = 300; // ~5s at 60fps

	void WorldRenderSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		auto** wgPtr = scene->GetRegistry().ctx().find<WorldGrid*>();
		if (!wgPtr || !*wgPtr)
			return;

		WorldGrid& grid = **wgPtr;

		// Stream chunks so generation/loading works in edit mode too
		grid.StreamUpdate(grid.Region);

		// Collect dirty chunks in render range, sorted by distance to Region.Center
		struct DirtyChunkEntry
		{
			WorldChunk* Chunk;
			float DistSq;
			uint8_t DesiredLOD;
		};

		std::vector<DirtyChunkEntry> dirtyMesh;
		std::vector<DirtyChunkEntry> dirtyPhysics;

		for (auto& [coord, chunk] : grid.Chunks()) {
			if (!chunk)
				continue;

			float chunkWorldSize = CHUNK_SIZE * grid.VoxelSize;
			glm::vec3 chunkCenter = glm::vec3(coord) * chunkWorldSize + glm::vec3(chunkWorldSize * 0.5f);
			float distSq = glm::dot(chunkCenter - grid.Region.Center, chunkCenter - grid.Region.Center);
			float dist = std::sqrt(distSq);

			// Determine desired LOD
			uint8_t desiredLOD = 0;
			if (dist > grid.Region.LOD2Distance) desiredLOD = 2;
			else if (dist > grid.Region.LOD1Distance) desiredLOD = 1;

			bool needsRebuild = (chunk->Generation != chunk->MeshGeneration);
			bool lodChanged = (desiredLOD != chunk->CurrentLOD);

			if ((needsRebuild || lodChanged) &&
				grid.Region.ChunkInRange(coord, grid.VoxelSize, grid.Region.RenderRadius))
				dirtyMesh.push_back({ chunk.get(), distSq, desiredLOD });

			if (chunk->PhysicsDirty &&
				grid.Region.ChunkInRange(coord, grid.VoxelSize, grid.Region.PhysicsRadius))
				dirtyPhysics.push_back({ chunk.get(), distSq, 0 });
		}

		// Sort nearest first
		auto sortByDist = [](const DirtyChunkEntry& a, const DirtyChunkEntry& b) {
			return a.DistSq < b.DistSq;
		};
		std::sort(dirtyMesh.begin(), dirtyMesh.end(), sortByDist);
		std::sort(dirtyPhysics.begin(), dirtyPhysics.end(), sortByDist);

		// Rebuild meshes (budget limited)
		// LOD1 costs 0.5, LOD2 costs 0.25 of budget
		float meshBudget = static_cast<float>(MAX_MESH_REBUILDS_PER_FRAME);
		for (auto& entry : dirtyMesh) {
			float cost = 1.0f;
			if (entry.DesiredLOD == 1) cost = 0.5f;
			else if (entry.DesiredLOD == 2) cost = 0.25f;

			if (meshBudget < cost)
				break;

			int step = 1 << entry.DesiredLOD; // 1, 2, or 4
			entry.Chunk->CachedMesh = WorldChunkMesher::BuildMesh(*entry.Chunk, grid, step);
			entry.Chunk->CurrentLOD = entry.DesiredLOD;
			entry.Chunk->MeshGeneration = entry.Chunk->Generation;
			entry.Chunk->MeshDirty = false;
			meshBudget -= cost;
		}

		// Rebuild physics (budget limited, only when physics world exists)
		PhysicsWorld* physWorld = scene->GetPhysicsWorld();
		int physicsRebuilds = 0;
		for (auto& entry : dirtyPhysics) {
			if (!physWorld)
				break;
			if (physicsRebuilds >= MAX_PHYSICS_REBUILDS_PER_FRAME)
				break;

			WorldChunk* chunk = entry.Chunk;

			// Ensure decompressed for physics rebuild
			chunk->EnsureDecompressed();

			// Build temporary voxelizer::VoxelGrid from chunk occupancy
			voxelizer::VoxelGrid tempGrid;
			tempGrid.size = glm::ivec3(CHUNK_SIZE);
			tempGrid.voxelSize = glm::vec3(grid.VoxelSize);
			tempGrid.origin = glm::vec3(chunk->Coord * CHUNK_SIZE) * grid.VoxelSize;
			tempGrid.totalVoxels = CHUNK_VOLUME;
			tempGrid.data.resize((CHUNK_VOLUME + 63) / 64, 0);

			bool hasAny = false;
			for (int i = 0; i < CHUNK_VOLUME; i++) {
				if (!chunk->Cells[i].IsAir()) {
					uint64_t arrIdx = i / 64;
					uint64_t bitIdx = i % 64;
					tempGrid.data[arrIdx] |= (uint64_t(1) << bitIdx);
					hasAny = true;
				}
			}

			// Destroy old body if exists
			if (chunk->HasPhysicsBody) {
				auto& bodyInterface = physWorld->GetBodyInterface();
				bodyInterface.RemoveBody(chunk->PhysicsBodyID);
				bodyInterface.DestroyBody(chunk->PhysicsBodyID);
				chunk->HasPhysicsBody = false;
			}

			if (hasAny) {
				auto shape = VoxelCollisionShapeGenerator::GenerateFromGrid(tempGrid);
				if (shape) {
					JPH::BodyCreationSettings bodySettings(
						shape,
						JPH::RVec3(tempGrid.origin.x, tempGrid.origin.y, tempGrid.origin.z),
						JPH::Quat::sIdentity(),
						JPH::EMotionType::Static,
						0 // Object layer 0 (default static)
					);
					bodySettings.mFriction = 0.5f;
					bodySettings.mRestitution = 0.3f;

					JPH::Body* body = physWorld->GetBodyInterface().CreateBody(bodySettings);
					if (body) {
						chunk->PhysicsBodyID = body->GetID();
						chunk->HasPhysicsBody = true;
						physWorld->GetBodyInterface().AddBody(chunk->PhysicsBodyID, JPH::EActivation::DontActivate);
					}
				}
			}

			chunk->PhysicsDirty = false;
			physicsRebuilds++;
		}

		// Freeze detection — compress idle chunks outside SimRadius
		for (auto& [coord, chunk] : grid.Chunks()) {
			if (!chunk || chunk->IsCompressed) continue;
			if (grid.Region.ChunkInRange(coord, grid.VoxelSize, grid.Region.SimRadius))
				continue; // don't compress active chunks

			if (chunk->Generation == chunk->IdleGeneration) {
				chunk->IdleFrames++;
				if (chunk->IdleFrames >= FREEZE_FRAMES)
					chunk->Compress();
			} else {
				chunk->IdleGeneration = chunk->Generation;
				chunk->IdleFrames = 0;
			}
		}
	}
}
