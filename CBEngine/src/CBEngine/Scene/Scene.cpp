#include "cbpch.h"
#include "Scene.h"

#include "Entity.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Systems/TransformSystem.h"
#include "CBEngine/Core/Application.h"
#include "CBEngine/Events/SceneEvent.h"

namespace CB
{
    Entity Scene::CreateEntity(const String Name)
    {
        return CreateEntityWithUUID(UUID(), Name);
    }

    Entity Scene::CreateEntityWithUUID(const UUID UUID, const String Name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>(UUID);
        String entityName = Name.empty() ? "Entity" : Name;
        entity.AddComponent<TagComponent>(entityName);
        entity.AddComponent<TransformComponent>();

        m_EntityMap[UUID] = entity;

        EntityCreatedEvent event(UUID, entityName);
        Application::Get().OnEvent(event);

        return entity;
    }

    void Scene::DestroyEntity(const Entity EntityToDelete)
    {
        // Clean up hierarchy before destroying
        if (EntityToDelete.HasComponent<TransformComponent>())
        {
            auto& transform = EntityToDelete.GetComponent<TransformComponent>();
            UUID myUUID = EntityToDelete.GetUUID();

            // Remove from parent's children list
            if (transform.HasParent())
            {
                Entity parent = GetEntityByUUID(transform.Parent);
                if (parent)
                {
                    auto& parentChildren = parent.GetComponent<TransformComponent>().Children;
                    parentChildren.erase(
                        std::remove(parentChildren.begin(), parentChildren.end(), myUUID),
                        parentChildren.end()
                    );
                }
            }

            // Reparent children to root
            for (UUID childUUID : transform.Children)
            {
                Entity child = GetEntityByUUID(childUUID);
                if (child)
                {
                    auto& childTransform = child.GetComponent<TransformComponent>();
                    childTransform.Position = childTransform.GetWorldPosition();
                    childTransform.Parent = UUID(0);
                    childTransform.Dirty = true;
                }
            }
        }

        m_EntityMap.erase(EntityToDelete.GetUUID());
        m_Registry.destroy(EntityToDelete);
    }

    Entity Scene::GetEntityByUUID(const UUID UUID)
    {
        if (m_EntityMap.find(UUID) != m_EntityMap.end())
            return {m_EntityMap.at(UUID), this};
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

        //2. Transform system
        TransformSystem::OnUpdate(this, ts);

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
