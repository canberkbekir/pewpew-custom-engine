#include "pewpch.h"
#include "Scene.h"

#include "Entity.h"
#include "PewPew/Components/CoreComponents.h"
#include "PewPew/Components/TransformComponent.h"

namespace PewPew
{ 
	Entity Scene::CreateEntity(const String Name)
	{
		return  CreateEntityWithUUID(UUID(), Name);
	}
	
	Entity Scene::CreateEntityWithUUID(const UUID UUID,const String Name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(UUID);
		entity.AddComponent<TagComponent>(Name.empty() ? "Entity" : Name);
		entity.AddComponent<TransformComponent>();

		m_EntityMap[UUID] = entity;
		return entity;
	}

	void Scene::DestroyEntity(const Entity EntityToDelete)
	{
		m_EntityMap.erase(EntityToDelete.GetUUID());
		m_Registry.destroy(EntityToDelete);
	}
	
	Entity Scene::GetEntityByUUID(const UUID UUID)
	{ 
		if (m_EntityMap.find(UUID) != m_EntityMap.end())
			return { m_EntityMap.at(UUID), this };
		return {};
	}
	
	bool Scene::EntityExists(const UUID UUID)
	{
		return m_EntityMap.find(UUID) != m_EntityMap.end();
	}
	
	void Scene::OnUpdate(Timestep ts)
	{
		// 1. Process deferred destruction
		ProcessDeferredDestroys();

		// 2. Destruction system (spawn fragments)
		//DestructionSystem::OnUpdate(*this, ts);

		// 3. Physics system (gravity, collision)
		//PhysicsSystem::OnUpdate(*this, ts);

		// 4. Voxel mesh regeneration
		//VoxelSystem::RegenerateDirtyMeshes(*this);

		// 5. Fragment cleanup
		//DestructionSystem::CleanupExpiredFragments(*this);
	}

	void Scene::OnRender()
	{
		// Rendering is handled by EditorLayer/ViewportPanel which calls RenderSystem
	}

	void Scene::ProcessDeferredDestroys()
	{
		for (auto entity : m_DeferredDestroys)
		{
			auto& id = m_Registry.get<IDComponent>(entity);
			m_EntityMap.erase(id.ID);
			m_Registry.destroy(entity);
		}
		m_DeferredDestroys.clear();
	}
}