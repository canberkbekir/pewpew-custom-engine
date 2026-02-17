#include "cbpch.h"
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

namespace CB
{
	static void JoltTraceImpl(const char* inFMT, ...)
	{
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);
		CB_CORE_INFO("[Jolt] {0}", buffer);
	}

#ifdef JPH_ENABLE_ASSERTS
	static bool JoltAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
	{
		CB_CORE_ERROR("[Jolt Assert] {0}:{1}: ({2}) {3}", inFile, inLine, inExpression, inMessage ? inMessage : "");
		return true; // trigger breakpoint
	}
#endif

	PhysicsWorld::PhysicsWorld()
	{
	}

	PhysicsWorld::~PhysicsWorld()
	{
		if (m_Initialized)
			Shutdown();
	}

	void PhysicsWorld::Init()
	{
		if (m_Initialized)
			return;

		// Register Jolt trace/assert handlers
		JPH::Trace = JoltTraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

		// Register types (must be called once)
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		// Allocators
		m_TempAllocator = CreateScope<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB
		m_JobSystem = CreateScope<JPH::JobSystemThreadPool>(
			JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
			std::max(1u, std::thread::hardware_concurrency() - 1));

		// Broad-phase layer mapping
		m_BroadPhaseLayerInterface = CreateScope<JPH::BroadPhaseLayerInterfaceTable>(
			ObjectLayers::NUM_LAYERS, BroadPhaseLayers::NUM_LAYERS);
		m_BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(ObjectLayers::NON_MOVING, BroadPhaseLayers::NON_MOVING);
		m_BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(ObjectLayers::MOVING, BroadPhaseLayers::MOVING);

		// Broad-phase vs object layer filter
		m_ObjectVsBroadPhaseLayerFilter = CreateScope<JPH::ObjectVsBroadPhaseLayerFilterTable>(
			*m_BroadPhaseLayerInterface, BroadPhaseLayers::NUM_LAYERS,
			*m_ObjectLayerPairFilter, ObjectLayers::NUM_LAYERS);

		// Object layer pair filter
		m_ObjectLayerPairFilter = CreateScope<JPH::ObjectLayerPairFilterTable>(ObjectLayers::NUM_LAYERS);
		m_ObjectLayerPairFilter->EnableCollision(ObjectLayers::NON_MOVING, ObjectLayers::MOVING);
		m_ObjectLayerPairFilter->EnableCollision(ObjectLayers::MOVING, ObjectLayers::MOVING);

		// Re-create broad-phase filter now that pair filter exists
		m_ObjectVsBroadPhaseLayerFilter = CreateScope<JPH::ObjectVsBroadPhaseLayerFilterTable>(
			*m_BroadPhaseLayerInterface, BroadPhaseLayers::NUM_LAYERS,
			*m_ObjectLayerPairFilter, ObjectLayers::NUM_LAYERS);

		// Initialize physics system
		const uint32_t maxBodies = 65536;
		const uint32_t numBodyMutexes = 0; // auto
		const uint32_t maxBodyPairs = 65536;
		const uint32_t maxContactConstraints = 10240;

		m_PhysicsSystem.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
			*m_BroadPhaseLayerInterface, *m_ObjectVsBroadPhaseLayerFilter, *m_ObjectLayerPairFilter);

		m_PhysicsSystem.SetContactListener(this);
		m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

		m_Initialized = true;
		CB_CORE_INFO("PhysicsWorld initialized");
	}

	void PhysicsWorld::Shutdown()
	{
		if (!m_Initialized)
			return;

		m_BodyToEntityMap.clear();

		m_PhysicsSystem.SetContactListener(nullptr);

		m_JobSystem.reset();
		m_TempAllocator.reset();
		m_ObjectVsBroadPhaseLayerFilter.reset();
		m_ObjectLayerPairFilter.reset();
		m_BroadPhaseLayerInterface.reset();

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		m_Initialized = false;
		CB_CORE_INFO("PhysicsWorld shut down");
	}

	void PhysicsWorld::Step(Timestep ts)
	{
		if (!m_Initialized)
			return;

		m_Accumulator += ts.GetSeconds();

		int steps = 0;
		while (m_Accumulator >= FIXED_TIMESTEP && steps < MAX_STEPS_PER_FRAME)
		{
			m_PhysicsSystem.Update(FIXED_TIMESTEP, 1, m_TempAllocator.get(), m_JobSystem.get());
			m_Accumulator -= FIXED_TIMESTEP;
			steps++;
		}

		// Clamp remaining accumulator to avoid spiral of death
		if (m_Accumulator > FIXED_TIMESTEP)
			m_Accumulator = FIXED_TIMESTEP;
	}

	void PhysicsWorld::SetGravity(const Vector3& gravity)
	{
		if (m_Initialized)
			m_PhysicsSystem.SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
	}

	Vector3 PhysicsWorld::GetGravity() const
	{
		if (!m_Initialized)
			return Vector3(0.0f, -9.81f, 0.0f);
		JPH::Vec3 g = m_PhysicsSystem.GetGravity();
		return Vector3(g.GetX(), g.GetY(), g.GetZ());
	}

	JPH::BodyInterface& PhysicsWorld::GetBodyInterface()
	{
		return m_PhysicsSystem.GetBodyInterface();
	}

	void PhysicsWorld::RegisterBodyEntity(JPH::BodyID bodyID, UUID entityUUID)
	{
		m_BodyToEntityMap[bodyID.GetIndexAndSequenceNumber()] = entityUUID;
	}

	void PhysicsWorld::UnregisterBody(JPH::BodyID bodyID)
	{
		m_BodyToEntityMap.erase(bodyID.GetIndexAndSequenceNumber());
	}

	// Contact listener implementation

	JPH::ValidateResult PhysicsWorld::OnContactValidate(
		const JPH::Body& inBody1, const JPH::Body& inBody2,
		JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
	{
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void PhysicsWorld::OnContactAdded(
		const JPH::Body& inBody1, const JPH::Body& inBody2,
		const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		if (!m_CollisionBeginCallback)
			return;

		auto itA = m_BodyToEntityMap.find(inBody1.GetID().GetIndexAndSequenceNumber());
		auto itB = m_BodyToEntityMap.find(inBody2.GetID().GetIndexAndSequenceNumber());
		if (itA == m_BodyToEntityMap.end() || itB == m_BodyToEntityMap.end())
			return;

		JPH::RVec3 contactPoint = inManifold.GetWorldSpaceContactPointOn1(0);
		JPH::Vec3 contactNormal = inManifold.mWorldSpaceNormal;

		CollisionCallback callback;
		callback.EntityA = itA->second;
		callback.EntityB = itB->second;
		callback.ContactPoint = Vector3(
			static_cast<float>(contactPoint.GetX()),
			static_cast<float>(contactPoint.GetY()),
			static_cast<float>(contactPoint.GetZ()));
		callback.ContactNormal = Vector3(contactNormal.GetX(), contactNormal.GetY(), contactNormal.GetZ());

		m_CollisionBeginCallback(callback);
	}

	void PhysicsWorld::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		if (!m_CollisionEndCallback)
			return;

		auto itA = m_BodyToEntityMap.find(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber());
		auto itB = m_BodyToEntityMap.find(inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());
		if (itA == m_BodyToEntityMap.end() || itB == m_BodyToEntityMap.end())
			return;

		CollisionCallback callback;
		callback.EntityA = itA->second;
		callback.EntityB = itB->second;

		m_CollisionEndCallback(callback);
	}
}
