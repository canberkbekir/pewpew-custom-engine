#include "cbpch.h"
#include "ScriptEngine.h"
#include "LuaBindings.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace CB
{
	static sol::state* s_LuaState = nullptr;
	static std::unordered_map<uint64_t, sol::table> s_ScriptInstances;

	void ScriptEngine::Init()
	{
		if (s_LuaState)
			return;

		s_LuaState = new sol::state();
		s_LuaState->open_libraries(
			sol::lib::base,
			sol::lib::math,
			sol::lib::string,
			sol::lib::table,
			sol::lib::os,
			sol::lib::coroutine
		);

		RegisterBindings();

		CB_CORE_INFO("ScriptEngine initialized (Lua 5.4)");
	}

	void ScriptEngine::Shutdown()
	{
		s_ScriptInstances.clear();
		delete s_LuaState;
		s_LuaState = nullptr;
		CB_CORE_INFO("ScriptEngine shut down");
	}

	sol::state& ScriptEngine::GetLuaState()
	{
		CB_CORE_ASSERT(s_LuaState, "ScriptEngine not initialized!");
		return *s_LuaState;
	}

	void ScriptEngine::RegisterBindings()
	{
		LuaBindings::RegisterAll(*s_LuaState);
	}

	bool ScriptEngine::HasScriptInstance(UUID entityUUID)
	{
		return s_ScriptInstances.find(static_cast<uint64_t>(entityUUID)) != s_ScriptInstances.end();
	}

	void ScriptEngine::OnEntityCreate(Scene* scene, Entity entity)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		auto& script = entity.GetComponent<ScriptComponent>();
		if (script.ScriptPath.empty())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());

		try
		{
			// Load and execute the script file
			auto result = s_LuaState->script_file(script.ScriptPath, sol::script_pass_on_error);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua script error ({0}): {1}", script.ScriptPath, err.what());
				return;
			}

			// Create a per-entity instance table (self)
			sol::table self = s_LuaState->create_table();

			// Store entity reference in self
			self["_entity"] = entity;
			self["_scene"] = scene;

			// Copy global functions defined by the script into self
			sol::table globals = s_LuaState->globals();
			for (auto& pair : globals)
			{
				if (pair.first.get_type() != sol::type::string)
					continue;
				std::string key = pair.first.as<std::string>();
				if (key == "OnCreate" || key == "OnUpdate" || key == "OnDestroy" ||
					key == "OnCollisionBegin" || key == "OnCollisionEnd")
				{
					self[key] = pair.second;
				}
			}

			s_ScriptInstances[uuid] = self;
			script.ScriptLoaded = true;

			// Call OnCreate if defined
			sol::object onCreateObj = self["OnCreate"];
			if (onCreateObj.is<sol::protected_function>())
			{
				sol::protected_function onCreate = onCreateObj;
				auto callResult = onCreate(self);
				if (!callResult.valid())
				{
					sol::error err = callResult;
					CB_CORE_ERROR("Lua OnCreate error ({0}): {1}", script.ScriptPath, err.what());
				}
			}
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("Lua script load error ({0}): {1}", script.ScriptPath, e.what());
		}
	}

	void ScriptEngine::OnEntityUpdate(Scene* scene, Entity entity, Timestep ts)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		auto& script = entity.GetComponent<ScriptComponent>();
		if (!script.ScriptLoaded)
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		sol::table& self = it->second;
		sol::object onUpdateObj = self["OnUpdate"];
		if (onUpdateObj.is<sol::protected_function>())
		{
			sol::protected_function onUpdate = onUpdateObj;
			auto result = onUpdate(self, ts.GetSeconds());
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua OnUpdate error ({0}): {1}", script.ScriptPath, err.what());
			}
		}
	}

	void ScriptEngine::OnEntityDestroy(Scene* scene, Entity entity)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		auto& script = entity.GetComponent<ScriptComponent>();
		if (!script.ScriptLoaded)
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		sol::table& self = it->second;
		sol::object onDestroyObj = self["OnDestroy"];
		if (onDestroyObj.is<sol::protected_function>())
		{
			sol::protected_function onDestroy = onDestroyObj;
			auto result = onDestroy(self);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua OnDestroy error ({0}): {1}", script.ScriptPath, err.what());
			}
		}

		s_ScriptInstances.erase(it);
		script.ScriptLoaded = false;
	}

	void ScriptEngine::OnCollision(Scene* scene, Entity entity, Entity other)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		auto& script = entity.GetComponent<ScriptComponent>();
		if (!script.ScriptLoaded)
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		sol::table& self = it->second;
		sol::object onCollisionObj = self["OnCollisionBegin"];
		if (onCollisionObj.is<sol::protected_function>())
		{
			sol::protected_function onCollisionBegin = onCollisionObj;
			auto result = onCollisionBegin(self, other);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua OnCollisionBegin error ({0}): {1}", script.ScriptPath, err.what());
			}
		}
	}

	void ScriptEngine::ReloadScript(const std::string& path)
	{
		// TODO: Implement per-path reload
		CB_CORE_INFO("Script hot-reload requested: {0}", path);
	}

	void ScriptEngine::ReloadAllScripts()
	{
		CB_CORE_INFO("Reloading all scripts...");
	}
}
