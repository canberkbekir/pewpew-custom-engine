#include "cbpch.h"
#include "PhysicsSystem.h"

#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"
#include "CBEngine/Physics/CollisionShapeCache.h"
#include "CBEngine/Systems/DestructionSystem.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace CB
{
	static PhysicsWorld* s_PhysicsWorld = nullptr;

	static JPH::EMotionType ToJoltMotionType(BodyType type)
	{
		switch (type)
		{
		case BodyType::Static:    return JPH::EMotionType::Static;
		case BodyType::Dynamic:   return JPH::EMotionType::Dynamic;
		case BodyType::Kinematic: return JPH::EMotionType::Kinematic;
		}
		return JPH::EMotionType::Dynamic;
	}

	static JPH::ObjectLayer ToJoltObjectLayer(BodyType type)
	{
		return type == BodyType::Static ? ObjectLayers::NON_MOVING : ObjectLayers::MOVING;
	}

	static JPH::RefConst<JPH::Shape> CreateJoltShape(ColliderComponent& collider,
		Scene* scene = nullptr, entt::entity entity = entt::null)
	{
		JPH::RefConst<JPH::Shape> shape;

		switch (collider.Shape)
		{
		case ColliderShape::Box:
			shape = new JPH::BoxShape(JPH::Vec3(collider.HalfExtents.x, collider.HalfExtents.y, collider.HalfExtents.z));
			break;
		case ColliderShape::Sphere:
			shape = new JPH::SphereShape(collider.Radius);
			break;
		case ColliderShape::Capsule:
			shape = new JPH::CapsuleShape(collider.CapsuleHalfHeight, collider.CapsuleRadius);
			break;
		case ColliderShape::VoxelCompound:
		{
			// Try to generate compound shape from VoxelRendererComponent grid
			bool generated = false;
			if (scene && entity != entt::null &&
				scene->GetRegistry().all_of<VoxelRendererComponent>(entity))
			{
				auto& voxelRenderer = scene->GetRegistry().get<VoxelRendererComponent>(entity);
				UUID vmeshUUID = voxelRenderer.VoxelMeshUUID;

				// Check cache first
				auto& cache = CollisionShapeCache::Get();
				if (vmeshUUID.IsValid() && cache.Has(vmeshUUID))
				{
					shape = cache.Get(vmeshUUID);
					generated = (shape != nullptr);
				}
				else if (vmeshUUID.IsValid())
				{
					// Load VoxelMeshAsset and generate shape from grid
					auto vmeshAsset = AssetManager::GetAsset<VoxelMeshAsset>(vmeshUUID);
					if (vmeshAsset && vmeshAsset->VoxelCount > 0)
					{
						shape = VoxelCollisionShapeGenerator::GenerateFromGrid(vmeshAsset->GridData);
						if (shape)
						{
							cache.Store(vmeshUUID, shape);
							generated = true;
						}
					}
				}
			}

			// Fallback to box if voxel shape generation failed
			if (!generated)
				shape = new JPH::BoxShape(JPH::Vec3(collider.HalfExtents.x, collider.HalfExtents.y, collider.HalfExtents.z));
			break;
		}
		}

		// Apply offset if non-zero
		if (glm::length(collider.Offset) > 0.001f)
		{
			shape = new JPH::OffsetCenterOfMassShape(shape,
				JPH::Vec3(collider.Offset.x, collider.Offset.y, collider.Offset.z));
		}

		collider.RuntimeShape = shape;
		collider.ShapeDirty = false;
		return shape;
	}

	void PhysicsSystem::Init(Scene* scene)
	{
		if (s_PhysicsWorld)
		{
			CB_CORE_WARN("PhysicsSystem already initialized");
			return;
		}

		s_PhysicsWorld = new PhysicsWorld();
		s_PhysicsWorld->Init();

		// Set up collision callbacks with impact-based destruction
		s_PhysicsWorld->SetCollisionBeginCallback([scene](const CollisionCallback& cb)
		{
			CB_CORE_TRACE("Collision begin: Entity {0} <-> Entity {1}", (uint64_t)cb.EntityA, (uint64_t)cb.EntityB);

			// Check relative velocity for destruction threshold
			// For high-speed impacts on voxel entities, queue destruction
			auto& bodyInterface = s_PhysicsWorld->GetBodyInterface();

			auto checkDestructible = [&](UUID entityUUID, UUID otherUUID)
			{
				Entity entity = scene->GetEntityByUUID(entityUUID);
				if (!entity || !entity.HasComponent<VoxelRendererComponent>())
					return;
				if (!entity.HasComponent<RigidBodyComponent>())
					return;

				Entity otherEntity = scene->GetEntityByUUID(otherUUID);
				if (!otherEntity || !otherEntity.HasComponent<RigidBodyComponent>())
					return;

				auto& otherRB = otherEntity.GetComponent<RigidBodyComponent>();
				if (!otherRB.BodyCreated)
					return;

				// Get relative velocity at contact point
				JPH::Vec3 vel = bodyInterface.GetLinearVelocity(otherRB.RuntimeBodyID);
				float speed = vel.Length();

				// Destruction threshold: 10 m/s impact speed
				constexpr float DESTRUCTION_THRESHOLD = 10.0f;
				if (speed >= DESTRUCTION_THRESHOLD)
				{
					DestructionRequest request;
					request.TargetEntity = entityUUID;
					request.ImpactPoint = cb.ContactPoint;
					request.DamageRadius = 0.5f + speed * 0.05f; // Scale radius with impact speed
					request.ImpactForce = Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
					DestructionSystem::RequestDestruction(request);
				}
			};

			checkDestructible(cb.EntityA, cb.EntityB);
			checkDestructible(cb.EntityB, cb.EntityA);
		});

		s_PhysicsWorld->SetCollisionEndCallback([](const CollisionCallback& cb)
		{
			CB_CORE_TRACE("Collision end: Entity {0} <-> Entity {1}", (uint64_t)cb.EntityA, (uint64_t)cb.EntityB);
		});

		CB_CORE_INFO("PhysicsSystem initialized");
	}

	void PhysicsSystem::Shutdown()
	{
		if (!s_PhysicsWorld)
			return;

		CollisionShapeCache::Get().Clear();

		s_PhysicsWorld->Shutdown();
		delete s_PhysicsWorld;
		s_PhysicsWorld = nullptr;

		CB_CORE_INFO("PhysicsSystem shut down");
	}

	void PhysicsSystem::OnUpdate(Scene* scene, Timestep ts)
	{
		if (!s_PhysicsWorld)
			return;

		SyncToPhysics(scene);
		s_PhysicsWorld->Step(ts);
		SyncFromPhysics(scene);
	}

	void PhysicsSystem::SyncToPhysics(Scene* scene)
	{
		auto view = scene->GetRegistry().view<RigidBodyComponent, ColliderComponent, TransformComponent>();

		for (auto entity : view)
		{
			auto& rb = view.get<RigidBodyComponent>(entity);
			auto& collider = view.get<ColliderComponent>(entity);

			// Create body if not yet created
			if (!rb.BodyCreated)
			{
				CreateBody(scene, entity);
				continue;
			}

			// Update kinematic targets
			if (rb.Type == BodyType::Kinematic && rb.BodyCreated)
			{
				auto& transform = view.get<TransformComponent>(entity);
				Vector3 pos = transform.GetWorldPosition();
				glm::quat rotation = glm::quat(transform.Rotation);

				s_PhysicsWorld->GetBodyInterface().MoveKinematic(
					rb.RuntimeBodyID,
					JPH::RVec3(pos.x, pos.y, pos.z),
					JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
					PhysicsWorld::FIXED_TIMESTEP);
			}

			// Rebuild shape if dirty
			if (collider.ShapeDirty && rb.BodyCreated)
			{
				JPH::RefConst<JPH::Shape> newShape = CreateJoltShape(collider, scene, entity);
				s_PhysicsWorld->GetBodyInterface().SetShape(
					rb.RuntimeBodyID, newShape, false, JPH::EActivation::Activate);
			}
		}
	}

	void PhysicsSystem::SyncFromPhysics(Scene* scene)
	{
		auto view = scene->GetRegistry().view<RigidBodyComponent, TransformComponent>();

		for (auto entity : view)
		{
			auto& rb = view.get<RigidBodyComponent>(entity);
			if (!rb.BodyCreated || rb.Type != BodyType::Dynamic)
				continue;

			auto& bodyInterface = s_PhysicsWorld->GetBodyInterface();
			if (!bodyInterface.IsActive(rb.RuntimeBodyID))
				continue;

			auto& transform = view.get<TransformComponent>(entity);

			JPH::RVec3 position = bodyInterface.GetCenterOfMassPosition(rb.RuntimeBodyID);
			JPH::Quat rotation = bodyInterface.GetRotation(rb.RuntimeBodyID);

			// Write back to transform
			if (transform.HasParent())
			{
				// For parented entities, convert world position to local
				// This is a simplification - proper inverse parent transform would be needed
				transform.Position = Vector3(
					static_cast<float>(position.GetX()),
					static_cast<float>(position.GetY()),
					static_cast<float>(position.GetZ()));
			}
			else
			{
				transform.Position = Vector3(
					static_cast<float>(position.GetX()),
					static_cast<float>(position.GetY()),
					static_cast<float>(position.GetZ()));
			}

			// Convert quaternion to euler
			glm::quat glmQuat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
			transform.Rotation = glm::eulerAngles(glmQuat);
			transform.Dirty = true;
		}
	}

	void PhysicsSystem::CreateBody(Scene* scene, entt::entity entity)
	{
		auto& rb = scene->GetRegistry().get<RigidBodyComponent>(entity);
		auto& collider = scene->GetRegistry().get<ColliderComponent>(entity);
		auto& transform = scene->GetRegistry().get<TransformComponent>(entity);
		auto& id = scene->GetRegistry().get<IDComponent>(entity);

		// Create Jolt shape
		JPH::RefConst<JPH::Shape> shape = CreateJoltShape(collider, scene, entity);

		// Get world position and rotation
		Vector3 worldPos = transform.GetWorldPosition();
		glm::quat rotation = glm::quat(transform.Rotation);

		JPH::BodyCreationSettings bodySettings(
			shape,
			JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
			JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
			ToJoltMotionType(rb.Type),
			ToJoltObjectLayer(rb.Type));

		// Apply physics properties
		if (rb.Type == BodyType::Dynamic)
		{
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			bodySettings.mMassPropertiesOverride.mMass = rb.Mass;
		}

		bodySettings.mLinearDamping = rb.LinearDamping;
		bodySettings.mAngularDamping = rb.AngularDamping;
		bodySettings.mFriction = rb.Friction;
		bodySettings.mRestitution = rb.Restitution;
		bodySettings.mGravityFactor = rb.UseGravity ? 1.0f : 0.0f;
		bodySettings.mIsSensor = collider.IsTrigger;

		// Create body in Jolt
		JPH::Body* body = s_PhysicsWorld->GetBodyInterface().CreateBody(bodySettings);
		if (!body)
		{
			CB_CORE_ERROR("Failed to create physics body for entity {0}", (uint64_t)id.ID);
			return;
		}

		rb.RuntimeBodyID = body->GetID();
		rb.BodyCreated = true;

		// Register for collision callbacks
		s_PhysicsWorld->RegisterBodyEntity(rb.RuntimeBodyID, id.ID);

		// Add to physics world
		s_PhysicsWorld->GetBodyInterface().AddBody(rb.RuntimeBodyID,
			rb.Type == BodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

		CB_CORE_TRACE("Created physics body for entity {0} (BodyID: {1})",
			(uint64_t)id.ID, rb.RuntimeBodyID.GetIndexAndSequenceNumber());
	}

	void PhysicsSystem::DestroyBody(entt::entity entity)
	{
		// This will be called when entities with physics bodies are destroyed
		// For now, handled via Scene lifecycle
	}
}
