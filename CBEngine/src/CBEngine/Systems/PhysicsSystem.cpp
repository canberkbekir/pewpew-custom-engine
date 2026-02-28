#include "cbpch.h"
#include "PhysicsSystem.h"

#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "CBEngine/Physics/PhysicsLayers.h"
#include "CBEngine/Physics/VoxelCollisionShapeGenerator.h"
#include "CBEngine/Physics/CollisionShapeCache.h"
#include "CBEngine/Systems/DestructionSystem.h"
#include "CBEngine/Systems/VoxelDestructionSystem.h"
#include "CBEngine/Debug/Instrumentor.h"
#include "CBEngine/Components/DestructibleVoxelComponent.h"
#include "CBEngine/Scripting/ScriptEngine.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/ProcessedMeshAsset.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <mutex>

namespace CB
{
	// Thread-safe collision event queue
	static std::mutex s_CollisionMutex;
	static std::vector<CollisionCallback> s_PendingCollisionBegin;
	static std::vector<CollisionCallback> s_PendingCollisionEnd;
	static std::vector<CollisionCallback> s_PendingTriggerEnter;
	static std::vector<CollisionCallback> s_PendingTriggerExit;

	static JPH::EMotionType ToJoltMotionType(BodyType type)
	{
		switch (type) {
		case BodyType::Static: return JPH::EMotionType::Static;
		case BodyType::Dynamic: return JPH::EMotionType::Dynamic;
		case BodyType::Kinematic: return JPH::EMotionType::Kinematic;
		}
		return JPH::EMotionType::Dynamic;
	}

	static JPH::ObjectLayer ToJoltObjectLayer(uint8_t entityLayer) { return entityLayer; }

	static JPH::RefConst<JPH::Shape> CreateJoltShape(ColliderComponent& collider,
	                                                 PhysicsWorld* world,
	                                                 const Vector3& worldScale = Vector3(1.0f),
	                                                 Scene* scene = nullptr,entt::entity entity = entt::null)
	{
		// Apply entity scale to collider dimensions (collider values are in local space)
		auto absScale = Vector3(std::abs(worldScale.x), std::abs(worldScale.y), std::abs(worldScale.z));

		// Respect pre-set RuntimeShape (e.g. fragment/collapsed-chunk VoxelCompound shapes).
		// This also handles the case where VoxelDestructionSystem regenerated the shape
		// from the modified grid and set RuntimeShape + ShapeDirty=true: we should use
		// the pre-generated shape rather than trying to load from VoxelMeshUUID (which
		// may not be set for chunks/fragments, causing a fallback to a tiny Box).
		if (collider.RuntimeShape != nullptr &&
			(!collider.ShapeDirty || collider.Shape == ColliderShape::VoxelCompound)) {
			bool needsScale = std::abs(absScale.x - 1.0f) > 0.001f ||
			std::abs(absScale.y - 1.0f) > 0.001f ||
			std::abs(absScale.z - 1.0f) > 0.001f;
			JPH::RefConst<JPH::Shape> result = collider.RuntimeShape;
			if (needsScale)
				result = new JPH::ScaledShape(result,
				                              JPH::Vec3(absScale.x, absScale.y, absScale.z));
			collider.ShapeDirty = false;
			return result;
		}

		JPH::RefConst<JPH::Shape> shape;

		switch (collider.Shape) {
		case ColliderShape::Box:
			{
				Vector3 scaledHalf = collider.HalfExtents * absScale;
				shape = new JPH::BoxShape(JPH::Vec3(scaledHalf.x, scaledHalf.y, scaledHalf.z));
				break;
			}
		case ColliderShape::Sphere:
			{
				float maxScale = std::max({absScale.x, absScale.y, absScale.z});
				shape = new JPH::SphereShape(collider.Radius * maxScale);
				break;
			}
		case ColliderShape::Capsule:
			{
				float radiusScale = std::max(absScale.x, absScale.z);
				shape = new JPH::CapsuleShape(collider.CapsuleHalfHeight * absScale.y,
				                              collider.CapsuleRadius * radiusScale);
				break;
			}
		case ColliderShape::VoxelCompound:
			{
				// Try to generate compound shape from VoxelRendererComponent grid
				bool generated = false;
				if (scene && entity != entt::null &&
					scene->GetRegistry().all_of<VoxelRendererComponent>(entity)) {
					auto& voxelRenderer = scene->GetRegistry().get<VoxelRendererComponent>(entity);
					UUID vmeshUUID = voxelRenderer.VoxelMeshUUID;

					// Check cache first
					auto& cache = world->GetShapeCache();
					if (vmeshUUID.IsValid() && cache.Has(vmeshUUID)) {
						shape = cache.Get(vmeshUUID);
						generated = (shape != nullptr);
					}
					else if (vmeshUUID.IsValid()) {
						// Load VoxelMeshAsset and generate shape from grid
						auto vmeshAsset = AssetManager::GetAsset<VoxelMeshAsset>(vmeshUUID);
						if (vmeshAsset && vmeshAsset->VoxelCount > 0) {
							shape = VoxelCollisionShapeGenerator::GenerateFromGrid(vmeshAsset->GridData);
							if (shape) {
								cache.Store(vmeshUUID, shape);
								generated = true;
							}
						}
					}
				}

				// Fallback to box if voxel shape generation failed
				if (!generated) {
					Vector3 scaledHalf = collider.HalfExtents * absScale;
					shape = new JPH::BoxShape(JPH::Vec3(scaledHalf.x, scaledHalf.y, scaledHalf.z));
				}
				else {
					// VoxelCompound shapes are in mesh local space, apply entity scale
					bool needsScale = std::abs(absScale.x - 1.0f) > 0.001f ||
					std::abs(absScale.y - 1.0f) > 0.001f ||
					std::abs(absScale.z - 1.0f) > 0.001f;
					if (needsScale) {
						shape = new JPH::ScaledShape(shape,
						                             JPH::Vec3(absScale.x, absScale.y, absScale.z));
					}
				}
				break;
			}
		}

		// Apply geometric offset if non-zero (scaled by entity transform)
		Vector3 scaledOffset = collider.Offset * absScale;
		if (glm::length(scaledOffset) > 0.001f) {
			shape = new JPH::RotatedTranslatedShape(
				JPH::Vec3(scaledOffset.x, scaledOffset.y, scaledOffset.z),
				JPH::Quat::sIdentity(), shape);
		}

		collider.RuntimeShape = shape;
		collider.ShapeDirty = false;
		return shape;
	}

	void PhysicsSystem::Init(Scene* scene,PhysicsWorld* world)
	{
		// 1.7: Set up thread-safe collision callbacks
		world->SetCollisionBeginCallback([](const CollisionCallback& cb)
		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			s_PendingCollisionBegin.push_back(cb);
		});

		world->SetCollisionEndCallback([](const CollisionCallback& cb)
		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			s_PendingCollisionEnd.push_back(cb);
		});

		world->SetTriggerEnterCallback([](const CollisionCallback& cb)
		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			s_PendingTriggerEnter.push_back(cb);
		});

		world->SetTriggerExitCallback([](const CollisionCallback& cb)
		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			s_PendingTriggerExit.push_back(cb);
		});

		CB_CORE_INFO("PhysicsSystem initialized");
	}

	void PhysicsSystem::Shutdown(PhysicsWorld* world)
	{
		if (!world)
			return;

		world->GetShapeCache().Clear();

		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			s_PendingCollisionBegin.clear();
			s_PendingCollisionEnd.clear();
			s_PendingTriggerEnter.clear();
			s_PendingTriggerExit.clear();
		}

		CB_CORE_INFO("PhysicsSystem shut down");
	}

	// 1.7: Flush collision events on the main thread
	static void FlushCollisionEvents(Scene* scene,PhysicsWorld* world)
	{
		std::vector<CollisionCallback> beginEvents;
		std::vector<CollisionCallback> endEvents;

		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			beginEvents = std::move(s_PendingCollisionBegin);
			s_PendingCollisionBegin.clear();
			endEvents = std::move(s_PendingCollisionEnd);
			s_PendingCollisionEnd.clear();
		}

		auto& bodyInterface = world->GetBodyInterface();

		for (const auto& cb : beginEvents) {
			CB_CORE_TRACE("Collision begin: Entity {0} <-> Entity {1}", static_cast<uint64_t>(cb.EntityA),
			              static_cast<uint64_t>(cb.EntityB));

			// Check relative velocity for destruction threshold
			auto checkDestructible = [&](UUID entityUUID,UUID otherUUID)
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

				JPH::Vec3 vel = bodyInterface.GetLinearVelocity(otherRB.RuntimeBodyID);
				float speed = vel.Length();

				constexpr float DESTRUCTION_THRESHOLD = 10.0f;
				if (speed >= DESTRUCTION_THRESHOLD) {
					DestructionRequest request;
					request.TargetEntity = entityUUID;
					request.ImpactPoint = cb.ContactPoint;
					request.DamageRadius = 0.5f + speed * 0.05f;
					request.ImpactForce = Vector3(vel.GetX(), vel.GetY(), vel.GetZ());
					DestructionSystem::RequestDestruction(request);
				}
			};

			checkDestructible(cb.EntityA, cb.EntityB);
			checkDestructible(cb.EntityB, cb.EntityA);

			// Voxel auto-impact damage — queue VoxelDamageEvent for DestructibleVoxelComponent entities
			auto checkVoxelImpact = [&](UUID entityUUID,UUID otherUUID)
			{
				Entity entity = scene->GetEntityByUUID(entityUUID);
				if (!entity || !entity.HasComponent<DestructibleVoxelComponent>())
					return;

				auto& dvComp = entity.GetComponent<DestructibleVoxelComponent>();
				if (!dvComp.Enabled || !dvComp.ReceivePhysicsImpact)
					return;

				if (!entity.HasComponent<VoxelRendererComponent>())
					return;

				Entity otherEntity = scene->GetEntityByUUID(otherUUID);
				if (!otherEntity || !otherEntity.HasComponent<RigidBodyComponent>())
					return;

				auto& otherRB = otherEntity.GetComponent<RigidBodyComponent>();
				if (!otherRB.BodyCreated)
					return;

				// Compute relative velocity as impact force estimate
				JPH::Vec3 otherVel = bodyInterface.GetLinearVelocity(otherRB.RuntimeBodyID);
				float impactSpeed = otherVel.Length();

				if (impactSpeed < dvComp.PhysicsImpactThreshold)
					return;

				// Transform contact point from world space to grid-local space
				auto& vr = entity.GetComponent<VoxelRendererComponent>();
				if (!vr.VoxelMeshUUID.IsValid())
					return;

				auto vmesh = AssetManager::GetAsset<VoxelMeshAsset>(vr.VoxelMeshUUID);
				if (!vmesh)
					return;

				// Get the grid (use modified copy if available)
				uint64_t uuid = entity.GetUUID();
				const voxelizer::VoxelGrid* grid = &vmesh->GridData;

				// Transform contact point to entity local space
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::mat4 worldInv = glm::inverse(transform.WorldMatrix);
				glm::vec3 contactWorld(cb.ContactPoint.x, cb.ContactPoint.y, cb.ContactPoint.z);
				auto contactLocal = glm::vec3(worldInv * glm::vec4(contactWorld, 1.0f));

				glm::ivec3 gridCoord = grid->WorldToVoxel(contactLocal);

				// Clamp and verify the coord is valid and filled
				if (!grid->IsValidCoord(gridCoord) || !grid->IsFilled(gridCoord)) {
					// Try nearby voxels — contact point may be on the surface edge
					bool found = false;
					static const glm::ivec3 offsets[] = {
						{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
					};
					for (const auto& off : offsets) {
						glm::ivec3 test = gridCoord + off;
						if (grid->IsValidCoord(test) && grid->IsFilled(test)) {
							gridCoord = test;
							found = true;
							break;
						}
					}
					if (!found)
						return;
				}

				// Queue damage — scale raw amount by speed and multiplier
				float rawDamage = impactSpeed * dvComp.PhysicsImpactMultiplier;

				VoxelDamageEvent dmgEvent;
				dmgEvent.GridCoord = gridCoord;
				dmgEvent.EntityUUID = uuid;
				dmgEvent.Type = VoxelDamageType::Impact;
				dmgEvent.RawAmount = rawDamage;
				dmgEvent.WorldOrigin = contactWorld;
				dmgEvent.Direction = glm::vec3(otherVel.GetX(), otherVel.GetY(), otherVel.GetZ());
				dmgEvent.InstigatorUUID = static_cast<uint64_t>(otherUUID);
				VoxelDestructionSystem::QueueDamage(dmgEvent);
			};

			checkVoxelImpact(cb.EntityA, cb.EntityB);
			checkVoxelImpact(cb.EntityB, cb.EntityA);

			// Script collision callbacks for both entities
			Entity entityA = scene->GetEntityByUUID(cb.EntityA);
			Entity entityB = scene->GetEntityByUUID(cb.EntityB);
			if (entityA && entityB) {
				ScriptEngine::OnCollision(scene, entityA, entityB, cb.ContactPoint, cb.ContactNormal);
				ScriptEngine::OnCollision(scene, entityB, entityA, cb.ContactPoint, -cb.ContactNormal);
			}
		}

		for (const auto& cb : endEvents) {
			CB_CORE_TRACE("Collision end: Entity {0} <-> Entity {1}", static_cast<uint64_t>(cb.EntityA),
			              static_cast<uint64_t>(cb.EntityB));

			// Script collision end callbacks for both entities
			Entity entityA = scene->GetEntityByUUID(cb.EntityA);
			Entity entityB = scene->GetEntityByUUID(cb.EntityB);
			if (entityA && entityB) {
				ScriptEngine::OnCollisionEnd(scene, entityA, entityB);
				ScriptEngine::OnCollisionEnd(scene, entityB, entityA);
			}
		}
	}

	static void FlushTriggerEvents(Scene* scene,PhysicsWorld* world)
	{
		std::vector<CollisionCallback> enterEvents;
		std::vector<CollisionCallback> exitEvents;

		{
			std::lock_guard<std::mutex> lock(s_CollisionMutex);
			enterEvents = std::move(s_PendingTriggerEnter);
			s_PendingTriggerEnter.clear();
			exitEvents = std::move(s_PendingTriggerExit);
			s_PendingTriggerExit.clear();
		}

		for (const auto& cb : enterEvents) {
			Entity entityA = scene->GetEntityByUUID(cb.EntityA);
			Entity entityB = scene->GetEntityByUUID(cb.EntityB);
			if (entityA && entityB) {
				ScriptEngine::OnTriggerEnter(scene, entityA, entityB, cb.ContactPoint, cb.ContactNormal);
				ScriptEngine::OnTriggerEnter(scene, entityB, entityA, cb.ContactPoint, -cb.ContactNormal);
			}
		}

		for (const auto& cb : exitEvents) {
			Entity entityA = scene->GetEntityByUUID(cb.EntityA);
			Entity entityB = scene->GetEntityByUUID(cb.EntityB);
			if (entityA && entityB) {
				ScriptEngine::OnTriggerExit(scene, entityA, entityB);
				ScriptEngine::OnTriggerExit(scene, entityB, entityA);
			}
		}
	}

	void PhysicsSystem::OnUpdate(Scene* scene,PhysicsWorld* world,Timestep ts)
	{
		CB_PROFILE_SCOPE_CAT("PhysicsSystem::OnUpdate", "Physics");
		if (!world)
			return;

		// 1.7: Flush collision events at the start of the update
		{
			CB_PROFILE_SCOPE_CAT("PhysicsSystem::FlushEvents", "Physics");
			FlushCollisionEvents(scene, world);
			FlushTriggerEvents(scene, world);
		}

		{
			CB_PROFILE_SCOPE_CAT("PhysicsSystem::SyncStaticColliders", "Physics");
			SyncStaticColliders(scene, world);
		}
		{
			CB_PROFILE_SCOPE_CAT("PhysicsSystem::SyncToPhysics", "Physics");
			SyncToPhysics(scene, world);
		}
		{
			CB_PROFILE_SCOPE_CAT("PhysicsWorld::Step", "Physics");
			world->Step(ts);
		}
		{
			CB_PROFILE_SCOPE_CAT("PhysicsSystem::SyncFromPhysics", "Physics");
			SyncFromPhysics(scene, world);
		}
	}

	void PhysicsSystem::SyncToPhysics(Scene* scene,PhysicsWorld* world)
	{
		auto view = scene->GetRegistry().view<RigidBodyComponent, ColliderComponent, TransformComponent>();

		for (auto entity : view) {
			auto& rb = view.get<RigidBodyComponent>(entity);
			auto& collider = view.get<ColliderComponent>(entity);

			// Create body if not yet created
			if (!rb.BodyCreated) {
				CreateBody(scene, world, entity);
				continue;
			}

			// Update kinematic targets
			if (rb.Type == BodyType::Kinematic && rb.BodyCreated) {
				auto& transform = view.get<TransformComponent>(entity);
				Vector3 pos = transform.GetWorldPosition();
				auto rotation = glm::quat(transform.Rotation);

				world->GetBodyInterface().MoveKinematic(
					rb.RuntimeBodyID,
					JPH::RVec3(pos.x, pos.y, pos.z),
					JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
					PhysicsWorld::FIXED_TIMESTEP);
			}

			// Rebuild shape if dirty
			if (collider.ShapeDirty && rb.BodyCreated) {
				auto& transform = view.get<TransformComponent>(entity);
				Vector3 worldScale = transform.Scale;
				if (transform.HasParent()) {
					worldScale = Vector3(
						glm::length(Vector3(transform.WorldMatrix[0])),
						glm::length(Vector3(transform.WorldMatrix[1])),
						glm::length(Vector3(transform.WorldMatrix[2])));
				}
				JPH::RefConst<JPH::Shape> newShape = CreateJoltShape(collider, world, worldScale, scene, entity);
				world->GetBodyInterface().SetShape(
					rb.RuntimeBodyID, newShape, false, JPH::EActivation::Activate);
			}
		}
	}

	// 1.6 Fix: Proper parent transform writeback
	void PhysicsSystem::SyncFromPhysics(Scene* scene,PhysicsWorld* world)
	{
		auto view = scene->GetRegistry().view<RigidBodyComponent, TransformComponent>();

		for (auto entity : view) {
			auto& rb = view.get<RigidBodyComponent>(entity);
			if (!rb.BodyCreated || rb.Type != BodyType::Dynamic)
				continue;

			auto& bodyInterface = world->GetBodyInterface();
			if (!bodyInterface.IsActive(rb.RuntimeBodyID))
				continue;

			auto& transform = view.get<TransformComponent>(entity);

			JPH::RVec3 position = bodyInterface.GetPosition(rb.RuntimeBodyID);
			JPH::Quat rotation = bodyInterface.GetRotation(rb.RuntimeBodyID);

			Vector3 worldPos(
				(position.GetX()),
				(position.GetY()),
				(position.GetZ()));

			glm::quat worldQuat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

			// When all rotation axes are frozen, the script owns rotation -- skip physics writeback
			bool allRotFrozen = rb.FreezeRotationX && rb.FreezeRotationY && rb.FreezeRotationZ;

			if (transform.HasParent()) {
				// Convert world position/rotation to local space using parent's world matrix
				Entity parentEntity = scene->GetEntityByUUID(transform.Parent);
				if (parentEntity) {
					Mat4 parentWorldMatrix = parentEntity.GetComponent<TransformComponent>().WorldMatrix;
					Mat4 invParent = glm::inverse(parentWorldMatrix);

					// Convert world position to local
					transform.Position = Vector3(invParent * Vector4(worldPos, 1.0f));

					// Convert world rotation to local (skip if script controls rotation)
					if (!allRotFrozen) {
						glm::quat parentWorldQuat = glm::quat_cast(parentWorldMatrix);
						glm::quat localQuat = glm::inverse(parentWorldQuat) * worldQuat;
						transform.Rotation = glm::eulerAngles(localQuat);
					}
				}
				else {
					transform.Position = worldPos;
					if (!allRotFrozen)
						transform.Rotation = glm::eulerAngles(worldQuat);
				}
			}
			else {
				transform.Position = worldPos;
				if (!allRotFrozen)
					transform.Rotation = glm::eulerAngles(worldQuat);
			}

			transform.Dirty = true;
		}
	}

	void PhysicsSystem::CreateBody(Scene* scene,PhysicsWorld* world,entt::entity entity)
	{
		auto& rb = scene->GetRegistry().get<RigidBodyComponent>(entity);
		auto& collider = scene->GetRegistry().get<ColliderComponent>(entity);
		auto& transform = scene->GetRegistry().get<TransformComponent>(entity);
		auto& id = scene->GetRegistry().get<IDComponent>(entity);

		// Extract world scale from transform for scaling collider dimensions
		Vector3 worldScale = transform.Scale;
		if (transform.HasParent()) {
			worldScale = Vector3(
				glm::length(Vector3(transform.WorldMatrix[0])),
				glm::length(Vector3(transform.WorldMatrix[1])),
				glm::length(Vector3(transform.WorldMatrix[2])));
		}

		// Create Jolt shape (scaled by entity world scale)
		JPH::RefConst<JPH::Shape> shape = CreateJoltShape(collider, world, worldScale, scene, entity);

		// Get world position and rotation
		Vector3 worldPos = transform.GetWorldPosition();
		auto rotation = glm::quat(transform.Rotation);

		JPH::BodyCreationSettings bodySettings(
			shape,
			JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
			JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
			ToJoltMotionType(rb.Type),
			ToJoltObjectLayer(id.Layer));

		// Apply physics properties
		if (rb.Type == BodyType::Dynamic) {
			bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			bodySettings.mMassPropertiesOverride.mMass = rb.Mass;
		}

		bodySettings.mLinearDamping = rb.LinearDamping;
		bodySettings.mAngularDamping = rb.AngularDamping;
		bodySettings.mFriction = rb.Friction;
		bodySettings.mRestitution = rb.Restitution;
		bodySettings.mGravityFactor = rb.UseGravity ? 1.0f : 0.0f;
		bodySettings.mIsSensor = collider.IsTrigger;

		// Freeze individual degrees of freedom
		if (rb.Type == BodyType::Dynamic || rb.Type == BodyType::Kinematic) {
			uint8_t dofs = static_cast<uint8_t>(JPH::EAllowedDOFs::All);
			if (rb.FreezePositionX) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationX);
			if (rb.FreezePositionY) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationY);
			if (rb.FreezePositionZ) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::TranslationZ);
			if (rb.FreezeRotationX) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationX);
			if (rb.FreezeRotationY) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationY);
			if (rb.FreezeRotationZ) dofs &= ~static_cast<uint8_t>(JPH::EAllowedDOFs::RotationZ);
			bodySettings.mAllowedDOFs = static_cast<JPH::EAllowedDOFs>(dofs);
		}

		// Create body in Jolt
		JPH::Body* body = world->GetBodyInterface().CreateBody(bodySettings);
		if (!body) {
			CB_CORE_ERROR("Failed to create physics body for entity {0}", static_cast<uint64_t>(id.ID));
			return;
		}

		rb.RuntimeBodyID = body->GetID();
		rb.BodyCreated = true;

		// Register for collision callbacks
		world->RegisterBodyEntity(rb.RuntimeBodyID, id.ID);

		// Add to physics world
		world->GetBodyInterface().AddBody(rb.RuntimeBodyID,
		                                  rb.Type == BodyType::Static
			                                  ? JPH::EActivation::DontActivate
			                                  : JPH::EActivation::Activate);

		CB_CORE_TRACE("Created physics body for entity {0} (BodyID: {1})",
		              static_cast<uint64_t>(id.ID), rb.RuntimeBodyID.GetIndexAndSequenceNumber());
	}

	void PhysicsSystem::SyncStaticColliders(Scene* scene,PhysicsWorld* world)
	{
		auto view = scene->GetRegistry().view<ColliderComponent, TransformComponent>();

		for (auto entity : view) {
			if (scene->GetRegistry().all_of<RigidBodyComponent>(entity))
				continue;

			auto& collider = view.get<ColliderComponent>(entity);
			auto& transform = view.get<TransformComponent>(entity);

			if (!collider.StaticBodyCreated) { CreateStaticColliderBody(scene, world, entity); }
			else {
				auto& bodyInterface = world->GetBodyInterface();

				// Update shape if dirty
				if (collider.ShapeDirty) {
					Vector3 worldScale = transform.Scale;
					if (transform.HasParent()) {
						worldScale = Vector3(
							glm::length(Vector3(transform.WorldMatrix[0])),
							glm::length(Vector3(transform.WorldMatrix[1])),
							glm::length(Vector3(transform.WorldMatrix[2])));
					}
					JPH::RefConst<JPH::Shape> newShape = CreateJoltShape(collider, world, worldScale, scene, entity);
					bodyInterface.SetShape(
						collider.StaticBodyID, newShape, false, JPH::EActivation::DontActivate);
				}

				// Sync position/rotation from transform to physics body every frame
				// This ensures child colliders follow their moving parent
				if (transform.HasParent()) {
					Vector3 worldPos = transform.GetWorldPosition();

					// Extract rotation from WorldMatrix, removing scale first
					Mat4 rotMat = transform.WorldMatrix;
					rotMat[0] = Vector4(glm::normalize(Vector3(rotMat[0])), 0.0f);
					rotMat[1] = Vector4(glm::normalize(Vector3(rotMat[1])), 0.0f);
					rotMat[2] = Vector4(glm::normalize(Vector3(rotMat[2])), 0.0f);
					glm::quat worldRot = glm::normalize(glm::quat_cast(rotMat));

					bodyInterface.SetPositionAndRotation(
						collider.StaticBodyID,
						JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
						JPH::Quat(worldRot.x, worldRot.y, worldRot.z, worldRot.w),
						JPH::EActivation::DontActivate);
				}
			}
		}
	}

	void PhysicsSystem::CreateStaticColliderBody(Scene* scene,PhysicsWorld* world,entt::entity entity)
	{
		auto& collider = scene->GetRegistry().get<ColliderComponent>(entity);
		auto& transform = scene->GetRegistry().get<TransformComponent>(entity);
		auto& id = scene->GetRegistry().get<IDComponent>(entity);

		Vector3 worldScale = transform.Scale;
		if (transform.HasParent()) {
			worldScale = Vector3(
				glm::length(Vector3(transform.WorldMatrix[0])),
				glm::length(Vector3(transform.WorldMatrix[1])),
				glm::length(Vector3(transform.WorldMatrix[2])));
		}

		JPH::RefConst<JPH::Shape> shape = CreateJoltShape(collider, world, worldScale, scene, entity);

		Vector3 worldPos = transform.GetWorldPosition();
		auto rotation = glm::quat(transform.Rotation);

		JPH::BodyCreationSettings bodySettings(
			shape,
			JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
			JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
			JPH::EMotionType::Static,
			ToJoltObjectLayer(id.Layer));

		bodySettings.mFriction = 0.5f;
		bodySettings.mRestitution = 0.3f;
		bodySettings.mIsSensor = collider.IsTrigger;

		JPH::Body* body = world->GetBodyInterface().CreateBody(bodySettings);
		if (!body) {
			CB_CORE_ERROR("Failed to create static collider body for entity {0}", static_cast<uint64_t>(id.ID));
			return;
		}

		collider.StaticBodyID = body->GetID();
		collider.StaticBodyCreated = true;

		world->RegisterBodyEntity(collider.StaticBodyID, id.ID);
		world->GetBodyInterface().AddBody(collider.StaticBodyID, JPH::EActivation::DontActivate);

		CB_CORE_TRACE("Created static collider body for entity {0} (BodyID: {1})",
		              static_cast<uint64_t>(id.ID), collider.StaticBodyID.GetIndexAndSequenceNumber());
	}

	// 1.5: Properly destroy physics bodies
	void PhysicsSystem::DestroyBody(PhysicsWorld* world,entt::entity entity,entt::registry& registry)
	{
		if (!world || !registry.all_of<RigidBodyComponent>(entity))
			return;

		auto& rb = registry.get<RigidBodyComponent>(entity);
		if (!rb.BodyCreated)
			return;

		auto& bodyInterface = world->GetBodyInterface();
		bodyInterface.RemoveBody(rb.RuntimeBodyID);
		bodyInterface.DestroyBody(rb.RuntimeBodyID);
		world->UnregisterBody(rb.RuntimeBodyID);

		rb.BodyCreated = false;
	}

	void PhysicsSystem::DestroyStaticBody(PhysicsWorld* world,entt::entity entity,entt::registry& registry)
	{
		if (!world || !registry.all_of<ColliderComponent>(entity))
			return;

		auto& collider = registry.get<ColliderComponent>(entity);
		if (!collider.StaticBodyCreated)
			return;

		auto& bodyInterface = world->GetBodyInterface();
		bodyInterface.RemoveBody(collider.StaticBodyID);
		bodyInterface.DestroyBody(collider.StaticBodyID);
		world->UnregisterBody(collider.StaticBodyID);

		collider.StaticBodyCreated = false;
	}
}