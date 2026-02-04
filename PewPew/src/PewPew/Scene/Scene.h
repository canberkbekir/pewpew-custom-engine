#pragma once 
#include "PewPew/Core/String.h"
#include "PewPew/Core/TimeStep.h"
#include "PewPew/Core/UUID.h"
#include "entt.hpp"
namespace PewPew
{
	class Entity;
	
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		Entity CreateEntity(const String Name);
		Entity CreateEntityWithUUID(const UUID UUID, const String Name);
		void DestroyEntity(const Entity EntityToDelete);

		Entity GetEntityByUUID(const UUID UUID);
		bool EntityExists(const UUID UUID);

		void OnUpdate(Timestep ts);
		void OnRender();

		
		template<typename... Components>
		auto GetEntitiesWith() { return m_Registry.view<Components...>(); }

		entt::registry & GetRegistry() { return m_Registry; }
	private:
		void ProcessDeferredDestroys();

		entt::registry m_Registry;
		std::unordered_map<UUID, entt::entity> m_EntityMap;  
		std::vector<entt::entity> m_DeferredDestroys;
	};
}
