#pragma once
#include "CBEngine/Core/String.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"
#include "entt.hpp"

namespace CB
{
    class Entity;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        Entity CreateEntity(String Name);
        Entity CreateEntityWithUUID(UUID UUID, String Name);
        void DestroyEntity(Entity EntityToDelete);

        Entity GetEntityByUUID(UUID UUID);
        bool EntityExists(UUID UUID);

        void OnUpdate(Timestep ts);
        void OnRender();

        // Physics
        void InitPhysics();
        void ShutdownPhysics();
        bool IsPhysicsInitialized() const { return m_PhysicsInitialized; }

        template <typename... Components>
        auto GetEntitiesWith() { return m_Registry.view<Components...>(); }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        void ProcessDeferredDestroys();

        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_EntityMap;
        std::vector<entt::entity> m_DeferredDestroys;

        bool m_PhysicsInitialized = false;
    };
}
