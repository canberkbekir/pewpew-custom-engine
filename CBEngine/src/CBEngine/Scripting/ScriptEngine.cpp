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

	// =========================================================================
	// FindClassTable — scan globals for a table containing a __fields sub-table
	// =========================================================================
	std::string ScriptEngine::FindClassTable()
	{
		sol::table globals = s_LuaState->globals();
		for (auto& pair : globals)
		{
			if (pair.first.get_type() != sol::type::string)
				continue;
			if (pair.second.get_type() != sol::type::table)
				continue;

			std::string key = pair.first.as<std::string>();

			// Skip built-in Lua globals
			if (key == "string" || key == "table" || key == "math" || key == "io" ||
				key == "os" || key == "coroutine" || key == "debug" || key == "package" ||
				key == "utf8" || key == "_G" || key == "Log" || key == "Input" ||
				key == "Key" || key == "Mouse" || key == "Vec3" || key == "Vec4" ||
				key == "Float" || key == "Int" || key == "Bool" || key == "String" ||
				key == "Color" || key == "Vector3")
				continue;

			sol::table tbl = pair.second;
			sol::object fieldsObj = tbl["__fields"];
			if (fieldsObj.valid() && fieldsObj.get_type() == sol::type::table)
				return key;
		}
		return "";
	}

	// =========================================================================
	// ParseScriptFields — parse __fields from a script without play mode
	// =========================================================================
	void ScriptEngine::ParseScriptFields(ScriptComponent& script)
	{
		if (!s_LuaState || script.ScriptPath.empty())
			return;

		try
		{
			auto result = s_LuaState->script_file(script.ScriptPath, sol::script_pass_on_error);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("ParseScriptFields error ({0}): {1}", script.ScriptPath, err.what());
				script.FieldsParsed = true;
				return;
			}

			std::string className = FindClassTable();
			if (className.empty())
			{
				script.ClassName = "";
				script.FieldsParsed = true;
				// Clean up — no class table found
				return;
			}

			script.ClassName = className;

			sol::table classTable = (*s_LuaState)[className];
			sol::table fieldsTable = classTable["__fields"];

			// Save existing overrides keyed by name
			std::unordered_map<std::string, ScriptFieldDef> existingOverrides;
			for (auto& f : script.Fields)
			{
				if (f.HasOverride)
					existingOverrides[f.Name] = f;
			}

			script.Fields.clear();

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

				// Parse type
				sol::object typeObj = fieldDef["type"];
				if (typeObj.valid() && typeObj.get_type() == sol::type::string)
					def.Type = ScriptFieldTypeFromString(typeObj.as<std::string>());

				// Parse min/max
				sol::object minObj = fieldDef["min"];
				if (minObj.valid() && minObj.get_type() == sol::type::number)
					def.Min = minObj.as<float>();
				sol::object maxObj = fieldDef["max"];
				if (maxObj.valid() && maxObj.get_type() == sol::type::number)
					def.Max = maxObj.as<float>();

				// Parse default value
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

				// Merge with existing override if the user previously edited this field
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

				script.Fields.push_back(def);
			}

			// Clean up: remove the class table from Lua globals to avoid pollution
			(*s_LuaState)[className] = sol::nil;

			script.FieldsParsed = true;
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("ParseScriptFields exception ({0}): {1}", script.ScriptPath, e.what());
			script.FieldsParsed = true;
		}
	}

	// =========================================================================
	// CallOnValidate — invoke OnValidate on the class table when a field changes
	// =========================================================================
	void ScriptEngine::CallOnValidate(ScriptComponent& script, const std::string& changedField)
	{
		if (!s_LuaState || script.ScriptPath.empty() || script.ClassName.empty())
			return;

		try
		{
			auto result = s_LuaState->script_file(script.ScriptPath, sol::script_pass_on_error);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("CallOnValidate script error ({0}): {1}", script.ScriptPath, err.what());
				return;
			}

			std::string className = FindClassTable();
			if (className.empty())
				return;

			sol::table classTable = (*s_LuaState)[className];
			sol::object onValidateObj = classTable["OnValidate"];
			if (!onValidateObj.is<sol::protected_function>())
			{
				(*s_LuaState)[className] = sol::nil;
				return;
			}

			// Build a table of current field values
			sol::table fieldsTable = s_LuaState->create_table();
			for (const auto& field : script.Fields)
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

			// Call OnValidate(classTable, fieldsTable, changedFieldName)
			sol::protected_function onValidate = onValidateObj;
			auto callResult = onValidate(classTable, fieldsTable, changedField);
			if (!callResult.valid())
			{
				sol::error err = callResult;
				CB_CORE_ERROR("OnValidate error ({0}): {1}", script.ScriptPath, err.what());
			}
			else if (callResult.get_type() == sol::type::table)
			{
				// Apply corrected values back
				sol::table corrected = callResult;
				for (auto& field : script.Fields)
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

			(*s_LuaState)[className] = sol::nil;
		}
		catch (const std::exception& e)
		{
			CB_CORE_ERROR("CallOnValidate exception ({0}): {1}", script.ScriptPath, e.what());
		}
	}

	// =========================================================================
	// OnEntityCreate — class-method style with __fields support
	// =========================================================================
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

			// Find the class table (global with __fields)
			std::string className = FindClassTable();

			// Create per-entity instance table (self)
			sol::table self = s_LuaState->create_table();

			if (!className.empty())
			{
				// Class-method style: set metatable with __index pointing to class table
				sol::table classTable = (*s_LuaState)[className];

				sol::table mt = s_LuaState->create_table();
				mt["__index"] = classTable;
				self[sol::metatable_key] = mt;

				script.ClassName = className;

				// Parse fields if not already done
				if (!script.FieldsParsed)
					ParseScriptFields(script);

				// Inject field values into self (override or default)
				for (const auto& field : script.Fields)
				{
					switch (field.Type)
					{
						case ScriptFieldType::Float:
							self[field.Name] = field.HasOverride ? field.FloatValue : field.FloatValue;
							break;
						case ScriptFieldType::Int:
							self[field.Name] = field.HasOverride ? field.IntValue : field.IntValue;
							break;
						case ScriptFieldType::Bool:
							self[field.Name] = field.HasOverride ? field.BoolValue : field.BoolValue;
							break;
						case ScriptFieldType::String:
							self[field.Name] = field.HasOverride ? field.StringValue : field.StringValue;
							break;
						case ScriptFieldType::Color:
						{
							const Vector4& c = field.ColorValue;
							sol::table ct = s_LuaState->create_table();
							ct[1] = c.r; ct[2] = c.g; ct[3] = c.b; ct[4] = c.a;
							self[field.Name] = ct;
							break;
						}
						case ScriptFieldType::Vector3:
							self[field.Name] = field.Vector3Value;
							break;
					}
				}

				// Clean up global to avoid pollution between scripts
				(*s_LuaState)[className] = sol::nil;
			}
			else
			{
				// Legacy flat-function style: copy global functions into self
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

			// Store entity/scene references
			self["_entity"] = entity;
			self["_scene"] = scene;

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

	void ScriptEngine::OnCollision(Scene* scene, Entity entity, Entity other,
		const Vector3& contactPoint, const Vector3& contactNormal)
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
			auto result = onCollisionBegin(self, other, contactPoint, contactNormal);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua OnCollisionBegin error ({0}): {1}", script.ScriptPath, err.what());
			}
		}
	}

	void ScriptEngine::OnCollisionEnd(Scene* scene, Entity entity, Entity other)
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
		sol::object onCollisionEndObj = self["OnCollisionEnd"];
		if (onCollisionEndObj.is<sol::protected_function>())
		{
			sol::protected_function onCollisionEnd = onCollisionEndObj;
			auto result = onCollisionEnd(self, other);
			if (!result.valid())
			{
				sol::error err = result;
				CB_CORE_ERROR("Lua OnCollisionEnd error ({0}): {1}", script.ScriptPath, err.what());
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
