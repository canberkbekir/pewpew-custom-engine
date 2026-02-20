#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"

#include <string>
#include <vector>

#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"

// Forward declare sol types to avoid including sol.hpp in the header
namespace sol { class state; }

namespace CB
{
	class Scene;
	class Entity;
	struct ScriptEntry;

	class CB_API ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static sol::state& GetLuaState();

		// Parse __fields from a Lua script entry without entering play mode
		static void ParseScriptFields(ScriptEntry& entry);

		// Script lifecycle per entity (loops all script entries)
		static void OnEntityCreate(Scene* scene, Entity entity);
		static void OnEntityUpdate(Scene* scene, Entity entity, Timestep ts);
		static void OnEntityDestroy(Scene* scene, Entity entity);
		static void OnCollision(Scene* scene, Entity entity, Entity other,
			const Vector3& contactPoint, const Vector3& contactNormal);
		static void OnCollisionEnd(Scene* scene, Entity entity, Entity other);

		// Call OnValidate on a script entry's class table when a field changes in the editor
		static void CallOnValidate(ScriptEntry& entry, const std::string& changedField);

		// Check if an entity has any loaded script instances
		static bool HasScriptInstance(UUID entityUUID);

		// Hot-reload
		static void ReloadScript(const String& path);
		static void ReloadAllScripts();

		// GameManager discovery
		static void PreloadBaseScripts();
		static bool IsGameManagerScript(const std::string& scriptPath);
		static std::vector<std::string> GetGameManagerScripts();
		static void ScanForGameManagerScripts();

	private:
		static void RegisterBindings();
		static void RegisterScriptBindings(); // Registers GetScript, GetGameManager on Entity/Scene
	};
}
