#include "cbpch.h"
#include "Entity.h"

#include "CBEngine/Components/CoreComponents.h"

namespace CB
{
	Entity::Entity(entt::entity handle,Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	UUID Entity::GetUUID() const { return GetComponent<IDComponent>().ID; }

	const String& Entity::GetName() const { return GetComponent<TagComponent>().Tag; }

	void Entity::SetParent(Entity newParent,bool keepWorldPosition)
	{
		if (!HasComponent<TransformComponent>())
			return;

		if (newParent == *this)
			return;

		if (newParent && newParent.IsDescendantOf(*this))
			return;

		auto& transform = GetComponent<TransformComponent>();
		Vector3 worldPos = transform.GetWorldPosition();

		if (transform.Parent != 0) {
			Entity oldParent = m_Scene->GetEntityByUUID(transform.Parent);
			if (oldParent) {
				auto& oldChildren = oldParent.GetComponent<TransformComponent>().Children;
				oldChildren.erase(
					std::remove(oldChildren.begin(), oldChildren.end(), GetUUID()),
					oldChildren.end()
				);
			}
		}

		// Set new parent
		if (newParent) {
			transform.Parent = newParent.GetUUID();
			newParent.GetComponent<TransformComponent>().Children.push_back(GetUUID());

			if (keepWorldPosition) {
				Mat4 parentInverse = inverse(newParent.GetComponent<TransformComponent>().WorldMatrix);
				transform.Position = Vector3(parentInverse * Vector4(worldPos, 1.0f));
			}
		}
		else {
			transform.Parent = UUID(0);
			if (keepWorldPosition)
				transform.Position = worldPos;
		}

		transform.Dirty = true;
	}

	void Entity::RemoveParent(bool keepWorldPosition) { SetParent(Entity{}, keepWorldPosition); }

	Entity Entity::GetParent() const
	{
		if (!HasComponent<TransformComponent>())
			return Entity{};

		UUID parentUUID = GetComponent<TransformComponent>().Parent;
		if (parentUUID == 0)
			return Entity{};

		return m_Scene->GetEntityByUUID(parentUUID);
	}

	std::vector<Entity> Entity::GetChildren() const
	{
		std::vector<Entity> result;
		if (!HasComponent<TransformComponent>())
			return result;

		for (UUID uuid : GetComponent<TransformComponent>().Children) {
			Entity child = m_Scene->GetEntityByUUID(uuid);
			if (child)
				result.push_back(child);
		}
		return result;
	}
}