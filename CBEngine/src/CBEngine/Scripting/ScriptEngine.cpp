#include "cbpch.h"
#include "ScriptEngine.h"
#include "LuaBindings.h"
#include "ComponentProxies.h"

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/ScriptComponent.h"
#include "CBEngine/Components/GameManagerComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Asset/AssetManager.h"
#include "CBEngine/Asset/Asset.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <filesystem>

namespace CB
{
	static sol::state* s_LuaState = nullptr;

	// Per-entity: vector of script instances (indexed same as ScriptComponent::Scripts)
	static std::unordered_map<uint64_t, std::vector<sol::table>> s_ScriptInstances;

	// Cached class tables by script path — ensures same class table reference for GetScript
	static std::unordered_map<std::string, sol::table> s_ClassTables;

	// Class names already registered as globals — so FindClassTable skips them
	static std::unordered_set<std::string> s_KnownClassNames;

	// Discovered scripts that inherit from GameManager
	static std::unordered_set<std::string> s_GameManagerScripts;

	// =========================================================================
	// File-scope helpers (forward declarations)
	// =========================================================================
	static std::string FindClassTable();
	static void InjectFields(sol::state& lua, sol::table& self, const std::vector<ScriptFieldDef>& fields);
	static sol::table GetOrLoadClassTable(const std::string& scriptPath, std::string& outClassName);
	static sol::table CreateScriptInstance(Scene* scene, Entity entity, ScriptEntry& entry);
	static sol::object GetScriptInstanceByTable(UUID entityUUID, sol::table classTableArg, sol::this_state L);

	// =========================================================================
	// ResolveScriptPath — resolve relative script paths to actual filesystem paths
	// =========================================================================
	static std::string ResolveScriptPath(const std::string& scriptPath)
	{
		if (scriptPath.empty())
			return scriptPath;

		// If the file exists as-is, use it (handles absolute paths and already-correct relative paths)
		if (std::filesystem::exists(scriptPath))
			return scriptPath;

		// Try resolving relative to the AssetManager's asset directory
		// metadata.FilePath is relative to asset root (e.g. "scripts/PlayerMovement.lua")
		const auto& assetDir = AssetManager::GetAssetDirectory();
		if (!assetDir.empty())
		{
			std::filesystem::path resolved = assetDir / scriptPath;
			if (std::filesystem::exists(resolved))
				return resolved.string();
		}

		// Try prepending "assets/" (common when working dir is CBEngine-Editor/)
		{
			std::string withAssets = "assets/" + scriptPath;
			if (std::filesystem::exists(withAssets))
				return withAssets;
		}

		// Return original — let script_file produce the error
		return scriptPath;
	}

	// =========================================================================
	// ScriptEngine public API
	// =========================================================================

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
			sol::lib::coroutine,
			sol::lib::package
		);

		RegisterBindings();

		// Configure Lua package.path for engine and user scripts
		(*s_LuaState)["package"]["path"] = "lua/?.lua;assets/scripts/?.lua";

		PreloadBaseScripts();

		CB_CORE_INFO("ScriptEngine initialized (Lua 5.4)");
	}

	void ScriptEngine::Shutdown()
	{
		s_ScriptInstances.clear();
		s_ClassTables.clear();
		s_KnownClassNames.clear();
		s_GameManagerScripts.clear();
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
		RegisterScriptBindings();
	}

	void ScriptEngine::RegisterScriptBindings()
	{
		auto& lua = *s_LuaState;

		sol::usertype<Entity> et = lua["Entity"];

		// --- Unified GetComponent(token_or_table) ---
		// If arg is a table with __component field -> built-in component proxy
		// If arg is a table without __component -> script class lookup
		// Also supports legacy string arg for backwards compat
		et["GetComponent"] = [](Entity& e, sol::object arg, sol::this_state L) -> sol::object {
			if (!e) return sol::make_object(L, sol::nil);

			std::string componentName;

			if (arg.get_type() == sol::type::string)
			{
				// Legacy string-based: entity:GetComponent("RigidBody")
				componentName = arg.as<std::string>();
			}
			else if (arg.get_type() == sol::type::table)
			{
				sol::table tbl = arg;
				sol::object comp = tbl["__component"];
				if (comp.valid() && comp.get_type() == sol::type::string)
				{
					// Built-in component token
					componentName = comp.as<std::string>();
				}
				else
				{
					// Script class table — delegate to script instance lookup
					if (!e.HasComponent<ScriptComponent>())
						return sol::make_object(L, sol::nil);
					return GetScriptInstanceByTable(e.GetUUID(), tbl, L);
				}
			}
			else
			{
				CB_CORE_WARN("[Lua] GetComponent: expected a component token or class table");
				return sol::make_object(L, sol::nil);
			}

			// Dispatch built-in component proxies
			if (componentName == "RigidBody" || componentName == "RigidBodyComponent")
			{
				if (!e.HasComponent<RigidBodyComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, RigidBodyProxy{e});
			}
			if (componentName == "Transform" || componentName == "TransformComponent")
			{
				if (!e.HasComponent<TransformComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, TransformProxy{e});
			}
			if (componentName == "Camera" || componentName == "CameraComponent")
			{
				if (!e.HasComponent<CameraComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, CameraProxy{e});
			}
			if (componentName == "Collider" || componentName == "ColliderComponent")
			{
				if (!e.HasComponent<ColliderComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, ColliderProxy{e});
			}
			if (componentName == "MeshRenderer" || componentName == "MeshRendererComponent")
			{
				if (!e.HasComponent<MeshRendererComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, MeshRendererProxy{e});
			}
			if (componentName == "VoxelRenderer" || componentName == "VoxelRendererComponent")
			{
				if (!e.HasComponent<VoxelRendererComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, VoxelRendererProxy{e});
			}
			if (componentName == "DirectionalLight" || componentName == "DirectionalLightComponent")
			{
				if (!e.HasComponent<DirectionalLightComponent>())
					return sol::make_object(L, sol::nil);
				return sol::make_object(L, DirectionalLightProxy{e});
			}

			CB_CORE_WARN("[Lua] GetComponent: unknown component type '{0}'", componentName);
			return sol::make_object(L, sol::nil);
		};

		// Keep GetScript as alias for backwards compat (script-class-only lookup)
		et["GetScript"] = [](Entity& e, sol::table classTableArg, sol::this_state L) -> sol::object {
			if (!e || !e.HasComponent<ScriptComponent>())
				return sol::make_object(L, sol::nil);
			return GetScriptInstanceByTable(e.GetUUID(), classTableArg, L);
		};

		// Add scene methods to the existing Scene usertype
		sol::usertype<Scene> st = lua["Scene"];

		// scene:GetMainCamera() — returns the entity with Primary=true CameraComponent
		st["GetMainCamera"] = [](Scene& scene, sol::this_state L) -> sol::object {
			auto view = scene.GetRegistry().view<CameraComponent>();
			for (auto entity : view)
			{
				auto& cam = view.get<CameraComponent>(entity);
				if (cam.Primary)
					return sol::make_object(L, Entity{ entity, &scene });
			}
			return sol::make_object(L, sol::nil);
		};

		st["GetGameManager"] = [](Scene& scene, sol::this_state L) -> sol::object {
			auto view = scene.GetRegistry().view<GameManagerComponent>();
			for (auto entity : view)
			{
				return sol::make_object(L, Entity{ entity, &scene });
			}
			CB_CORE_WARN("[Lua] No GameManager entity found in scene");
			return sol::make_object(L, sol::nil);
		};
	}

	bool ScriptEngine::HasScriptInstance(UUID entityUUID)
	{
		return s_ScriptInstances.find(static_cast<uint64_t>(entityUUID)) != s_ScriptInstances.end();
	}

	// =========================================================================
	// FindClassTable — scan globals for a NEW table containing __fields
	// =========================================================================
	static std::string FindClassTable()
	{
		sol::table globals = s_LuaState->globals();
		for (auto& pair : globals)
		{
			if (pair.first.get_type() != sol::type::string)
				continue;

			std::string key = pair.first.as<std::string>();

			// Skip non-table types
			if (pair.second.get_type() != sol::type::table)
				continue;

			// Skip built-in Lua globals and component token tables
			if (key == "string" || key == "table" || key == "math" || key == "io" ||
				key == "os" || key == "coroutine" || key == "debug" || key == "package" ||
				key == "utf8" || key == "_G" || key == "Log" || key == "Input" ||
				key == "Key" || key == "Mouse" || key == "Vec3" || key == "Vec4" ||
				key == "Float" || key == "Int" || key == "Bool" || key == "String" ||
				key == "Color" || key == "Vector3" || key == "BodyType" ||
				key == "Layer" || key == "Debug" || key == "Quat" ||
				key == "Entity" || key == "Scene" || key == "Mouse" ||
				key == "RigidBody" || key == "Transform" || key == "Camera" ||
				key == "Collider" || key == "MeshRenderer" || key == "VoxelRenderer" ||
				key == "DirectionalLight" || key == "RaycastHit")
				continue;

			// Skip already-known class tables
			if (s_KnownClassNames.count(key))
				continue;

			sol::table tbl = pair.second;
			sol::object fieldsObj = tbl["__fields"];
			if (fieldsObj.valid() && fieldsObj.get_type() == sol::type::table)
				return key;
		}
		return "";
	}

	// =========================================================================
	// ParseScriptFields — parse __fields from a script entry without play mode
	// =========================================================================
	void ScriptEngine::ParseScriptFields(ScriptEntry& entry)
	{
		if (!s_LuaState || entry.ScriptPath.empty())
			return;

		try
		{
			// Use GetOrLoadClassTable which handles caching — avoids the bug where
			// ScanForGameManagerScripts adds class names to s_KnownClassNames before
			// ParseScriptFields runs, making FindClassTable unable to find them.
			std::string className;
			sol::table classTable = GetOrLoadClassTable(entry.ScriptPath, className);

			if (!classTable.valid())
			{
				entry.ClassName = "";
				entry.FieldsParsed = true;
				return;
			}

			// GetOrLoadClassTable returns empty className when using cache;
			// use entry.ClassName as fallback, or discover from global
			if (className.empty())
				className = entry.ClassName;
			if (className.empty())
			{
				// Try to find the class name from the global that points to this table
				sol::table globals = s_LuaState->globals();
				for (auto& pair : globals)
				{
					if (pair.first.get_type() != sol::type::string) continue;
					if (pair.second.get_type() != sol::type::table) continue;
					sol::table tbl = pair.second;
					if (tbl.pointer() == classTable.pointer())
					{
						className = pair.first.as<std::string>();
						break;
					}
				}
			}

			entry.ClassName = className;
			sol::table fieldsTable = classTable["__fields"];

			// Save existing overrides keyed by name
			std::unordered_map<std::string, ScriptFieldDef> existingOverrides;
			for (auto& f : entry.Fields)
			{
				if (f.HasOverride)
					existingOverrides[f.Name] = f;
			}

			entry.Fields.clear();

			for (auto& pair : fieldsTable)
			{
				if (pair.first.get_type() != sol::type::string)
					continue;
				if (pair.second.get_type() != sol::type::table)
					continue;

				std::string fieldName = pair.first.as<std::string>();
				sol::table fieldDef = pair.second;

				ScriptFieldDef def;
				def.Name = fieldName;

				sol::object typeObj = fieldDef["type"];
				if (typeObj.valid() && typeObj.get_type() == sol::type::string)
					def.Type = ScriptFieldTypeFromString(typeObj.as<std::string>());

				sol::object minObj = fieldDef["min"];
				if (minObj.valid() && minObj.get_type() == sol::type::number)
					def.Min = minObj.as<float>();
				sol::object maxObj = fieldDef["max"];
				if (maxObj.valid() && maxObj.get_type() == sol::type::number)
					def.Max = maxObj.as<float>();

				sol::object defaultObj = fieldDef["default"];
				if (defaultObj.valid())
				{
					switch (def.Type)
					{
						case ScriptFieldType::Float:
							if (defaultObj.get_type() == sol::type::number)
								def.FloatValue = defaultObj.as<float>();
							break;
						case ScriptFieldType::Int:
							if (defaultObj.get_type() == sol::type::number)
								def.IntValue = defaultObj.as<int>();
							break;
						case ScriptFieldType::Bool:
							if (defaultObj.get_type() == sol::type::boolean)
								def.BoolValue = defaultObj.as<bool>();
							break;
						case ScriptFieldType::String:
							if (defaultObj.get_type() == sol::type::string)
								def.StringValue = defaultObj.as<std::string>();
							break;
						case ScriptFieldType::Color:
							if (defaultObj.get_type() == sol::type::table)
							{
								sol::table t = defaultObj;
								if (t.size() >= 4)
								{
									def.ColorValue.r = t[1].get<float>();
									def.ColorValue.g = t[2].get<float>();
									def.ColorValue.b = t[3].get<float>();
									def.ColorValue.a = t[4].get<float>();
								}
							}
							break;
						case ScriptFieldType::Vector3:
							if (defaultObj.get_type() == sol::type::table)
							{
								sol::table t = defaultObj;
								if (t.size() >= 3)
								{
									def.Vector3Value.x = t[1].get<float>();
									def.Vector3Value.y = t[2].get<float>();
									def.Vector3Value.z = t[3].get<float>();
								}
							}
							break;
					}
				}

				auto it = existingOverrides.find(fieldName);
				if (it != existingOverrides.end() && it->second.Type == def.Type)
				{
					def.HasOverride = true;
					def.FloatValue = it->second.FloatValue;
					def.IntValue = it->second.IntValue;
					def.BoolValue = it->second.BoolValue;
					def.StringValue = it->second.StringValue;
					def.ColorValue = it->second.ColorValue;
					def.Vector3Value = it->second.Vector3Value;
				}

				entry.Fields.push_back(def);
			}

			// Keep class table in globals for GetScript access, but track it
			s_KnownClassNames.insert(className);

			// Check if this script inherits from GameManager
			sol::object isGM = classTable["__isGameManager"];
			if (isGM.valid() && isGM.get_type() == sol::type::boolean && isGM.as<bool>())
				s_GameManagerScripts.insert(entry.ScriptPath);

			entry.FieldsParsed = true;
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("ParseScriptFields exception ({0}): {1}", entry.ScriptPath, e.what());
			entry.FieldsParsed = true;
		}
	}

	// =========================================================================
	// CallOnValidate — invoke OnValidate on the class table when a field changes
	// =========================================================================
	void ScriptEngine::CallOnValidate(ScriptEntry& entry, const std::string& changedField)
	{
		if (!s_LuaState || entry.ScriptPath.empty() || entry.ClassName.empty())
			return;

		try
		{
			std::string resolved = ResolveScriptPath(entry.ScriptPath);
			auto result = s_LuaState->script_file(resolved, sol::script_pass_on_error);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("CallOnValidate script error ({0}): {1}", resolved, err.what());
				return;
			}

			std::string className = FindClassTable();
			if (className.empty())
				return;

			sol::table classTable = (*s_LuaState)[className];
			sol::object onValidateObj = classTable["OnValidate"];
			if (!onValidateObj.is<sol::protected_function>())
			{
				s_KnownClassNames.insert(className);
				return;
			}

			sol::table fieldsTable = s_LuaState->create_table();
			for (const auto& field : entry.Fields)
			{
				switch (field.Type)
				{
					case ScriptFieldType::Float:
						fieldsTable[field.Name] = field.FloatValue;
						break;
					case ScriptFieldType::Int:
						fieldsTable[field.Name] = field.IntValue;
						break;
					case ScriptFieldType::Bool:
						fieldsTable[field.Name] = field.BoolValue;
						break;
					case ScriptFieldType::String:
						fieldsTable[field.Name] = field.StringValue;
						break;
					case ScriptFieldType::Color:
					{
						sol::table ct = s_LuaState->create_table();
						ct[1] = field.ColorValue.r; ct[2] = field.ColorValue.g;
						ct[3] = field.ColorValue.b; ct[4] = field.ColorValue.a;
						fieldsTable[field.Name] = ct;
						break;
					}
					case ScriptFieldType::Vector3:
						fieldsTable[field.Name] = field.Vector3Value;
						break;
				}
			}

			sol::protected_function onValidate = onValidateObj;
			auto callResult = onValidate(classTable, fieldsTable, changedField);
			if (!callResult.valid())
			{
				sol::error err = callResult;
				CB_CORE_ERROR("OnValidate error ({0}): {1}", entry.ScriptPath, err.what());
			}
			else if (callResult.get_type() == sol::type::table)
			{
				sol::table corrected = callResult;
				for (auto& field : entry.Fields)
				{
					sol::object val = corrected[field.Name];
					if (!val.valid())
						continue;

					switch (field.Type)
					{
						case ScriptFieldType::Float:
							if (val.get_type() == sol::type::number)
								field.FloatValue = val.as<float>();
							break;
						case ScriptFieldType::Int:
							if (val.get_type() == sol::type::number)
								field.IntValue = val.as<int>();
							break;
						case ScriptFieldType::Bool:
							if (val.get_type() == sol::type::boolean)
								field.BoolValue = val.as<bool>();
							break;
						case ScriptFieldType::String:
							if (val.get_type() == sol::type::string)
								field.StringValue = val.as<std::string>();
							break;
						case ScriptFieldType::Color:
							if (val.get_type() == sol::type::table)
							{
								sol::table t = val;
								if (t.size() >= 4)
								{
									field.ColorValue.r = t[1].get<float>();
									field.ColorValue.g = t[2].get<float>();
									field.ColorValue.b = t[3].get<float>();
									field.ColorValue.a = t[4].get<float>();
								}
							}
							break;
						case ScriptFieldType::Vector3:
							if (val.is<Vector3>())
								field.Vector3Value = val.as<Vector3>();
							break;
					}
				}
			}

			s_KnownClassNames.insert(className);
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("CallOnValidate exception ({0}): {1}", entry.ScriptPath, e.what());
		}
	}

	// =========================================================================
	// OnEntityCreate — create instances for all script entries on an entity
	// =========================================================================
	void ScriptEngine::OnEntityCreate(Scene* scene, Entity entity)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		auto& scriptComp = entity.GetComponent<ScriptComponent>();
		if (scriptComp.Scripts.empty())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());

		auto& instances = s_ScriptInstances[uuid];
		instances.clear();

		for (auto& entry : scriptComp.Scripts)
		{
			sol::table self = CreateScriptInstance(scene, entity, entry);
			instances.push_back(self);
		}
	}

	// =========================================================================
	// OnEntityUpdate — call OnUpdate on all script instances for an entity
	// =========================================================================
	void ScriptEngine::OnEntityUpdate(Scene* scene, Entity entity, Timestep ts)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		auto& scriptComp = entity.GetComponent<ScriptComponent>();

		for (size_t i = 0; i < it->second.size(); i++)
		{
			sol::table& self = it->second[i];
			if (!self.valid()) continue;

			sol::object onUpdateObj = self["OnUpdate"];
			if (onUpdateObj.is<sol::protected_function>())
			{
				sol::protected_function onUpdate = onUpdateObj;
				auto result = onUpdate(self, ts.GetSeconds());
				if (!result.valid())
				{
					sol::error err = result;
					const String& path = (i < scriptComp.Scripts.size())
						? scriptComp.Scripts[i].ScriptPath : "unknown";
					CB_CORE_ERROR("Lua OnUpdate error ({0}): {1}", path, err.what());
				}
			}
		}
	}

	// =========================================================================
	// OnEntityDestroy — call OnDestroy on all instances, then clean up
	// =========================================================================
	void ScriptEngine::OnEntityDestroy(Scene* scene, Entity entity)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		auto& scriptComp = entity.GetComponent<ScriptComponent>();

		for (size_t i = 0; i < it->second.size(); i++)
		{
			sol::table& self = it->second[i];
			if (!self.valid()) continue;

			sol::object onDestroyObj = self["OnDestroy"];
			if (onDestroyObj.is<sol::protected_function>())
			{
				sol::protected_function onDestroy = onDestroyObj;
				auto result = onDestroy(self);
				if (!result.valid())
				{
					sol::error err = result;
					const String& path = (i < scriptComp.Scripts.size())
						? scriptComp.Scripts[i].ScriptPath : "unknown";
					CB_CORE_ERROR("Lua OnDestroy error ({0}): {1}", path, err.what());
				}
			}
		}

		s_ScriptInstances.erase(it);

		for (auto& entry : scriptComp.Scripts)
			entry.ScriptLoaded = false;
	}

	// =========================================================================
	// OnCollision / OnCollisionEnd — call on all instances
	// =========================================================================
	void ScriptEngine::OnCollision(Scene* scene, Entity entity, Entity other,
		const Vector3& contactPoint, const Vector3& contactNormal)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		auto& scriptComp = entity.GetComponent<ScriptComponent>();

		for (size_t i = 0; i < it->second.size(); i++)
		{
			sol::table& self = it->second[i];
			if (!self.valid()) continue;

			sol::object onCollisionObj = self["OnCollisionBegin"];
			if (onCollisionObj.is<sol::protected_function>())
			{
				sol::protected_function onCollisionBegin = onCollisionObj;
				auto result = onCollisionBegin(self, other, contactPoint, contactNormal);
				if (!result.valid())
				{
					sol::error err = result;
					const String& path = (i < scriptComp.Scripts.size())
						? scriptComp.Scripts[i].ScriptPath : "unknown";
					CB_CORE_ERROR("Lua OnCollisionBegin error ({0}): {1}", path, err.what());
				}
			}
		}
	}

	void ScriptEngine::OnCollisionEnd(Scene* scene, Entity entity, Entity other)
	{
		if (!s_LuaState || !entity.HasComponent<ScriptComponent>())
			return;

		uint64_t uuid = static_cast<uint64_t>(entity.GetUUID());
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return;

		auto& scriptComp = entity.GetComponent<ScriptComponent>();

		for (size_t i = 0; i < it->second.size(); i++)
		{
			sol::table& self = it->second[i];
			if (!self.valid()) continue;

			sol::object onCollisionEndObj = self["OnCollisionEnd"];
			if (onCollisionEndObj.is<sol::protected_function>())
			{
				sol::protected_function onCollisionEnd = onCollisionEndObj;
				auto result = onCollisionEnd(self, other);
				if (!result.valid())
				{
					sol::error err = result;
					const String& path = (i < scriptComp.Scripts.size())
						? scriptComp.Scripts[i].ScriptPath : "unknown";
					CB_CORE_ERROR("Lua OnCollisionEnd error ({0}): {1}", path, err.what());
				}
			}
		}
	}

	void ScriptEngine::ReloadScript(const std::string& path)
	{
		s_ClassTables.erase(path);
		CB_CORE_INFO("Script hot-reload requested: {0}", path);
	}

	void ScriptEngine::ReloadAllScripts()
	{
		s_ClassTables.clear();
		s_KnownClassNames.clear();
		CB_CORE_INFO("Reloading all scripts...");
	}

	// =========================================================================
	// GameManager discovery
	// =========================================================================

	void ScriptEngine::PreloadBaseScripts()
	{
		if (!s_LuaState)
			return;

		// Embed GameManager base class directly so it works regardless of working directory
		static const char* s_GameManagerLua = R"lua(
GameManager = {
    __fields = {},
    __isGameManager = true
}

function GameManager:Extend()
    local child = {}
    child.__fields = {}
    for k, v in pairs(self.__fields) do child.__fields[k] = v end
    setmetatable(child, { __index = self })
    child.__isGameManager = true
    return child
end

function GameManager:OnCreate() end
function GameManager:OnUpdate(dt) end
function GameManager:OnDestroy() end
)lua";

		auto result = s_LuaState->script(s_GameManagerLua, sol::script_pass_on_error);
		if (result.valid())
		{
			s_KnownClassNames.insert("GameManager");
			CB_CORE_INFO("Preloaded GameManager base class");
		}
		else
		{
			sol::error err = result;
			CB_CORE_ERROR("Failed to preload GameManager base class: {0}", err.what());
		}
	}

	bool ScriptEngine::IsGameManagerScript(const std::string& scriptPath)
	{
		if (!s_LuaState || scriptPath.empty())
			return false;

		// Check cache first
		if (s_GameManagerScripts.count(scriptPath))
			return true;

		try
		{
			std::string className;
			sol::table classTable = GetOrLoadClassTable(scriptPath, className);
			if (!classTable.valid())
				return false;

			sol::object isGM = classTable["__isGameManager"];
			if (isGM.valid() && isGM.get_type() == sol::type::boolean && isGM.as<bool>())
			{
				s_GameManagerScripts.insert(scriptPath);
				return true;
			}
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("IsGameManagerScript error ({0}): {1}", scriptPath, e.what());
		}

		return false;
	}

	std::vector<std::string> ScriptEngine::GetGameManagerScripts()
	{
		return std::vector<std::string>(s_GameManagerScripts.begin(), s_GameManagerScripts.end());
	}

	void ScriptEngine::ScanForGameManagerScripts()
	{
		s_GameManagerScripts.clear();

		auto& registry = AssetManager::GetRegistry();
		for (auto& [uuid, metadata] : registry.GetAllAssets())
		{
			if (metadata.Type != AssetType::Script)
				continue;

			std::string scriptPath = "assets/" + metadata.FilePath.string();

			// Normalize path separators
			std::replace(scriptPath.begin(), scriptPath.end(), '\\', '/');

			IsGameManagerScript(scriptPath);
		}

		CB_CORE_INFO("Scanned for GameManager scripts, found {0}", s_GameManagerScripts.size());
	}

	// =========================================================================
	// File-scope helper implementations
	// =========================================================================

	static void InjectFields(sol::state& lua, sol::table& self, const std::vector<ScriptFieldDef>& fields)
	{
		for (const auto& field : fields)
		{
			switch (field.Type)
			{
				case ScriptFieldType::Float:
					self[field.Name] = field.FloatValue;
					break;
				case ScriptFieldType::Int:
					self[field.Name] = field.IntValue;
					break;
				case ScriptFieldType::Bool:
					self[field.Name] = field.BoolValue;
					break;
				case ScriptFieldType::String:
					self[field.Name] = field.StringValue;
					break;
				case ScriptFieldType::Color:
				{
					const Vector4& c = field.ColorValue;
					sol::table ct = lua.create_table();
					ct[1] = c.r; ct[2] = c.g; ct[3] = c.b; ct[4] = c.a;
					self[field.Name] = ct;
					break;
				}
				case ScriptFieldType::Vector3:
					self[field.Name] = field.Vector3Value;
					break;
			}
		}
	}

	static sol::table GetOrLoadClassTable(const std::string& scriptPath, std::string& outClassName)
	{
		// Resolve the path to an actual file on disk
		std::string resolvedPath = ResolveScriptPath(scriptPath);

		// Check cache with both original and resolved paths
		auto cacheIt = s_ClassTables.find(scriptPath);
		if (cacheIt != s_ClassTables.end())
		{
			outClassName = ""; // Caller should use entry.ClassName
			return cacheIt->second;
		}
		if (resolvedPath != scriptPath)
		{
			cacheIt = s_ClassTables.find(resolvedPath);
			if (cacheIt != s_ClassTables.end())
			{
				// Cache under original path too for future lookups
				s_ClassTables[scriptPath] = cacheIt->second;
				outClassName = "";
				return cacheIt->second;
			}
		}

		// Snapshot global table keys AND their table pointers BEFORE running
		// the script. This lets us detect both NEW globals and globals whose
		// value was REPLACED (script re-executed and created a new table).
		std::unordered_map<std::string, const void*> globalsBefore;
		{
			sol::table globals = s_LuaState->globals();
			for (auto& pair : globals)
			{
				if (pair.first.get_type() != sol::type::string) continue;
				std::string key = pair.first.as<std::string>();
				if (pair.second.get_type() == sol::type::table)
					globalsBefore[key] = pair.second.as<sol::table>().pointer();
				else
					globalsBefore[key] = nullptr;
			}
		}

		auto result = s_LuaState->script_file(resolvedPath, sol::script_pass_on_error);
		if (!result.valid())
		{
			sol::error err = result;
			CB_CORE_ERROR("Lua script error ({0}): {1}", resolvedPath, err.what());
			outClassName = "";
			return sol::table();
		}

		// First try the fast path: FindClassTable (works when s_KnownClassNames is clean)
		std::string className = FindClassTable();

		// If FindClassTable failed, diff globals to find the class.
		// Detects both NEW keys and keys whose table pointer CHANGED.
		if (className.empty())
		{
			sol::table globals = s_LuaState->globals();
			for (auto& pair : globals)
			{
				if (pair.first.get_type() != sol::type::string) continue;
				if (pair.second.get_type() != sol::type::table) continue;

				std::string key = pair.first.as<std::string>();
				sol::table tbl = pair.second;

				// Check if this is new or changed
				auto prevIt = globalsBefore.find(key);
				if (prevIt != globalsBefore.end() && prevIt->second == tbl.pointer())
					continue; // existed before with same pointer — not from this script

				sol::object fieldsObj = tbl["__fields"];
				if (fieldsObj.valid() && fieldsObj.get_type() == sol::type::table)
				{
					className = key;
					break;
				}
			}
		}

		if (className.empty())
		{
			outClassName = "";
			return sol::table();
		}

		sol::table classTable = (*s_LuaState)[className];

		s_ClassTables[scriptPath] = classTable;
		if (resolvedPath != scriptPath)
			s_ClassTables[resolvedPath] = classTable;
		s_KnownClassNames.insert(className);

		outClassName = className;
		return classTable;
	}

	static sol::object GetScriptInstanceByTable(UUID entityUUID, sol::table classTableArg, sol::this_state L)
	{
		uint64_t uuid = static_cast<uint64_t>(entityUUID);
		auto it = s_ScriptInstances.find(uuid);
		if (it == s_ScriptInstances.end())
			return sol::make_object(L, sol::nil);

		for (auto& self : it->second)
		{
			if (!self.valid()) continue;

			sol::table mt = self[sol::metatable_key];
			if (!mt.valid()) continue;

			sol::object indexObj = mt["__index"];
			if (!indexObj.valid() || indexObj.get_type() != sol::type::table)
				continue;

			sol::table instanceClass = indexObj;
			if (instanceClass.pointer() == classTableArg.pointer())
				return sol::make_object(L, self);
		}

		return sol::make_object(L, sol::nil);
	}

	static sol::table CreateScriptInstance(Scene* scene, Entity entity, ScriptEntry& entry)
	{
		if (entry.ScriptPath.empty())
			return sol::table();

		try
		{
			std::string className;
			sol::table classTable = GetOrLoadClassTable(entry.ScriptPath, className);

			sol::table self = s_LuaState->create_table();

			if (classTable.valid())
			{
				if (className.empty())
					className = entry.ClassName;
				else
					entry.ClassName = className;

				sol::table mt = s_LuaState->create_table();
				mt["__index"] = classTable;
				self[sol::metatable_key] = mt;

				if (!entry.FieldsParsed)
					ScriptEngine::ParseScriptFields(entry);

				InjectFields(*s_LuaState, self, entry.Fields);
			}
			else
			{
				// Legacy flat-function style
				std::string resolved = ResolveScriptPath(entry.ScriptPath);
				auto result = s_LuaState->script_file(resolved, sol::script_pass_on_error);
				if (!result.valid())
				{
					sol::error err = result;
					CB_CORE_ERROR("Lua script error ({0}): {1}", resolved, err.what());
					return sol::table();
				}

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
			}

			self["_entity"] = entity;
			self["_scene"] = scene;

			// self:GetScript(ClassTable) — cross-script access on same entity
			uint64_t entityUUID = static_cast<uint64_t>(entity.GetUUID());
			self["GetScript"] = [entityUUID](sol::table /*self*/, sol::table classTableArg, sol::this_state L) -> sol::object {
				return GetScriptInstanceByTable(UUID(entityUUID), classTableArg, L);
			};

			entry.ScriptLoaded = true;

			sol::object onCreateObj = self["OnCreate"];
			if (onCreateObj.is<sol::protected_function>())
			{
				sol::protected_function onCreate = onCreateObj;
				auto callResult = onCreate(self);
				if (!callResult.valid())
				{
					sol::error err = callResult;
					CB_CORE_ERROR("Lua OnCreate error ({0}): {1}", entry.ScriptPath, err.what());
				}
			}

			return self;
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("Lua script load error ({0}): {1}", entry.ScriptPath, e.what());
			return sol::table();
		}
	}
}
