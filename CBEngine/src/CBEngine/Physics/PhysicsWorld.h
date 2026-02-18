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

#include "CollisionShapeCache.h"

#include <functional>

namespace CB
{
	// Broad-phase layers
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr uint32_t NUM_LAYERS = 2;
	}

	// Object layers
	namespace ObjectLayers
	{
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr uint32_t NUM_LAYERS = 2;
	}

	struct CollisionCallback
	{
		UUID EntityA;
		UUID EntityB;
		Vector3 ContactPoint;
		Vector3 ContactNormal;
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

		CollisionShapeCache& GetShapeCache() { return m_ShapeCache; }

		using CollisionCallbackFn = std::function<void(const CollisionCallback&)>;
		void SetCollisionBeginCallback(CollisionCallbackFn fn) { m_CollisionBeginCallback = std::move(fn); }
		void SetCollisionEndCallback(CollisionCallbackFn fn) { m_CollisionEndCallback = std::move(fn); }

		// Map Jolt BodyID -> Entity UUID for collision callback lookup
		void RegisterBodyEntity(JPH::BodyID bodyID, UUID entityUUID);
		void UnregisterBody(JPH::BodyID bodyID);

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

		CollisionShapeCache m_ShapeCache;

		bool m_Initialized = false;
	};
}
