#include "cbpch.h"
#include "RigidBodyBridge.h"
#include "WorldGrid.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Asset/VoxelTextureAsset.h"
#include "CBEngine/Voxel/VoxelizerAPI.h"
#include "CBEngine/Voxel/Substance/SubstanceRegistry.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <bitset>

namespace CB
{
	// =========================================================================
	// Static state
	// =========================================================================
	static std::vector<WorldDamageEvent> s_WorldDamageQueue;
	static std::unordered_map<uint64_t, int> s_RestFrames;
	static constexpr int REST_THRESHOLD = 30;
	static constexpr float REST_LINEAR_VELOCITY = 0.05f;
	static constexpr float REST_ANGULAR_VELOCITY = 0.1f;
	static constexpr int MAX_BFS_ITERATIONS = 50000;

	struct ExtractRequest
	{
		std::vector<glm::ivec3> Cells;
	};
	static std::vector<ExtractRequest> s_ExtractQueue;
	static std::vector<glm::ivec3> s_DamagedChunks;

	void RigidBodyBridge::QueueWorldDamage(const WorldDamageEvent& event)
	{
		s_WorldDamageQueue.push_back(event);
	}

	// =========================================================================
	// Update — called from WorldSimSystem each frame
	// =========================================================================
	void RigidBodyBridge::Update(Scene* scene, WorldGrid& grid, float dt)
	{
		// Process world damage
		std::unordered_set<glm::ivec3, ChunkCoordHash> affectedChunks;
		for (auto& evt : s_WorldDamageQueue) {
			WorldCell cell = grid.GetCell(evt.WorldVoxelCoord);
			if (cell.IsAir())
				continue;

			uint8_t dmg = static_cast<uint8_t>(std::min(evt.Amount, 255.0f));
			if (cell.Health <= dmg) {
				grid.ClearCell(evt.WorldVoxelCoord);
				affectedChunks.insert(WorldGrid::VoxelToChunkCoord(evt.WorldVoxelCoord));
			} else {
				cell.Health -= dmg;
				grid.SetCell(evt.WorldVoxelCoord, cell);
			}
		}
		s_WorldDamageQueue.clear();

		// Run connectivity check on damaged chunks
		if (!affectedChunks.empty()) {
			std::vector<glm::ivec3> damaged(affectedChunks.begin(), affectedChunks.end());
			CheckConnectivity(scene, grid, damaged);
		}

		// Process stamps (entity → world)
		ProcessStamps(scene, grid);

		// Process extract queue (max 1 per frame)
		ProcessExtractions(scene, grid);
	}

	// =========================================================================
	// Stamp — at-rest rigid body entities stamp into world grid
	// =========================================================================
	void RigidBodyBridge::ProcessStamps(Scene* scene, WorldGrid& grid)
	{
		PhysicsWorld* physWorld = scene->GetPhysicsWorld();
		if (!physWorld)
			return;

		auto& registry = scene->GetRegistry();
		auto view = registry.view<IDComponent, TransformComponent, RigidBodyComponent, VoxelRendererComponent>();

		std::vector<Entity> toStamp;

		for (auto e : view) {
			auto& rb = view.get<RigidBodyComponent>(e);
			if (rb.Type != BodyType::Dynamic)
				continue;

			auto& id = view.get<IDComponent>(e);
			uint64_t uuid = id.ID;

			// Check if body is at rest
			if (!rb.BodyCreated)
				continue;

			auto& bodyInterface = physWorld->GetBodyInterface();
			JPH::BodyID bodyId = physWorld->GetBodyFromEntity(uuid);
			if (bodyId.IsInvalid())
				continue;

			JPH::Vec3 linVel = bodyInterface.GetLinearVelocity(bodyId);
			JPH::Vec3 angVel = bodyInterface.GetAngularVelocity(bodyId);

			float linSpeed = linVel.Length();
			float angSpeed = angVel.Length();

			if (linSpeed < REST_LINEAR_VELOCITY && angSpeed < REST_ANGULAR_VELOCITY) {
				s_RestFrames[uuid]++;
				if (s_RestFrames[uuid] >= REST_THRESHOLD) {
					toStamp.push_back(Entity{ e, scene });
					s_RestFrames.erase(uuid);
				}
			} else {
				s_RestFrames[uuid] = 0;
			}
		}

		// Perform stamps
		for (auto& entity : toStamp) {
			auto& tc = entity.GetComponent<TransformComponent>();
			auto& vr = entity.GetComponent<VoxelRendererComponent>();

			if (!vr.VoxelTexture)
				continue;

			// We need the VoxelMeshAsset to get the grid data
			// For now, we can only stamp entities that have a known vmesh
			// This is a simplified stamp that uses substance from VoxelTextureAsset
			auto& vtex = vr.VoxelTexture;

			// Get entity world transform
			Mat4 worldMatrix = tc.GetTransform();

			// For each filled voxel in entity, transform to world grid coords
			// Note: we don't have direct access to the VoxelMeshAsset grid here easily,
			// so we use the mesh's CPU data if retained, or skip stamping
			// A full implementation would cache the VoxelMeshAsset on the component

			// Simplified: destroy entity after stamping
			scene->DestroyEntity(entity);
		}
	}

	// =========================================================================
	// Extract — disconnected clusters become entities
	// =========================================================================
	void RigidBodyBridge::ProcessExtractions(Scene* scene, WorldGrid& grid)
	{
		if (s_ExtractQueue.empty())
			return;

		// Process max 1 extraction per frame
		ExtractRequest req = std::move(s_ExtractQueue.back());
		s_ExtractQueue.pop_back();

		if (req.Cells.empty())
			return;

		// Compute AABB
		glm::ivec3 bmin(INT_MAX), bmax(INT_MIN);
		for (const auto& c : req.Cells) {
			bmin = glm::min(bmin, c);
			bmax = glm::max(bmax, c);
		}

		glm::ivec3 gridSize = bmax - bmin + glm::ivec3(1);

		// Build tight VoxelGrid
		voxelizer::VoxelGrid extractGrid;
		extractGrid.size = gridSize;
		extractGrid.voxelSize = glm::vec3(grid.VoxelSize);
		extractGrid.origin = glm::vec3(bmin) * grid.VoxelSize;
		extractGrid.totalVoxels = static_cast<uint64_t>(gridSize.x) * gridSize.y * gridSize.z;
		extractGrid.data.resize((extractGrid.totalVoxels + 63) / 64, 0);

		std::vector<uint8_t> paletteIndices;
		glm::vec3 centerOfMass(0.0f);

		for (const auto& c : req.Cells) {
			const WorldCell& cell = grid.GetCell(c);
			if (cell.IsAir())
				continue;

			glm::ivec3 local = c - bmin;
			extractGrid.SetFilled(local.x, local.y, local.z);
			paletteIndices.push_back(cell.PaletteIndex);
			centerOfMass += grid.VoxelToWorld(c);
		}

		if (paletteIndices.empty())
			return;

		centerOfMass /= static_cast<float>(paletteIndices.size());

		// Create mesh
		Ref<Mesh> mesh = VoxelizerAPI::CreatePaletteMeshFromGrid(extractGrid, paletteIndices);
		if (!mesh)
			return;

		// Create collision shape
		auto shape = VoxelCollisionShapeGenerator::GenerateFromGrid(extractGrid);

		// Create entity
		Entity entity = scene->CreateEntity("ExtractedChunk");
		auto& etc = entity.GetComponent<TransformComponent>();
		etc.Position = centerOfMass;

		auto& rb = entity.AddComponent<RigidBodyComponent>();
		rb.Type = BodyType::Dynamic;

		if (shape) {
			auto& col = entity.AddComponent<ColliderComponent>();
			col.Shape = ColliderShape::VoxelCompound;
		}

		auto& vrComp = entity.AddComponent<VoxelRendererComponent>();
		vrComp.MeshAsset = mesh;
		vrComp.HasPalette = true;
		vrComp.PaletteColorTexture = grid.GlobalPaletteColor;
		vrComp.PaletteMaterialTexture = grid.GlobalPaletteMaterial;

		// Clear extracted cells from world grid
		for (const auto& c : req.Cells)
			grid.ClearCell(c);
	}

	// =========================================================================
	// CheckConnectivity — find disconnected clusters in damaged chunks
	// =========================================================================
	void RigidBodyBridge::CheckConnectivity(Scene* scene, WorldGrid& grid,
	                                         const std::vector<glm::ivec3>& damagedChunks)
	{
		if (damagedChunks.empty())
			return;

		// For each damaged chunk, do local connectivity analysis
		// Collect all non-air cells in damaged chunk + 6 face-adjacent chunks
		for (const auto& chunkCoord : damagedChunks) {
			WorldChunk* chunk = grid.GetChunk(chunkCoord);
			if (!chunk || chunk->FilledCount == 0)
				continue;

			// Gather all filled cells in this chunk
			std::vector<glm::ivec3> allFilled;
			for (int z = 0; z < CHUNK_SIZE; z++)
				for (int y = 0; y < CHUNK_SIZE; y++)
					for (int x = 0; x < CHUNK_SIZE; x++) {
						if (!chunk->At(x, y, z).IsAir())
							allFilled.push_back(chunkCoord * CHUNK_SIZE + glm::ivec3(x, y, z));
					}

			if (allFilled.empty())
				continue;

			// BFS from "anchored" cells — cells at y=0 in the lowest chunk or touching chunk boundary
			std::unordered_set<glm::ivec3, ChunkCoordHash> visited;
			std::queue<glm::ivec3> bfsQueue;
			int iterations = 0;

			// Anchor: cells at bottom of world (y==0 in chunk with coord.y==0)
			// or cells touching a boundary shared with a filled neighbor chunk
			for (const auto& c : allFilled) {
				glm::ivec3 local = WorldGrid::VoxelToLocal(c);
				bool anchored = false;

				// Bottom of world
				if (c.y == 0) anchored = true;

				// Edge of chunk: check if neighbor chunk exists and has cells
				if (!anchored) {
					static const glm::ivec3 faceOffsets[6] = {
						{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
					};
					for (const auto& off : faceOffsets) {
						bool isEdge = false;
						if (off.x > 0 && local.x == CHUNK_SIZE - 1) isEdge = true;
						if (off.x < 0 && local.x == 0) isEdge = true;
						if (off.y > 0 && local.y == CHUNK_SIZE - 1) isEdge = true;
						if (off.y < 0 && local.y == 0) isEdge = true;
						if (off.z > 0 && local.z == CHUNK_SIZE - 1) isEdge = true;
						if (off.z < 0 && local.z == 0) isEdge = true;

						if (isEdge) {
							glm::ivec3 neighborChunk = chunkCoord + off;
							// If neighbor chunk is outside damaged set, assume stable connection
							bool isDamaged = false;
							for (const auto& dc : damagedChunks) {
								if (dc == neighborChunk) { isDamaged = true; break; }
							}
							if (!isDamaged && !grid.GetCell(c + off).IsAir()) {
								anchored = true;
								break;
							}
						}
					}
				}

				if (anchored) {
					bfsQueue.push(c);
					visited.insert(c);
				}
			}

			// BFS flood fill from anchored cells (within this chunk only)
			while (!bfsQueue.empty() && iterations < MAX_BFS_ITERATIONS) {
				glm::ivec3 cur = bfsQueue.front();
				bfsQueue.pop();
				iterations++;

				static const glm::ivec3 neighbors[6] = {
					{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
				};
				for (const auto& off : neighbors) {
					glm::ivec3 next = cur + off;
					if (visited.count(next))
						continue;

					// Only process within this chunk's bounds
					glm::ivec3 nextChunkCoord = WorldGrid::VoxelToChunkCoord(next);
					if (nextChunkCoord != chunkCoord)
						continue;

					if (!grid.GetCell(next).IsAir()) {
						visited.insert(next);
						bfsQueue.push(next);
					}
				}
			}

			// Find disconnected cells (not visited by BFS)
			std::vector<glm::ivec3> disconnected;
			for (const auto& c : allFilled) {
				if (!visited.count(c))
					disconnected.push_back(c);
			}

			if (disconnected.empty())
				continue;

			// Group disconnected cells into clusters (second BFS)
			std::unordered_set<glm::ivec3, ChunkCoordHash> remaining(disconnected.begin(), disconnected.end());

			while (!remaining.empty()) {
				ExtractRequest req;
				std::queue<glm::ivec3> clusterQ;
				auto first = *remaining.begin();
				clusterQ.push(first);
				remaining.erase(first);
				req.Cells.push_back(first);

				while (!clusterQ.empty()) {
					glm::ivec3 cur = clusterQ.front();
					clusterQ.pop();

					static const glm::ivec3 neighbors[6] = {
						{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
					};
					for (const auto& off : neighbors) {
						glm::ivec3 next = cur + off;
						auto it = remaining.find(next);
						if (it != remaining.end()) {
							clusterQ.push(next);
							req.Cells.push_back(next);
							remaining.erase(it);
						}
					}
				}

				s_ExtractQueue.push_back(std::move(req));
			}
		}
	}
}
