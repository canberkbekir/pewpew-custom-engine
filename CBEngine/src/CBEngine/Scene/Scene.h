#pragma once
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Events/Event.h"
#include "entt.hpp"

#include <functional>
#include <vector>
#include <algorithm>

namespace CB
{
    class Entity;
    class PhysicsWorld;
    class ISystem;

    class CB_API Scene
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity(String Name);
        Entity CreateEntityWithUUID(UUID UUID, String Name);
        void DestroyEntity(Entity EntityToDelete);

        Entity GetEntityByUUID(UUID UUID);
        bool EntityExists(UUID UUID);

        void OnUpdate(Timestep ts);
        void OnRender();

        // Event callback
        using EventCallbackFn = std::function<void(Event&)>;
        void SetEventCallback(EventCallbackFn fn) { m_EventCallback = std::move(fn); }

        // Physics
        void InitPhysics();
        void ShutdownPhysics();
        bool IsPhysicsInitialized() const { return m_PhysicsInitialized; }
        PhysicsWorld* GetPhysicsWorld() const { return m_PhysicsWorld.get(); }

        // Simulation control
        void PausePhysics() { m_PhysicsPaused = true; }
        void ResumePhysics() { m_PhysicsPaused = false; }
        void StepOneFrame() { m_PhysicsStepOneFrame = true; }
        bool IsPhysicsPaused() const { return m_PhysicsPaused; }

        // System registry
        void RegisterSystem(Scope<ISystem> system);
        void UnregisterSystem(const char* name);

        template <typename... Components>
        auto GetEntitiesWith() { return m_Registry.view<Components...>(); }

        entt::registry& GetRegistry() { return m_Registry; }

    private:
        void ProcessDeferredDestroys();

        entt::registry m_Registry;
        std::unordered_map<UUID, entt::entity> m_EntityMap;
        std::vector<entt::entity> m_DeferredDestroys;

        std::vector<Scope<ISystem>> m_Systems;

        Scope<PhysicsWorld> m_PhysicsWorld;
        bool m_PhysicsInitialized = false;
        bool m_PhysicsPaused = false;
        bool m_PhysicsStepOneFrame = false;

        EventCallbackFn m_EventCallback;

        // Scene snapshot for play mode restore
        String m_PrePlaySnapshot;
    };
}
