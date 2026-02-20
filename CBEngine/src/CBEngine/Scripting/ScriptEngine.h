#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"

#include <string>

#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"

// Forward declare sol types to avoid including sol.hpp in the header
namespace sol { class state; }

namespace CB
{
	class Scene;
	class Entity;
	struct ScriptComponent;

	class CB_API ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static sol::state& GetLuaState();

		// Parse __fields from a Lua script without entering play mode
		static void ParseScriptFields(ScriptComponent& script);

		// Script lifecycle per entity
		static void OnEntityCreate(Scene* scene, Entity entity);
		static void OnEntityUpdate(Scene* scene, Entity entity, Timestep ts);
		static void OnEntityDestroy(Scene* scene, Entity entity);
		static void OnCollision(Scene* scene, Entity entity, Entity other,
			const Vector3& contactPoint, const Vector3& contactNormal);
		static void OnCollisionEnd(Scene* scene, Entity entity, Entity other);

		// Call OnValidate on a script's class table when a field changes in the editor
		static void CallOnValidate(ScriptComponent& script, const std::string& changedField);

		// Check if an entity has a loaded script instance
		static bool HasScriptInstance(UUID entityUUID);

		// Hot-reload
		static void ReloadScript(const String& path);
		static void ReloadAllScripts();

	private:
		static void RegisterBindings();

		// Find the class table (global table with __fields) after executing a script
		static std::string FindClassTable();
	};
}
