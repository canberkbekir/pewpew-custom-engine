#include "cbpch.h"
#include "DestructionSystem.h"

#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Physics/VoxelSplitter.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"
#include "CBEngine/Utils/VoxelizerAPI.h"

namespace CB
{
	std::vector<DestructionRequest> DestructionSystem::s_PendingRequests;

	void DestructionSystem::RequestDestruction(const DestructionRequest& request)
	{
		s_PendingRequests.push_back(request);
	}

	void DestructionSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		if (s_PendingRequests.empty())
			return;

		// Process all pending requests
		auto requests = std::move(s_PendingRequests);
		s_PendingRequests.clear();

		for (const auto& request : requests)
		{
			ProcessRequest(scene, request);
		}
	}

	void DestructionSystem::ProcessRequest(Scene* scene, const DestructionRequest& request)
	{
		Entity targetEntity = scene->GetEntityByUUID(request.TargetEntity);
		if (!targetEntity)
		{
			CB_CORE_WARN("DestructionSystem: Target entity {0} not found", (uint64_t)request.TargetEntity);
			return;
		}

		if (!targetEntity.HasComponent<VoxelRendererComponent>())
		{
			CB_CORE_WARN("DestructionSystem: Target entity {0} has no VoxelRendererComponent",
				(uint64_t)request.TargetEntity);
			return;
		}

		auto& voxelRenderer = targetEntity.GetComponent<VoxelRendererComponent>();

		// Load the VoxelMeshAsset to get the grid
		if (!voxelRenderer.VoxelMeshUUID.IsValid())
		{
			CB_CORE_WARN("DestructionSystem: Target entity has no valid VoxelMeshUUID");
			return;
		}

		auto vmeshAsset = AssetManager::GetAsset<VoxelMeshAsset>(voxelRenderer.VoxelMeshUUID);
		if (!vmeshAsset)
		{
			CB_CORE_WARN("DestructionSystem: Failed to load VoxelMeshAsset");
			return;
		}

		// Work on a copy of the grid
		voxelizer::VoxelGrid grid = vmeshAsset->GridData;
		std::vector<uint8_t> paletteIndices = vmeshAsset->PaletteIndices;

		// Get the entity's world transform to convert impact point to grid-local space
		auto& transform = targetEntity.GetComponent<TransformComponent>();
		Vector3 localImpact = request.ImpactPoint; // TODO: transform to local space if entity has transform offset

		// 1. Remove voxels in the damage sphere
		uint64_t remainingVoxels = VoxelSplitter::RemoveSphere(
			grid, paletteIndices, localImpact, request.DamageRadius);

		if (remainingVoxels == 0)
		{
			// Entity fully destroyed
			CB_CORE_INFO("DestructionSystem: Entity {0} fully destroyed", (uint64_t)request.TargetEntity);
			scene->DestroyEntity(targetEntity);
			return;
		}

		// 2. Find connected components
		auto fragments = VoxelSplitter::FindConnectedComponents(grid, paletteIndices);

		CB_CORE_INFO("DestructionSystem: Entity {0} split into {1} fragments",
			(uint64_t)request.TargetEntity, fragments.size());

		if (fragments.size() <= 1)
		{
			// Single component - just update the existing entity's mesh and collision shape
			if (!fragments.empty())
			{
				// Regenerate mesh from updated grid
				Ref<Mesh> newMesh;
				if (!paletteIndices.empty())
					newMesh = VoxelizerAPI::CreatePaletteMeshFromGrid(grid, paletteIndices);
				else
					newMesh = VoxelizerAPI::CreateMeshFromGrid(grid);

				if (newMesh)
				{
					voxelRenderer.MeshAsset = newMesh;

					// Invalidate collision cache and mark collider dirty
					if (auto* physWorld = scene->GetPhysicsWorld())
						physWorld->GetShapeCache().Invalidate(voxelRenderer.VoxelMeshUUID);
					if (targetEntity.HasComponent<ColliderComponent>())
					{
						auto& collider = targetEntity.GetComponent<ColliderComponent>();
						collider.ShapeDirty = true;
					}
				}
			}
			return;
		}

		// 3. Multiple fragments - destroy original entity and spawn new entities per fragment
		// Save properties from original entity
		Vector3 origPosition = transform.GetWorldPosition();
		Vector3 origRotation = transform.Rotation;
		Vector3 origScale = transform.Scale;
		String origName = targetEntity.GetComponent<TagComponent>().Tag;

		// Get velocity from physics body if it exists
		Vector3 linearVelocity(0.0f);
		Vector3 angularVelocity(0.0f);
		bool hadPhysics = targetEntity.HasComponent<RigidBodyComponent>();
		BodyType origBodyType = BodyType::Dynamic;
		float origFriction = 0.5f;
		float origRestitution = 0.3f;

		if (hadPhysics)
		{
			auto& rb = targetEntity.GetComponent<RigidBodyComponent>();
			origBodyType = rb.Type;
			origFriction = rb.Friction;
			origRestitution = rb.Restitution;
			// Note: velocity retrieval would require access to PhysicsWorld body interface
			// This is simplified - in practice you'd read from Jolt
		}

		// Copy rendering state for fragments
		auto palette = vmeshAsset->Palette;
		auto paletteColorTex = voxelRenderer.PaletteColorTexture;
		auto paletteMaterialTex = voxelRenderer.PaletteMaterialTexture;
		bool hasPalette = voxelRenderer.HasPalette;

		// Destroy the original entity
		scene->DestroyEntity(targetEntity);

		// Spawn new entities for each fragment
		for (size_t i = 0; i < fragments.size(); i++)
		{
			auto& frag = fragments[i];
			if (frag.VoxelCount == 0)
				continue;

			String fragmentName = origName + "_frag" + std::to_string(i);
			Entity fragEntity = scene->CreateEntity(fragmentName);

			// Set transform
			auto& fragTransform = fragEntity.GetComponent<TransformComponent>();
			fragTransform.Position = frag.CenterOfMass;
			fragTransform.Rotation = origRotation;
			fragTransform.Scale = origScale;

			// Create VoxelRendererComponent with regenerated mesh
			auto& fragVoxel = fragEntity.AddComponent<VoxelRendererComponent>();
			fragVoxel.PaletteColorTexture = paletteColorTex;
			fragVoxel.PaletteMaterialTexture = paletteMaterialTex;
			fragVoxel.HasPalette = hasPalette;
			fragVoxel.Visible = true;

			// Generate mesh from fragment grid
			if (!frag.PaletteIndices.empty())
				fragVoxel.MeshAsset = VoxelizerAPI::CreatePaletteMeshFromGrid(frag.Grid, frag.PaletteIndices);
			else
				fragVoxel.MeshAsset = VoxelizerAPI::CreateMeshFromGrid(frag.Grid);

			// Add physics
			auto& fragRB = fragEntity.AddComponent<RigidBodyComponent>();
			fragRB.Type = BodyType::Dynamic;
			fragRB.Mass = static_cast<float>(frag.VoxelCount) * 0.1f; // Mass proportional to voxel count
			fragRB.Friction = origFriction;
			fragRB.Restitution = origRestitution;
			fragRB.UseGravity = true;

			auto& fragCollider = fragEntity.AddComponent<ColliderComponent>();
			fragCollider.Shape = ColliderShape::VoxelCompound;
			fragCollider.ShapeDirty = true;

			// Note: The fragment's VoxelMeshUUID won't be set (runtime-only fragment),
			// so VoxelCompound shape generation will need to handle this case.
			// For fragments, we generate the shape directly here.
			auto compoundShape = VoxelCollisionShapeGenerator::GenerateFromGrid(frag.Grid);
			if (compoundShape)
			{
				fragCollider.RuntimeShape = compoundShape;
				fragCollider.ShapeDirty = false;
			}

			CB_CORE_TRACE("  Fragment {0}: {1} voxels, mass={2:.1f}",
				i, frag.VoxelCount, fragRB.Mass);
		}
	}
}
