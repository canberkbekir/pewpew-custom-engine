#pragma once

#include <entt.hpp>
#include "Scene.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Core/UUID.h"

namespace CB
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene* scene);
        Entity(const Entity& other) = default;

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            CB_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
            return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T>
        T& GetComponent()
        {
            CB_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->GetRegistry().get<T>(m_EntityHandle);
        }

        template <typename T>
        const T& GetComponent() const
        {
            CB_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            return m_Scene->GetRegistry().get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent() const
        {
            return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
        }

        template <typename T>
        void RemoveComponent()
        {
            CB_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
            m_Scene->GetRegistry().remove<T>(m_EntityHandle);
        }

        UUID GetUUID() const;
        const String& GetName() const;

        operator bool() const { return m_EntityHandle != entt::null; }
        operator entt::entity() const { return m_EntityHandle; }
        operator uint32_t() const { return static_cast<uint32_t>(m_EntityHandle); }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

        // ===== Hierarchy =====
        void SetParent(Entity parent, bool keepWorldPosition = true);
        void RemoveParent(bool keepWorldPosition = true);
        Entity GetParent() const;
        std::vector<Entity> GetChildren() const;

        bool HasParent() const
        {
            if (!HasComponent<TransformComponent>())
                return false;
            return GetComponent<TransformComponent>().HasParent();
        }

        bool HasChildren() const
        {
            if (!HasComponent<TransformComponent>())
                return false;
            return GetComponent<TransformComponent>().HasChildren();
        }

        bool IsDescendantOf(Entity ancestor) const
        {
            Entity current = GetParent();
            while (current)
            {
                if (current == ancestor)
                    return true;
                current = current.GetParent();
            }
            return false;
        }

    private:
        entt::entity m_EntityHandle{entt::null};
        Scene* m_Scene = nullptr;
    };
}
