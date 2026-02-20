#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"
#include "CBEngine/Math/CoreMath.h"

#include <string>
#include <vector>

namespace CB
{
	enum class ScriptFieldType
	{
		Float, Int, Bool, String, Color, Vector3
	};

	inline const char* ScriptFieldTypeToString(ScriptFieldType type)
	{
		switch (type)
		{
			case ScriptFieldType::Float:  return "float";
			case ScriptFieldType::Int:    return "int";
			case ScriptFieldType::Bool:   return "bool";
			case ScriptFieldType::String: return "string";
			case ScriptFieldType::Color:  return "color";
			case ScriptFieldType::Vector3:   return "vector3";
		}
		return "float";
	}

	inline ScriptFieldType ScriptFieldTypeFromString(const String& str)
	{
		if (str == "float")  return ScriptFieldType::Float;
		if (str == "int")    return ScriptFieldType::Int;
		if (str == "bool")   return ScriptFieldType::Bool;
		if (str == "string") return ScriptFieldType::String;
		if (str == "color")  return ScriptFieldType::Color;
		if (str == "vector3")   return ScriptFieldType::Vector3;
		return ScriptFieldType::Float;
	}

	struct ScriptFieldDef
	{
		String Name;
		ScriptFieldType Type = ScriptFieldType::Float;

		float FloatValue = 0.0f;
		int IntValue = 0;
		bool BoolValue = false;
		String StringValue;
		Vector4 ColorValue = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector3 Vector3Value = { 0.0f, 0.0f, 0.0f };

		float Min = 0.0f;
		float Max = 0.0f; // 0,0 = no constraint

		bool HasOverride = false;
	};

	struct ScriptComponent
	{
		String ScriptPath; // e.g., "assets/scripts/player.lua"
		String ClassName;  // Lua class table name (e.g. "CharacterController")
		std::vector<ScriptFieldDef> Fields;

		// Runtime-only (not serialized)
		bool ScriptLoaded = false;
		bool FieldsParsed = false;

		static constexpr auto YAMLKey = "ScriptComponent";

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "ScriptPath" << YAML::Value << ScriptPath;
			out << YAML::Key << "ClassName" << YAML::Value << ClassName;

			if (!Fields.empty())
			{
				out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
				for (const auto& field : Fields)
				{
					if (!field.HasOverride)
						continue;

					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << field.Name;
					out << YAML::Key << "Type" << YAML::Value << ScriptFieldTypeToString(field.Type);

					switch (field.Type)
					{
						case ScriptFieldType::Float:
							out << YAML::Key << "Value" << YAML::Value << field.FloatValue;
							break;
						case ScriptFieldType::Int:
							out << YAML::Key << "Value" << YAML::Value << field.IntValue;
							break;
						case ScriptFieldType::Bool:
							out << YAML::Key << "Value" << YAML::Value << field.BoolValue;
							break;
						case ScriptFieldType::String:
							out << YAML::Key << "Value" << YAML::Value << field.StringValue;
							break;
						case ScriptFieldType::Color:
							out << YAML::Key << "Value" << YAML::Value << YAML::Flow
								<< YAML::BeginSeq << field.ColorValue.r << field.ColorValue.g
								<< field.ColorValue.b << field.ColorValue.a << YAML::EndSeq;
							break;
						case ScriptFieldType::Vector3:
							out << YAML::Key << "Value" << YAML::Value << YAML::Flow
								<< YAML::BeginSeq << field.Vector3Value.x << field.Vector3Value.y
								<< field.Vector3Value.z << YAML::EndSeq;
							break;
					}

					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["ScriptPath"])
				ScriptPath = node["ScriptPath"].as<String>();
			if (node["ClassName"])
				ClassName = node["ClassName"].as<String>();

			if (node["Fields"])
			{
				for (auto fieldNode : node["Fields"])
				{
					ScriptFieldDef field;
					field.Name = fieldNode["Name"].as<String>();
					field.Type = ScriptFieldTypeFromString(fieldNode["Type"].as<String>());
					field.HasOverride = true;

					auto val = fieldNode["Value"];
					if (val)
					{
						switch (field.Type)
						{
							case ScriptFieldType::Float:
								field.FloatValue = val.as<float>();
								break;
							case ScriptFieldType::Int:
								field.IntValue = val.as<int>();
								break;
							case ScriptFieldType::Bool:
								field.BoolValue = val.as<bool>();
								break;
							case ScriptFieldType::String:
								field.StringValue = val.as<String>();
								break;
							case ScriptFieldType::Color:
								if (val.IsSequence() && val.size() >= 4)
								{
									field.ColorValue.r = val[0].as<float>();
									field.ColorValue.g = val[1].as<float>();
									field.ColorValue.b = val[2].as<float>();
									field.ColorValue.a = val[3].as<float>();
								}
								break;
							case ScriptFieldType::Vector3:
								if (val.IsSequence() && val.size() >= 3)
								{
									field.Vector3Value.x = val[0].as<float>();
									field.Vector3Value.y = val[1].as<float>();
									field.Vector3Value.z = val[2].as<float>();
								}
								break;
						}
					}

					Fields.push_back(field);
				}
			}
		}
	};
}
