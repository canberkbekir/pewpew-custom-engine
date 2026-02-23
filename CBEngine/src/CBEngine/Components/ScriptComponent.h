#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Core/UUID.h"

#include <string>
#include <vector>

namespace CB
{
	// Handle for a .blueprint asset reference (BlueprintRef field, content browser drag).
	// For scene entity cloning use Scene:Instantiate(entity) directly.
	struct BlueprintHandle
	{
		UUID AssetUUID;
		BlueprintHandle() = default;
		explicit BlueprintHandle(UUID uuid) : AssetUUID(uuid) {}
		bool IsValid() const { return AssetUUID.IsValid(); }
	};

	enum class ScriptFieldType
	{
		Float, Int, Bool, String, Color, Vector3,
		// Reference types — editor drag-drop; injected as Entity/proxy/script at runtime
		EntityRef,    // entity handle
		ComponentRef, // built-in component proxy (subtype = component name, e.g. "RigidBody")
		ScriptRef,    // script instance (subtype = class name, e.g. "PlayerMovement")
		BlueprintRef, // .blueprint asset path (StringValue); spawned via Scene:InstantiateBlueprint()
		AudioClipRef, // .sfx asset; injected as AudioHandle at runtime
	};

	inline const char* ScriptFieldTypeToString(ScriptFieldType type)
	{
		switch (type)
		{
			case ScriptFieldType::Float:        return "float";
			case ScriptFieldType::Int:          return "int";
			case ScriptFieldType::Bool:         return "bool";
			case ScriptFieldType::String:       return "string";
			case ScriptFieldType::Color:        return "color";
			case ScriptFieldType::Vector3:      return "vector3";
			case ScriptFieldType::EntityRef:    return "entityref";
			case ScriptFieldType::ComponentRef: return "componentref";
			case ScriptFieldType::ScriptRef:    return "scriptref";
			case ScriptFieldType::BlueprintRef: return "blueprintref";
		case ScriptFieldType::AudioClipRef: return "audioclipref";
		}
		return "float";
	}

	inline ScriptFieldType ScriptFieldTypeFromString(const String& str)
	{
		if (str == "float")        return ScriptFieldType::Float;
		if (str == "int")          return ScriptFieldType::Int;
		if (str == "bool")         return ScriptFieldType::Bool;
		if (str == "string")       return ScriptFieldType::String;
		if (str == "color")        return ScriptFieldType::Color;
		if (str == "vector3")      return ScriptFieldType::Vector3;
		if (str == "entityref")    return ScriptFieldType::EntityRef;
		if (str == "componentref") return ScriptFieldType::ComponentRef;
		if (str == "scriptref")    return ScriptFieldType::ScriptRef;
		if (str == "blueprintref") return ScriptFieldType::BlueprintRef;
		if (str == "audioclipref") return ScriptFieldType::AudioClipRef;
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

		// For EntityRef / ComponentRef / ScriptRef
		UUID EntityRefValue = UUID(0);   // UUID of the referenced entity
		String EntityRefSubtype;          // component name (ComponentRef) or class name (ScriptRef)

		float Min = 0.0f;
		float Max = 0.0f; // 0,0 = no constraint

		bool HasOverride = false;
	};

	struct ScriptEntry
	{
		String ScriptPath; // e.g., "assets/scripts/player.lua"
		String ClassName;  // Lua class table name (e.g. "CharacterController")
		std::vector<ScriptFieldDef> Fields;

		// Runtime-only (not serialized)
		bool ScriptLoaded = false;
		bool FieldsParsed = false;

		ScriptEntry() = default;
		ScriptEntry(const ScriptEntry&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::BeginMap;
			out << YAML::Key << "ScriptPath" << YAML::Value << ScriptPath;
			out << YAML::Key << "ClassName" << YAML::Value << ClassName;

			if (!Fields.empty())
			{
				bool hasOverrides = false;
				for (const auto& field : Fields)
				{
					if (field.HasOverride) { hasOverrides = true; break; }
				}

				if (hasOverrides)
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
							case ScriptFieldType::EntityRef:
							case ScriptFieldType::ComponentRef:
							case ScriptFieldType::ScriptRef:
								out << YAML::Key << "Value" << YAML::Value << static_cast<uint64_t>(field.EntityRefValue);
								if (!field.EntityRefSubtype.empty())
									out << YAML::Key << "Subtype" << YAML::Value << field.EntityRefSubtype;
								break;
							case ScriptFieldType::BlueprintRef:
							case ScriptFieldType::AudioClipRef:
								out << YAML::Key << "Value" << YAML::Value << static_cast<uint64_t>(field.EntityRefValue);
								if (!field.EntityRefSubtype.empty())
									out << YAML::Key << "Subtype" << YAML::Value << field.EntityRefSubtype;
								break;
						}

						out << YAML::EndMap;
					}
					out << YAML::EndSeq;
				}
			}
			out << YAML::EndMap;
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
							case ScriptFieldType::EntityRef:
							case ScriptFieldType::ComponentRef:
							case ScriptFieldType::ScriptRef:
								field.EntityRefValue = UUID(val.as<uint64_t>());
								if (fieldNode["Subtype"])
									field.EntityRefSubtype = fieldNode["Subtype"].as<String>();
								break;
							case ScriptFieldType::BlueprintRef:
							case ScriptFieldType::AudioClipRef:
								field.EntityRefValue = UUID(val.as<uint64_t>());
								if (fieldNode["Subtype"])
									field.EntityRefSubtype = fieldNode["Subtype"].as<String>();
								break;
						}
					}

					Fields.push_back(field);
				}
			}
		}
	};

	struct ScriptComponent
	{
		std::vector<ScriptEntry> Scripts;

		static constexpr auto YAMLKey = "ScriptComponent";

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		void Serialize(YAML::Emitter& out) const
		{
			out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;
			for (const auto& entry : Scripts)
				entry.Serialize(out);
			out << YAML::EndSeq;
		}

		void Deserialize(const YAML::Node& node)
		{
			if (node["Scripts"])
			{
				// New multi-script format
				for (auto scriptNode : node["Scripts"])
				{
					ScriptEntry entry;
					entry.Deserialize(scriptNode);
					Scripts.push_back(entry);
				}
			}
			else if (node["ScriptPath"])
			{
				// Legacy single-script format -- auto-migrate
				ScriptEntry entry;
				entry.Deserialize(node);
				Scripts.push_back(entry);
			}
		}
	};
}
