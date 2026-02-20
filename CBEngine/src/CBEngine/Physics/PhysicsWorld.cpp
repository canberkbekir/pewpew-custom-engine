#include "cbpch.h"
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Body/BodyLock.h>

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

	static bool s_JoltGlobalsInitialized = false;

	void PhysicsWorld::InitJoltGlobals()
	{
		if (s_JoltGlobalsInitialized)
			return;

		JPH::Trace = JoltTraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		s_JoltGlobalsInitialized = true;
		CB_CORE_INFO("Jolt globals initialized");
	}

	void PhysicsWorld::ShutdownJoltGlobals()
	{
		if (!s_JoltGlobalsInitialized)
			return;

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		s_JoltGlobalsInitialized = false;
		CB_CORE_INFO("Jolt globals shut down");
	}

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

		CB_CORE_ASSERT(s_JoltGlobalsInitialized, "Must call PhysicsWorld::InitJoltGlobals() before Init()!");

		// Allocators
		m_TempAllocator = CreateScope<JPH::TempAllocatorImpl>(10 * 1024 * 1024); // 10 MB
		m_JobSystem = CreateScope<JPH::JobSystemThreadPool>(
			JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
			std::max(1u, std::thread::hardware_concurrency() - 1));

		// Broad-phase layer mapping: 16 object layers -> 2 broadphase layers
		// Layers 0 (Default) and 3 (Environment) map to NON_MOVING broadphase;
		// all other layers map to MOVING broadphase.
		m_BroadPhaseLayerInterface = CreateScope<JPH::BroadPhaseLayerInterfaceTable>(
			ObjectLayers::NUM_LAYERS, BroadPhaseLayers::NUM_LAYERS);
		for (uint32_t i = 0; i < ObjectLayers::NUM_LAYERS; i++)
		{
			if (i == 0 || i == 3) // Default, Environment
				m_BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(static_cast<JPH::ObjectLayer>(i), BroadPhaseLayers::NON_MOVING);
			else
				m_BroadPhaseLayerInterface->MapObjectToBroadPhaseLayer(static_cast<JPH::ObjectLayer>(i), BroadPhaseLayers::MOVING);
		}

		// Object layer pair filter — enable all-vs-all collisions by default
		m_ObjectLayerPairFilter = CreateScope<JPH::ObjectLayerPairFilterTable>(ObjectLayers::NUM_LAYERS);
		for (uint32_t i = 0; i < ObjectLayers::NUM_LAYERS; i++)
			for (uint32_t j = i; j < ObjectLayers::NUM_LAYERS; j++)
				m_ObjectLayerPairFilter->EnableCollision(static_cast<JPH::ObjectLayer>(i), static_cast<JPH::ObjectLayer>(j));

		// Broad-phase vs object layer filter
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

	UUID PhysicsWorld::GetEntityFromBody(JPH::BodyID bodyID) const
	{
		auto it = m_BodyToEntityMap.find(bodyID.GetIndexAndSequenceNumber());
		if (it != m_BodyToEntityMap.end())
			return it->second;
		return UUID(0);
	}

	JPH::BodyID PhysicsWorld::GetBodyFromEntity(UUID entityUUID) const
	{
		for (auto& [key, uuid] : m_BodyToEntityMap)
		{
			if (uuid == entityUUID)
				return JPH::BodyID(key);
		}
		return JPH::BodyID();
	}

	// Custom ObjectLayerFilter for raycast layer masking
	class LayerMaskObjectLayerFilter : public JPH::ObjectLayerFilter
	{
	public:
		LayerMaskObjectLayerFilter(uint16_t mask) : m_Mask(mask) {}

		bool ShouldCollide(JPH::ObjectLayer inLayer) const override
		{
			return (m_Mask & (1 << inLayer)) != 0;
		}

	private:
		uint16_t m_Mask;
	};

	bool PhysicsWorld::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
		RaycastHit& outHit, uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized)
			return false;

		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		float dirLen = dir.Length();
		if (dirLen < 0.0001f)
			return false;

		JPH::Vec3 normDir = dir / dirLen;

		JPH::RRayCast ray(
			JPH::RVec3(origin.x, origin.y, origin.z),
			normDir * maxDistance);

		JPH::RayCastResult result;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = m_PhysicsSystem.GetNarrowPhaseQuery();

		bool hit;
		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			hit = narrowPhase.CastRay(ray, result, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			hit = narrowPhase.CastRay(ray, result, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		if (!hit)
			return false;

		outHit.Fraction = result.mFraction;
		outHit.Distance = result.mFraction * maxDistance;

		JPH::RVec3 hitPoint = ray.GetPointOnRay(result.mFraction);
		outHit.Point = Vector3(
			static_cast<float>(hitPoint.GetX()),
			static_cast<float>(hitPoint.GetY()),
			static_cast<float>(hitPoint.GetZ()));

		outHit.EntityUUID = GetEntityFromBody(result.mBodyID);

		// Get surface normal
		JPH::BodyLockRead bodyLock(m_PhysicsSystem.GetBodyLockInterface(), result.mBodyID);
		if (bodyLock.Succeeded())
		{
			const JPH::Body& body = bodyLock.GetBody();
			JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint);
			outHit.Normal = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
		}

		return true;
	}

	std::vector<RaycastHit> PhysicsWorld::RaycastAll(const Vector3& origin, const Vector3& direction,
		float maxDistance, uint16_t layerMask, UUID ignoreEntity) const
	{
		std::vector<RaycastHit> hits;

		if (!m_Initialized)
			return hits;

		JPH::Vec3 dir(direction.x, direction.y, direction.z);
		float dirLen = dir.Length();
		if (dirLen < 0.0001f)
			return hits;

		JPH::Vec3 normDir = dir / dirLen;

		JPH::RRayCast ray(
			JPH::RVec3(origin.x, origin.y, origin.z),
			normDir * maxDistance);

		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = m_PhysicsSystem.GetNarrowPhaseQuery();
		JPH::RayCastSettings settings;
		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			narrowPhase.CastRay(ray, settings, collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			narrowPhase.CastRay(ray, settings, collector, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		collector.Sort();

		hits.reserve(collector.mHits.size());
		for (const auto& result : collector.mHits)
		{
			RaycastHit hit;
			hit.Fraction = result.mFraction;
			hit.Distance = result.mFraction * maxDistance;

			JPH::RVec3 hitPoint = ray.GetPointOnRay(result.mFraction);
			hit.Point = Vector3(
				static_cast<float>(hitPoint.GetX()),
				static_cast<float>(hitPoint.GetY()),
				static_cast<float>(hitPoint.GetZ()));

			hit.EntityUUID = GetEntityFromBody(result.mBodyID);

			JPH::BodyLockRead bodyLock(m_PhysicsSystem.GetBodyLockInterface(), result.mBodyID);
			if (bodyLock.Succeeded())
			{
				const JPH::Body& body = bodyLock.GetBody();
				JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, hitPoint);
				hit.Normal = Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
			}

			hits.push_back(hit);
		}

		return hits;
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
