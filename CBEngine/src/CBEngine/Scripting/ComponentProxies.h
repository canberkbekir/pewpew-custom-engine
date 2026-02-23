#pragma once

#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/AudioSourceComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/BodyInterface.h>

namespace CB
{
	// =========================================================================
	// RigidBodyProxy
	// =========================================================================
	struct RigidBodyProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<RigidBodyComponent>()
				&& OwnerEntity.GetComponent<RigidBodyComponent>().BodyCreated;
		}
		PhysicsWorld* GetWorld() const
		{
			Scene* scene = OwnerEntity.GetScene();
			return scene ? scene->GetPhysicsWorld() : nullptr;
		}
		JPH::BodyID GetBodyID() const
		{
			return OwnerEntity.GetComponent<RigidBodyComponent>().RuntimeBodyID;
		}
	};

	// =========================================================================
	// TransformProxy
	// =========================================================================
	struct TransformProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<TransformComponent>();
		}
	};

	// =========================================================================
	// CameraProxy
	// =========================================================================
	struct CameraProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<CameraComponent>();
		}
	};

	// =========================================================================
	// ColliderProxy
	// =========================================================================
	struct ColliderProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<ColliderComponent>();
		}
	};

	// =========================================================================
	// MeshRendererProxy
	// =========================================================================
	struct MeshRendererProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<MeshRendererComponent>();
		}
	};

	// =========================================================================
	// VoxelRendererProxy
	// =========================================================================
	struct VoxelRendererProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<VoxelRendererComponent>();
		}
	};

	// =========================================================================
	// DirectionalLightProxy
	// =========================================================================
	struct DirectionalLightProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<DirectionalLightComponent>();
		}
	};

	// =========================================================================
	// MeshProxy — wraps a Ref<Mesh>
	// =========================================================================
	struct MeshProxy
	{
		Ref<Mesh> MeshAsset;

		bool IsValid() const { return MeshAsset != nullptr; }
	};

	// =========================================================================
	// MaterialProxy — wraps the material on a MeshRendererComponent
	// =========================================================================
	struct MaterialProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<MeshRendererComponent>()
				&& OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset != nullptr;
		}
	};

	// =========================================================================
	// AudioSourceProxy — wraps AudioSourceComponent
	// =========================================================================
	struct AudioSourceProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<AudioSourceComponent>();
		}
	};
}
