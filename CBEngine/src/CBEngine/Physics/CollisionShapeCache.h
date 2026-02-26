#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/UUID.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <unordered_map>

namespace CB
{
	class CollisionShapeCache
	{
	public:
		CollisionShapeCache() = default;

		bool Has(UUID id) const { return m_Cache.find(id) != m_Cache.end(); }

		JPH::RefConst<JPH::Shape> Get(UUID id) const
		{
			auto it = m_Cache.find(id);
			if (it != m_Cache.end())
				return it->second;
			return nullptr;
		}

		void Store(UUID id,JPH::RefConst<JPH::Shape> shape) { m_Cache[id] = std::move(shape); }

		void Invalidate(UUID id) { m_Cache.erase(id); }

		void Clear() { m_Cache.clear(); }
	private:
		std::unordered_map<UUID, JPH::RefConst<JPH::Shape>> m_Cache;
	};
}