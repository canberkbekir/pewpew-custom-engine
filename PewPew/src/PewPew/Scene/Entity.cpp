#include "pewpch.h"
#include "Entity.h"

#include "PewPew/Components/CoreComponents.h"

namespace PewPew {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	UUID Entity::GetUUID() const
	{
		return GetComponent<IDComponent>().ID;
	}

	const String& Entity::GetName() const
	{
		return GetComponent<TagComponent>().Tag;
	}

}