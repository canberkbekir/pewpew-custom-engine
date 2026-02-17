#pragma once

#include "Event.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"

namespace CB
{
	class CollisionBeginEvent : public Event
	{
	public:
		CollisionBeginEvent(UUID entityA, UUID entityB, const Vector3& contactPoint, const Vector3& contactNormal)
			: m_EntityA(entityA), m_EntityB(entityB), m_ContactPoint(contactPoint), m_ContactNormal(contactNormal)
		{
		}

		UUID GetEntityA() const { return m_EntityA; }
		UUID GetEntityB() const { return m_EntityB; }
		const Vector3& GetContactPoint() const { return m_ContactPoint; }
		const Vector3& GetContactNormal() const { return m_ContactNormal; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "CollisionBeginEvent: " << (uint64_t)m_EntityA << " <-> " << (uint64_t)m_EntityB;
			return ss.str();
		}

		EVENT_CLASS_TYPE(CollisionBegin)
		EVENT_CLASS_CATEGORY(EventCategoryPhysics)

	private:
		UUID m_EntityA;
		UUID m_EntityB;
		Vector3 m_ContactPoint;
		Vector3 m_ContactNormal;
	};

	class CollisionEndEvent : public Event
	{
	public:
		CollisionEndEvent(UUID entityA, UUID entityB)
			: m_EntityA(entityA), m_EntityB(entityB)
		{
		}

		UUID GetEntityA() const { return m_EntityA; }
		UUID GetEntityB() const { return m_EntityB; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "CollisionEndEvent: " << (uint64_t)m_EntityA << " <-> " << (uint64_t)m_EntityB;
			return ss.str();
		}

		EVENT_CLASS_TYPE(CollisionEnd)
		EVENT_CLASS_CATEGORY(EventCategoryPhysics)

	private:
		UUID m_EntityA;
		UUID m_EntityB;
	};
}
