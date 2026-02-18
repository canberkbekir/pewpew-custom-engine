#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"

#include <string>

// Forward declare sol types to avoid including sol.hpp in the header
namespace sol { class state; }

namespace CB
{
	class Scene;
	class Entity;

	class CB_API ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static sol::state& GetLuaState();

		// Script lifecycle per entity
		static void OnEntityCreate(Scene* scene, Entity entity);
		static void OnEntityUpdate(Scene* scene, Entity entity, Timestep ts);
		static void OnEntityDestroy(Scene* scene, Entity entity);
		static void OnCollision(Scene* scene, Entity entity, Entity other);

		// Check if an entity has a loaded script instance
		static bool HasScriptInstance(UUID entityUUID);

		// Hot-reload
		static void ReloadScript(const std::string& path);
		static void ReloadAllScripts();

	private:
		static void RegisterBindings();
	};
}
