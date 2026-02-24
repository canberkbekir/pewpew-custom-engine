#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/BodyFilter.h>

#include "CollisionShapeCache.h"
#include "PhysicsLayers.h"

#include <functional>
#include <vector>
#include <unordered_set>
#include <mutex>

namespace CB
{
	// Broad-phase layers
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr uint32_t NUM_LAYERS = 2;
	}

	// Object layers — 16 user-facing layers mapped to 2 broadphase layers.
	// Layer indices match PhysicsLayers::GetLayerName().
	namespace ObjectLayers
	{
		static constexpr uint32_t NUM_LAYERS = PhysicsLayers::NUM_LAYERS;
	}

	struct CollisionCallback
	{
		UUID EntityA;
		UUID EntityB;
		Vector3 ContactPoint;
		Vector3 ContactNormal;
	};

	struct RaycastHit
	{
		UUID EntityUUID;
		Vector3 Point;
		Vector3 Normal;
		float Fraction = 0.0f;
		float Distance = 0.0f;
	};

	class PhysicsWorld : public JPH::ContactListener
	{
	public:
		// Global Jolt init/shutdown - call once from Application
		static void InitJoltGlobals();
		static void ShutdownJoltGlobals();

		PhysicsWorld();
		~PhysicsWorld();

		void Init();
		void Shutdown();
		void Step(Timestep ts);

		void SetGravity(const Vector3& gravity);
		Vector3 GetGravity() const;

		JPH::BodyInterface& GetBodyInterface();
		JPH::PhysicsSystem& GetPhysicsSystem() { return m_PhysicsSystem; }
		const JPH::PhysicsSystem& GetPhysicsSystem() const { return m_PhysicsSystem; }

		CollisionShapeCache& GetShapeCache() { return m_ShapeCache; }

		using CollisionCallbackFn = std::function<void(const CollisionCallback&)>;
		void SetCollisionBeginCallback(CollisionCallbackFn fn) { m_CollisionBeginCallback = std::move(fn); }
		void SetCollisionEndCallback(CollisionCallbackFn fn) { m_CollisionEndCallback = std::move(fn); }
		// Trigger callbacks fire only when at least one body has IsSensor=true
		void SetTriggerEnterCallback(CollisionCallbackFn fn) { m_TriggerEnterCallback = std::move(fn); }
		void SetTriggerExitCallback(CollisionCallbackFn fn) { m_TriggerExitCallback = std::move(fn); }

		// Map Jolt BodyID -> Entity UUID for collision callback lookup
		void RegisterBodyEntity(JPH::BodyID bodyID, UUID entityUUID);
		void UnregisterBody(JPH::BodyID bodyID);
		UUID GetEntityFromBody(JPH::BodyID bodyID) const;
		JPH::BodyID GetBodyFromEntity(UUID entityUUID) const;

		// Raycasting
		bool Raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
			RaycastHit& outHit, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;
		std::vector<RaycastHit> RaycastAll(const Vector3& origin, const Vector3& direction,
			float maxDistance, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		// Overlap queries — return all entities whose physics shape overlaps the test volume
		std::vector<RaycastHit> OverlapSphere(const Vector3& center, float radius,
			uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;
		std::vector<RaycastHit> OverlapBox(const Vector3& center, const Vector3& halfExtents,
			uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		// Sweep casts — cast a shape along a ray, return first hit
		bool SphereCast(const Vector3& origin, const Vector3& dir, float radius, float maxDist,
			RaycastHit& outHit, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;
		bool BoxCast(const Vector3& origin, const Vector3& dir, const Vector3& halfExtents, float maxDist,
			RaycastHit& outHit, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		std::vector<RaycastHit> SphereCastAll(const Vector3& origin, const Vector3& dir, float radius,
			float maxDist, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;
		std::vector<RaycastHit> BoxCastAll(const Vector3& origin, const Vector3& dir, const Vector3& halfExtents,
			float maxDist, uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		// Overlap capsule (Y-axis aligned)
		std::vector<RaycastHit> OverlapCapsule(const Vector3& center, float radius, float halfHeight,
			uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		// Overlap capsule oriented along the segment p0→p1 with the given radius
		std::vector<RaycastHit> OverlapCapsuleSegment(const Vector3& p0, const Vector3& p1, float radius,
			uint16_t layerMask = PhysicsLayers::AllLayers,
			UUID ignoreEntity = UUID()) const;

		// JPH::ContactListener interface
		JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2,
			JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
		void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
			const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
		void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

		static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;

	private:
		JPH::PhysicsSystem m_PhysicsSystem;
		Scope<JPH::TempAllocatorImpl> m_TempAllocator;
		Scope<JPH::JobSystemThreadPool> m_JobSystem;

		Scope<JPH::BroadPhaseLayerInterfaceTable> m_BroadPhaseLayerInterface;
		Scope<JPH::ObjectVsBroadPhaseLayerFilterTable> m_ObjectVsBroadPhaseLayerFilter;
		Scope<JPH::ObjectLayerPairFilterTable> m_ObjectLayerPairFilter;

		float m_Accumulator = 0.0f;
		static constexpr int MAX_STEPS_PER_FRAME = 4;

		std::unordered_map<uint32_t, UUID> m_BodyToEntityMap;

		CollisionCallbackFn m_CollisionBeginCallback;
		CollisionCallbackFn m_CollisionEndCallback;
		CollisionCallbackFn m_TriggerEnterCallback;
		CollisionCallbackFn m_TriggerExitCallback;

		CollisionShapeCache m_ShapeCache;

		// Tracks active sensor (trigger) contact pairs so OnContactRemoved can
		// route correctly without needing a BodyLockRead (which deadlocks in Jolt).
		// Key = canonical pair key: min(bodyA,bodyB) | (max(bodyA,bodyB) << 32)
		std::unordered_set<uint64_t> m_SensorPairs;
		std::mutex m_SensorPairsMutex;

		bool m_Initialized = false;
	};
}
