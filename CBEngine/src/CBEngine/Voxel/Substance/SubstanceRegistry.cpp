#include "cbpch.h"
#include "SubstanceRegistry.h"
#include "SubstancePropertySchema.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <algorithm>

namespace CB
{
	// =========================================================================
	// Static storage
	// =========================================================================
	std::unordered_map<SubstanceID, SubstanceDefinition> SubstanceRegistry::s_Substances;
	bool SubstanceRegistry::s_Loaded = false;
	std::string SubstanceRegistry::s_LoadedPath;

	SubstanceDefinition SubstanceRegistry::s_Fallback = []
	{
		SubstanceDefinition def;
		def.ID = "__fallback__";
		def.DisplayColor = {0.5f, 0.5f, 0.5f};
		auto& p = def.Properties;
		p.Health = 1e9f;
		p.Hardness = 100.0f;
		p.ExplosionResistance = 1.0f;
		p.SliceResistance = 1.0f;
		p.TensileStrength = 1e9f;
		p.ImpactThreshold = 1e9f;
		p.Fracture = FractureBehavior::None;
		p.FragmentCount = 0;
		p.FragmentsHavePhysics = false;
		p.Flammable = false;
		p.PropagatesDamage = false;
		return def;
	}();

	// =========================================================================
	// YAML helpers
	// =========================================================================
	static float SafeFloat(const YAML::Node& node, const char* key, float def)
	{
		if (node[key]) return node[key].as<float>();
		return def;
	}

	static bool SafeBool(const YAML::Node& node, const char* key, bool def)
	{
		if (node[key]) return node[key].as<bool>();
		return def;
	}

	static int SafeInt(const YAML::Node& node, const char* key, int def)
	{
		if (node[key]) return node[key].as<int>();
		return def;
	}

	static VoxelDamageType DamageTypeFromString(const std::string& s)
	{
		if (s == "Impact") return VoxelDamageType::Impact;
		if (s == "Explosion") return VoxelDamageType::Explosion;
		if (s == "Slice") return VoxelDamageType::Slice;
		if (s == "Fire") return VoxelDamageType::Fire;
		if (s == "Acid") return VoxelDamageType::Acid;
		if (s == "Pressure") return VoxelDamageType::Pressure;
		if (s == "Structural") return VoxelDamageType::Structural;
		return VoxelDamageType::None;
	}

	static const char* DamageTypeToString(VoxelDamageType t)
	{
		switch (t) {
		case VoxelDamageType::Impact: return "Impact";
		case VoxelDamageType::Explosion: return "Explosion";
		case VoxelDamageType::Slice: return "Slice";
		case VoxelDamageType::Fire: return "Fire";
		case VoxelDamageType::Acid: return "Acid";
		case VoxelDamageType::Pressure: return "Pressure";
		case VoxelDamageType::Structural: return "Structural";
		default: return "None";
		}
	}

	static const char* FractureBehaviorToString(FractureBehavior f)
	{
		switch (f) {
		case FractureBehavior::None: return "NONE";
		case FractureBehavior::Chip: return "CHIP";
		case FractureBehavior::Crack: return "CRACK";
		case FractureBehavior::Shatter: return "SHATTER";
		case FractureBehavior::Crumble: return "CRUMBLE";
		default: return "NONE";
		}
	}

	// =========================================================================
	// Schema-driven property deserialization
	// =========================================================================
	static void DeserializeScalarProperties(const YAML::Node& node, VoxelSubstanceProperties& props)
	{
		const auto& schema = GetSubstancePropertySchema();
		for (const auto& meta : schema) {
			if (meta.Type == SubstancePropType::Custom || meta.Type == SubstancePropType::DamageTintMap)
				continue;

			if (!node[meta.YAMLKey])
				continue;

			char* base = reinterpret_cast<char*>(&props);
			switch (meta.Type) {
			case SubstancePropType::Float:
				*reinterpret_cast<float*>(base + meta.Offset) = node[meta.YAMLKey].as<float>();
				break;
			case SubstancePropType::Int:
				*reinterpret_cast<int*>(base + meta.Offset) = node[meta.YAMLKey].as<int>();
				break;
			case SubstancePropType::Bool:
				*reinterpret_cast<bool*>(base + meta.Offset) = node[meta.YAMLKey].as<bool>();
				break;
			case SubstancePropType::Enum:
			{
				auto str = node[meta.YAMLKey].as<std::string>();
				if (std::string(meta.YAMLKey) == "fracture_behavior")
					*reinterpret_cast<FractureBehavior*>(base + meta.Offset) = FractureBehaviorFromString(str);
				break;
			}
			case SubstancePropType::Color3:
			{
				const auto& seq = node[meta.YAMLKey];
				if (seq.IsSequence() && seq.size() == 3) {
					auto& v = *reinterpret_cast<Vector3*>(base + meta.Offset);
					v.x = seq[0].as<float>();
					v.y = seq[1].as<float>();
					v.z = seq[2].as<float>();
				}
				break;
			}
			default: break;
			}
		}
	}

	static void DeserializeDamageTints(const YAML::Node& node, VoxelSubstanceProperties& props)
	{
		if (!node["damage_tints"] || !node["damage_tints"].IsMap())
			return;

		for (auto it = node["damage_tints"].begin(); it != node["damage_tints"].end(); ++it) {
			auto typeName = it->first.as<std::string>();
			VoxelDamageType dmgType = DamageTypeFromString(typeName);
			if (dmgType == VoxelDamageType::None) continue;

			const auto& tNode = it->second;
			DamageTintConfig cfg;
			if (tNode["color"] && tNode["color"].IsSequence() && tNode["color"].size() == 3)
				cfg.Color = Vector3(tNode["color"][0].as<float>(),
				                    tNode["color"][1].as<float>(),
				                    tNode["color"][2].as<float>());
			cfg.Intensity = SafeFloat(tNode, "intensity", 0.0f);
			cfg.SpreadRadius = SafeInt(tNode, "spread_radius", 0);
			cfg.SpreadFalloff = SafeFloat(tNode, "spread_falloff", 0.5f);
			props.DamageTints[dmgType] = cfg;
		}
	}

	static void DeserializeBurnStages(const YAML::Node& node, VoxelSubstanceProperties& props)
	{
		if (!node["burn_stages"])
			return;

		const auto& bs = node["burn_stages"];
		if (bs["tints"] && bs["tints"].IsSequence()) {
			int count = std::min(static_cast<int>(bs["tints"].size()), VoxelSubstanceProperties::kBurnStageCount);
			for (int i = 0; i < count; ++i) {
				const auto& seq = bs["tints"][i];
				if (seq.IsSequence() && seq.size() == 3) {
					props.BurnStageTints[i] = Vector3(
						seq[0].as<float>(), seq[1].as<float>(), seq[2].as<float>());
				}
			}
		}

		if (bs["thresholds"] && bs["thresholds"].IsSequence()) {
			int count = std::min(static_cast<int>(bs["thresholds"].size()), 3);
			for (int i = 0; i < count; ++i)
				props.BurnStageThresholds[i] = bs["thresholds"][i].as<float>();
		}
	}

	static VoxelSubstanceProperties DeserializeProperties(const YAML::Node& node)
	{
		VoxelSubstanceProperties props;
		DeserializeScalarProperties(node, props);
		DeserializeDamageTints(node, props);
		DeserializeBurnStages(node, props);
		return props;
	}

	// =========================================================================
	// Schema-driven property serialization
	// =========================================================================
	static void SerializeScalarProperties(YAML::Emitter& out, const VoxelSubstanceProperties& props)
	{
		const auto& schema = GetSubstancePropertySchema();
		const char* base = reinterpret_cast<const char*>(&props);

		for (const auto& meta : schema) {
			if (meta.Type == SubstancePropType::Custom || meta.Type == SubstancePropType::DamageTintMap)
				continue;

			switch (meta.Type) {
			case SubstancePropType::Float:
				out << YAML::Key << meta.YAMLKey << YAML::Value
					<< *reinterpret_cast<const float*>(base + meta.Offset);
				break;
			case SubstancePropType::Int:
				out << YAML::Key << meta.YAMLKey << YAML::Value
					<< *reinterpret_cast<const int*>(base + meta.Offset);
				break;
			case SubstancePropType::Bool:
				out << YAML::Key << meta.YAMLKey << YAML::Value
					<< *reinterpret_cast<const bool*>(base + meta.Offset);
				break;
			case SubstancePropType::Enum:
			{
				if (std::string(meta.YAMLKey) == "fracture_behavior") {
					auto val = *reinterpret_cast<const FractureBehavior*>(base + meta.Offset);
					out << YAML::Key << meta.YAMLKey << YAML::Value << FractureBehaviorToString(val);
				}
				break;
			}
			case SubstancePropType::Color3:
			{
				const auto& v = *reinterpret_cast<const Vector3*>(base + meta.Offset);
				out << YAML::Key << meta.YAMLKey << YAML::Value << YAML::Flow
					<< YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
				break;
			}
			default: break;
			}
		}
	}

	static void SerializeDamageTints(YAML::Emitter& out, const VoxelSubstanceProperties& props)
	{
		if (props.DamageTints.empty())
			return;

		out << YAML::Key << "damage_tints" << YAML::Value << YAML::BeginMap;
		for (const auto& [dmgType, cfg] : props.DamageTints) {
			out << YAML::Key << DamageTypeToString(dmgType);
			out << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "color" << YAML::Value << YAML::Flow
				<< YAML::BeginSeq << cfg.Color.x << cfg.Color.y << cfg.Color.z << YAML::EndSeq;
			out << YAML::Key << "intensity" << YAML::Value << cfg.Intensity;
			out << YAML::Key << "spread_radius" << YAML::Value << cfg.SpreadRadius;
			out << YAML::Key << "spread_falloff" << YAML::Value << cfg.SpreadFalloff;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;
	}

	static void SerializeBurnStages(YAML::Emitter& out, const VoxelSubstanceProperties& props)
	{
		if (!props.Flammable)
			return;

		out << YAML::Key << "burn_stages" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "tints" << YAML::Value << YAML::BeginSeq;
		for (int i = 0; i < VoxelSubstanceProperties::kBurnStageCount; ++i) {
			out << YAML::Flow << YAML::BeginSeq
				<< props.BurnStageTints[i].x << props.BurnStageTints[i].y << props.BurnStageTints[i].z
				<< YAML::EndSeq;
		}
		out << YAML::EndSeq;

		out << YAML::Key << "thresholds" << YAML::Value << YAML::Flow
			<< YAML::BeginSeq
			<< props.BurnStageThresholds[0] << props.BurnStageThresholds[1] << props.BurnStageThresholds[2]
			<< YAML::EndSeq;

		out << YAML::EndMap;
	}

	// =========================================================================
	// SubstanceRegistry::Load
	// =========================================================================
	void SubstanceRegistry::Load(const std::string& path)
	{
		s_Substances.clear();
		s_LoadedPath = path;

		std::ifstream file(path);
		if (!file.is_open()) {
			CB_CORE_WARN("SubstanceRegistry: Could not open '{0}'. No substances loaded.", path);
			return;
		}

		YAML::Node root;
		try { root = YAML::Load(file); }
		catch (const YAML::Exception& e) {
			CB_CORE_ERROR("SubstanceRegistry: YAML parse error in '{0}': {1}", path, e.what());
			return;
		}

		const YAML::Node substances = root["substances"];
		if (!substances || !substances.IsMap()) {
			CB_CORE_WARN("SubstanceRegistry: No 'substances' map found in '{0}'", path);
			return;
		}

		int loaded = 0;
		for (auto it = substances.begin(); it != substances.end(); ++it) {
			auto name = it->first.as<std::string>();
			const auto& entry = it->second;

			SubstanceDefinition def;
			def.ID = name;

			// New format: display_color + pbr_defaults + properties sub-node
			if (entry["display_color"] && entry["display_color"].IsSequence() && entry["display_color"].size() == 3) {
				def.DisplayColor = Vector3(
					entry["display_color"][0].as<float>(),
					entry["display_color"][1].as<float>(),
					entry["display_color"][2].as<float>());
			}
			else {
				// Legacy format: infer display color from material type name
				VoxelMaterialType legacyType = VoxelMaterialTypeFromString(name);
				def.DisplayColor = VoxelMaterialTypeDisplayColor(legacyType);
			}

			if (entry["pbr_defaults"]) {
				const auto& pbr = entry["pbr_defaults"];
				def.PBRDefaults.Metallic = SafeFloat(pbr, "metallic", 0.0f);
				def.PBRDefaults.Roughness = SafeFloat(pbr, "roughness", 0.5f);
				def.PBRDefaults.Emission = SafeFloat(pbr, "emission", 0.0f);
			}
			else {
				// Legacy: infer from type name
				VoxelMaterialType legacyType = VoxelMaterialTypeFromString(name);
				def.PBRDefaults = GetDefaultPBRProperties(legacyType);
			}

			// Properties: new format nests under "properties", old format has them at top level
			if (entry["properties"] && entry["properties"].IsMap())
				def.Properties = DeserializeProperties(entry["properties"]);
			else
				def.Properties = DeserializeProperties(entry);

			s_Substances[name] = std::move(def);
			++loaded;
		}

		s_Loaded = true;
		CB_CORE_INFO("SubstanceRegistry: Loaded {0} substances from '{1}'", loaded, path);
	}

	// =========================================================================
	// SubstanceRegistry::Save
	// =========================================================================
	void SubstanceRegistry::Save(const std::string& path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "substances" << YAML::Value << YAML::BeginMap;

		// Sort IDs for deterministic output
		auto ids = GetAllIDs();
		std::sort(ids.begin(), ids.end());

		for (const auto& id : ids) {
			const auto& def = s_Substances[id];
			out << YAML::Key << id;
			out << YAML::Value << YAML::BeginMap;

			// Display color
			out << YAML::Key << "display_color" << YAML::Value << YAML::Flow
				<< YAML::BeginSeq << def.DisplayColor.x << def.DisplayColor.y << def.DisplayColor.z << YAML::EndSeq;

			// PBR defaults
			out << YAML::Key << "pbr_defaults" << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "metallic" << YAML::Value << def.PBRDefaults.Metallic;
			out << YAML::Key << "roughness" << YAML::Value << def.PBRDefaults.Roughness;
			out << YAML::Key << "emission" << YAML::Value << def.PBRDefaults.Emission;
			out << YAML::EndMap;

			// Properties
			out << YAML::Key << "properties" << YAML::Value << YAML::BeginMap;
			SerializeScalarProperties(out, def.Properties);
			SerializeBurnStages(out, def.Properties);
			SerializeDamageTints(out, def.Properties);
			out << YAML::EndMap;

			out << YAML::EndMap;
		}

		out << YAML::EndMap << YAML::EndMap;

		std::ofstream file(path);
		if (!file.is_open()) {
			CB_CORE_ERROR("SubstanceRegistry: Failed to save to '{0}'", path);
			return;
		}
		file << out.c_str();
	}

	// =========================================================================
	// Accessors
	// =========================================================================
	const VoxelSubstanceProperties& SubstanceRegistry::Get(const SubstanceID& id)
	{
		auto it = s_Substances.find(id);
		if (it != s_Substances.end())
			return it->second.Properties;
		return s_Fallback.Properties;
	}

	SubstanceDefinition& SubstanceRegistry::GetDefinition(const SubstanceID& id)
	{
		auto it = s_Substances.find(id);
		if (it != s_Substances.end())
			return it->second;
		return s_Fallback;
	}

	const SubstanceDefinition& SubstanceRegistry::GetFallback()
	{
		return s_Fallback;
	}

	VoxelSubstanceProperties& SubstanceRegistry::GetMutable(const SubstanceID& id)
	{
		auto it = s_Substances.find(id);
		if (it != s_Substances.end())
			return it->second.Properties;
		return s_Fallback.Properties;
	}

	std::vector<SubstanceID> SubstanceRegistry::GetAllIDs()
	{
		std::vector<SubstanceID> ids;
		ids.reserve(s_Substances.size());
		for (const auto& [id, _] : s_Substances)
			ids.push_back(id);
		std::sort(ids.begin(), ids.end());
		return ids;
	}

	bool SubstanceRegistry::Exists(const SubstanceID& id)
	{
		return s_Substances.count(id) > 0;
	}

	void SubstanceRegistry::Register(SubstanceDefinition def)
	{
		s_Substances[def.ID] = std::move(def);
	}

	void SubstanceRegistry::Remove(const SubstanceID& id)
	{
		s_Substances.erase(id);
	}

	void SubstanceRegistry::Duplicate(const SubstanceID& sourceID, const SubstanceID& newID)
	{
		auto it = s_Substances.find(sourceID);
		if (it == s_Substances.end()) return;

		SubstanceDefinition copy = it->second;
		copy.ID = newID;
		s_Substances[newID] = std::move(copy);
	}

	void SubstanceRegistry::Rename(const SubstanceID& oldID, const SubstanceID& newID)
	{
		auto it = s_Substances.find(oldID);
		if (it == s_Substances.end()) return;
		if (s_Substances.count(newID)) return; // target already exists

		SubstanceDefinition def = std::move(it->second);
		def.ID = newID;
		s_Substances.erase(it);
		s_Substances[newID] = std::move(def);
	}

	// =========================================================================
	// Legacy bridge
	// =========================================================================
	const VoxelSubstanceProperties& SubstanceRegistry::GetByLegacyType(VoxelMaterialType type)
	{
		return Get(VoxelMaterialTypeToString(type));
	}

	Vector3 SubstanceRegistry::GetDisplayColor(const SubstanceID& id)
	{
		auto it = s_Substances.find(id);
		if (it != s_Substances.end())
			return it->second.DisplayColor;
		return {0.5f, 0.5f, 0.5f};
	}
}
