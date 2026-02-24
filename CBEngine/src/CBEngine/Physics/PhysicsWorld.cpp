#include "cbpch.h"
#include "PhysicsWorld.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
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
		{
			std::lock_guard<std::mutex> lock(m_SensorPairsMutex);
			m_SensorPairs.clear();
		}

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

	// -------------------------------------------------------------------------
	// Overlap helpers — shared logic for sphere/box overlap queries
	// -------------------------------------------------------------------------

	static std::vector<RaycastHit> DoOverlapQuery(
		const PhysicsWorld* world,
		const JPH::Shape* testShape,
		const JPH::RMat44& transform,
		const Vector3& center,
		uint16_t layerMask,
		UUID ignoreEntity)
	{
		std::vector<RaycastHit> hits;

		JPH::CollideShapeSettings settings;
		settings.mMaxSeparationDistance = 0.0f;

		JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = world->GetPhysicsSystem().GetNarrowPhaseQuery();

		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = world->GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			narrowPhase.CollideShape(testShape, JPH::Vec3::sReplicate(1.0f),
				transform, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			narrowPhase.CollideShape(testShape, JPH::Vec3::sReplicate(1.0f),
				transform, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		hits.reserve(collector.mHits.size());
		for (const auto& result : collector.mHits)
		{
			RaycastHit hit;
			hit.EntityUUID = world->GetEntityFromBody(result.mBodyID2);
			if (!hit.EntityUUID.IsValid())
				continue;

			hit.Point = Vector3(
				static_cast<float>(result.mContactPointOn2.GetX()),
				static_cast<float>(result.mContactPointOn2.GetY()),
				static_cast<float>(result.mContactPointOn2.GetZ()));
			hit.Normal = Vector3(
				result.mPenetrationAxis.GetX(),
				result.mPenetrationAxis.GetY(),
				result.mPenetrationAxis.GetZ());
			hit.Distance = glm::distance(center, hit.Point);
			hit.Fraction = 0.0f;
			hits.push_back(hit);
		}

		return hits;
	}

	std::vector<RaycastHit> PhysicsWorld::OverlapSphere(const Vector3& center, float radius,
		uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};

		JPH::SphereShape sphereShape(std::max(radius, 0.001f));
		JPH::RMat44 transform = JPH::RMat44::sTranslation(JPH::RVec3(center.x, center.y, center.z));
		return DoOverlapQuery(this, &sphereShape, transform, center, layerMask, ignoreEntity);
	}

	std::vector<RaycastHit> PhysicsWorld::OverlapBox(const Vector3& center, const Vector3& halfExtents,
		uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};

		Vector3 safeHalf = Vector3(
			std::max(halfExtents.x, 0.001f),
			std::max(halfExtents.y, 0.001f),
			std::max(halfExtents.z, 0.001f));
		JPH::BoxShape boxShape(JPH::Vec3(safeHalf.x, safeHalf.y, safeHalf.z));
		JPH::RMat44 transform = JPH::RMat44::sTranslation(JPH::RVec3(center.x, center.y, center.z));
		return DoOverlapQuery(this, &boxShape, transform, center, layerMask, ignoreEntity);
	}

	// -------------------------------------------------------------------------
	// Sweep cast helpers
	// -------------------------------------------------------------------------

	// Shared helper: build a RaycastHit from a ShapeCastResult
	static RaycastHit MakeSweepHit(const PhysicsWorld* world,
		const JPH::RShapeCast& shapeCast, const JPH::ShapeCastResult& result, float maxDist)
	{
		RaycastHit hit;
		hit.Fraction = result.mFraction;
		hit.Distance = result.mFraction * maxDist;
		hit.EntityUUID = world->GetEntityFromBody(result.mBodyID2);

		// Hit center-of-mass position = start + direction * fraction
		JPH::RVec3 hitPos = JPH::RVec3(shapeCast.mCenterOfMassStart.GetTranslation())
			+ JPH::RVec3(shapeCast.mDirection) * result.mFraction;
		hit.Point = Vector3(
			static_cast<float>(hitPos.GetX()),
			static_cast<float>(hitPos.GetY()),
			static_cast<float>(hitPos.GetZ()));

		// Penetration axis points from body 2 to body 1; negate for surface normal
		hit.Normal = Vector3(
			-result.mPenetrationAxis.GetX(),
			-result.mPenetrationAxis.GetY(),
			-result.mPenetrationAxis.GetZ());
		return hit;
	}

	// Shared shape-cast executor — returns sorted results
	static std::vector<JPH::ShapeCastResult> DoCastShape(
		const PhysicsWorld* world,
		const JPH::Shape* shape,
		const Vector3& origin, const JPH::Vec3& normDir, float maxDist,
		uint16_t layerMask, UUID ignoreEntity)
	{
		JPH::RMat44 startTransform = JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z));
		JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
			shape, JPH::Vec3::sReplicate(1.0f), startTransform, normDir * maxDist);

		JPH::ShapeCastSettings settings;
		JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = world->GetPhysicsSystem().GetNarrowPhaseQuery();
		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = world->GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		collector.Sort();
		return { collector.mHits.begin(), collector.mHits.end() };
	}

	bool PhysicsWorld::SphereCast(const Vector3& origin, const Vector3& dir, float radius, float maxDist,
		RaycastHit& outHit, uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return false;
		JPH::Vec3 jDir(dir.x, dir.y, dir.z);
		float dirLen = jDir.Length();
		if (dirLen < 0.0001f) return false;
		jDir /= dirLen;

		JPH::SphereShape sphereShape(std::max(radius, 0.001f));
		JPH::RMat44 startTransform = JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z));
		JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
			&sphereShape, JPH::Vec3::sReplicate(1.0f), startTransform, jDir * maxDist);

		JPH::ShapeCastSettings settings;
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = m_PhysicsSystem.GetNarrowPhaseQuery();
		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		if (!collector.HadHit()) return false;
		outHit = MakeSweepHit(this, shapeCast, collector.mHit, maxDist);
		return true;
	}

	bool PhysicsWorld::BoxCast(const Vector3& origin, const Vector3& dir, const Vector3& halfExtents, float maxDist,
		RaycastHit& outHit, uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return false;
		JPH::Vec3 jDir(dir.x, dir.y, dir.z);
		float dirLen = jDir.Length();
		if (dirLen < 0.0001f) return false;
		jDir /= dirLen;

		Vector3 safeHalf = Vector3(
			std::max(halfExtents.x, 0.001f),
			std::max(halfExtents.y, 0.001f),
			std::max(halfExtents.z, 0.001f));
		JPH::BoxShape boxShape(JPH::Vec3(safeHalf.x, safeHalf.y, safeHalf.z));
		JPH::RMat44 startTransform = JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z));
		JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
			&boxShape, JPH::Vec3::sReplicate(1.0f), startTransform, jDir * maxDist);

		JPH::ShapeCastSettings settings;
		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
		LayerMaskObjectLayerFilter layerFilter(layerMask);

		const auto& narrowPhase = m_PhysicsSystem.GetNarrowPhaseQuery();
		if (ignoreEntity.IsValid())
		{
			JPH::BodyID ignoreBody = GetBodyFromEntity(ignoreEntity);
			JPH::IgnoreSingleBodyFilter bodyFilter(ignoreBody);
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter, bodyFilter);
		}
		else
		{
			narrowPhase.CastShape(shapeCast, settings, JPH::RVec3::sZero(),
				collector, JPH::BroadPhaseLayerFilter{}, layerFilter);
		}

		if (!collector.HadHit()) return false;
		outHit = MakeSweepHit(this, shapeCast, collector.mHit, maxDist);
		return true;
	}

	std::vector<RaycastHit> PhysicsWorld::SphereCastAll(const Vector3& origin, const Vector3& dir, float radius,
		float maxDist, uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};
		JPH::Vec3 jDir(dir.x, dir.y, dir.z);
		float dirLen = jDir.Length();
		if (dirLen < 0.0001f) return {};
		jDir /= dirLen;

		JPH::SphereShape sphereShape(std::max(radius, 0.001f));
		JPH::RMat44 startTransform = JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z));
		JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
			&sphereShape, JPH::Vec3::sReplicate(1.0f), startTransform, jDir * maxDist);

		auto results = DoCastShape(this, &sphereShape, origin, jDir, maxDist, layerMask, ignoreEntity);
		std::vector<RaycastHit> hits;
		hits.reserve(results.size());
		for (const auto& r : results)
			hits.push_back(MakeSweepHit(this, shapeCast, r, maxDist));
		return hits;
	}

	std::vector<RaycastHit> PhysicsWorld::BoxCastAll(const Vector3& origin, const Vector3& dir, const Vector3& halfExtents,
		float maxDist, uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};
		JPH::Vec3 jDir(dir.x, dir.y, dir.z);
		float dirLen = jDir.Length();
		if (dirLen < 0.0001f) return {};
		jDir /= dirLen;

		Vector3 safeHalf = Vector3(
			std::max(halfExtents.x, 0.001f),
			std::max(halfExtents.y, 0.001f),
			std::max(halfExtents.z, 0.001f));
		JPH::BoxShape boxShape(JPH::Vec3(safeHalf.x, safeHalf.y, safeHalf.z));
		JPH::RMat44 startTransform = JPH::RMat44::sTranslation(JPH::RVec3(origin.x, origin.y, origin.z));
		JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
			&boxShape, JPH::Vec3::sReplicate(1.0f), startTransform, jDir * maxDist);

		auto results = DoCastShape(this, &boxShape, origin, jDir, maxDist, layerMask, ignoreEntity);
		std::vector<RaycastHit> hits;
		hits.reserve(results.size());
		for (const auto& r : results)
			hits.push_back(MakeSweepHit(this, shapeCast, r, maxDist));
		return hits;
	}

	std::vector<RaycastHit> PhysicsWorld::OverlapCapsule(const Vector3& center, float radius, float halfHeight,
		uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};

		// Jolt CapsuleShape(halfHeight, radius) — capsule along Y axis
		JPH::CapsuleShape capsuleShape(std::max(halfHeight, 0.001f), std::max(radius, 0.001f));
		JPH::RMat44 transform = JPH::RMat44::sTranslation(JPH::RVec3(center.x, center.y, center.z));
		return DoOverlapQuery(this, &capsuleShape, transform, center, layerMask, ignoreEntity);
	}

	std::vector<RaycastHit> PhysicsWorld::OverlapCapsuleSegment(
		const Vector3& p0, const Vector3& p1, float radius,
		uint16_t layerMask, UUID ignoreEntity) const
	{
		if (!m_Initialized) return {};

		JPH::Vec3 jSeg(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
		float segLen = jSeg.Length();

		// Degenerate segment — fall back to sphere
		if (segLen < 1e-6f)
			return OverlapSphere((p0 + p1) * 0.5f, radius, layerMask, ignoreEntity);

		float   halfHeight = segLen * 0.5f;
		Vector3 center((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f, (p0.z + p1.z) * 0.5f);

		// CapsuleShape is Y-axis aligned; build rotation from Y to segment direction
		JPH::Vec3 segDir = jSeg / segLen;
		JPH::Vec3 yAxis(0.0f, 1.0f, 0.0f);
		float     dot = yAxis.Dot(segDir);

		JPH::Quat jRot;
		if (dot > 1.0f - 1e-5f)
		{
			jRot = JPH::Quat::sIdentity();
		}
		else if (dot < -1.0f + 1e-5f)
		{
			// 180° around X axis: (x=1,y=0,z=0,w=0) in Jolt's (x,y,z,w) convention
			jRot = JPH::Quat(1.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			JPH::Vec3 axis  = yAxis.Cross(segDir).Normalized();
			float     angle = std::acos(std::max(-1.0f, std::min(1.0f, dot)));
			jRot = JPH::Quat::sRotation(axis, angle);
		}

		JPH::CapsuleShape capsuleShape(std::max(halfHeight, 0.001f), std::max(radius, 0.001f));
		JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(
			jRot, JPH::RVec3(center.x, center.y, center.z));

		return DoOverlapQuery(this, &capsuleShape, transform, center, layerMask, ignoreEntity);
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

		// Route sensor contacts to trigger callbacks; physical contacts to collision callbacks
		bool isSensor = inBody1.IsSensor() || inBody2.IsSensor();
		if (isSensor)
		{
			// Track this pair so OnContactRemoved can route it without a body lock
			uint32_t a = inBody1.GetID().GetIndexAndSequenceNumber();
			uint32_t b = inBody2.GetID().GetIndexAndSequenceNumber();
			uint64_t key = (uint64_t)std::min(a, b) | ((uint64_t)std::max(a, b) << 32);
			{
				std::lock_guard<std::mutex> lock(m_SensorPairsMutex);
				m_SensorPairs.insert(key);
			}
			if (m_TriggerEnterCallback)
				m_TriggerEnterCallback(callback);
		}
		else
		{
			if (m_CollisionBeginCallback)
				m_CollisionBeginCallback(callback);
		}
	}

	void PhysicsWorld::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		auto itA = m_BodyToEntityMap.find(inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber());
		auto itB = m_BodyToEntityMap.find(inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber());
		if (itA == m_BodyToEntityMap.end() || itB == m_BodyToEntityMap.end())
			return;

		CollisionCallback callback;
		callback.EntityA = itA->second;
		callback.EntityB = itB->second;

		// Check the sensor pair set — no body lock needed (BodyLockRead inside
		// OnContactRemoved deadlocks because Jolt already holds internal locks here).
		uint32_t a = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
		uint32_t b = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();
		uint64_t key = (uint64_t)std::min(a, b) | ((uint64_t)std::max(a, b) << 32);

		bool isSensor = false;
		{
			std::lock_guard<std::mutex> lock(m_SensorPairsMutex);
			auto it = m_SensorPairs.find(key);
			if (it != m_SensorPairs.end())
			{
				isSensor = true;
				m_SensorPairs.erase(it);
			}
		}

		if (isSensor)
		{
			if (m_TriggerExitCallback)
				m_TriggerExitCallback(callback);
		}
		else
		{
			if (m_CollisionEndCallback)
				m_CollisionEndCallback(callback);
		}
	}
}
