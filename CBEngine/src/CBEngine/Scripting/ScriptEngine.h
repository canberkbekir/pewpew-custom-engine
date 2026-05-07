#pragma once

#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"

#include <string>
#include <vector>

#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"

// Forward declare sol types to avoid including sol.hpp in the header
namespace sol
{
	class state;
}

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

		// Called at the start of each frame to update the Lua Time global table
		static void BeginFrame(Timestep ts);

		// Script lifecycle per entity (loops all script entries)
		static void OnEntityCreate(Scene* scene,Entity entity);
		static void OnEntityUpdate(Scene* scene,Entity entity,Timestep ts);
		static void OnEntityLateUpdate(Scene* scene,Entity entity,Timestep ts);
		// Called at a fixed timestep in sync with the physics step rate
		static void OnEntityFixedUpdate(Scene* scene,Entity entity,Timestep fixedTs);
		static void OnEntityDestroy(Scene* scene,Entity entity);
		static void OnCollision(Scene* scene,Entity entity,Entity other,
		                        const Vector3& contactPoint,const Vector3& contactNormal);
		static void OnCollisionEnd(Scene* scene,Entity entity,Entity other);
		static void OnTriggerEnter(Scene* scene,Entity entity,Entity other,
		                           const Vector3& contactPoint,const Vector3& contactNormal);
		static void OnTriggerExit(Scene* scene,Entity entity,Entity other);

		// Voxel destruction events — fired by VoxelDestructionSystem
		static void OnVoxelDamaged(uint64_t entityUUID,int gx,int gy,int gz,
		                           float normalizedHealth,float damageAmount);
		static void OnVoxelDestroyed(uint64_t entityUUID,int gx,int gy,int gz);
		static void OnStructuralCollapse(uint64_t entityUUID,int clusterCount,int totalVoxels);

		// Call OnValidate on a script entry's class table when a field changes in the editor
		static void CallOnValidate(ScriptEntry& entry,const std::string& changedField);

		// Check if an entity has any loaded script instances
		static bool HasScriptInstance(UUID entityUUID);

		// Hot-reload
		static void ReloadScript(const String& path);
		static void ReloadAllScripts();

		// Deferred scene management (safe to call from Lua scripts mid-frame)
		static void RequestSceneLoad(const std::string& path);
		static void RequestSceneReload();
		static bool HasPendingSceneChange();
		static std::string GetPendingScenePath();
		static void ClearPendingSceneChange();

		// Deferred play-mode stop (safe to call from Lua scripts mid-frame)
		static void RequestStopPlay();
		static bool HasPendingStopPlay();
		static void ClearPendingStopPlay();

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