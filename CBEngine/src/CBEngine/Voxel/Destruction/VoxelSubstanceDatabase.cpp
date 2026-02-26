#include "cbpch.h"
#include "VoxelSubstanceDatabase.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
	// =========================================================================
	// Static storage
	// =========================================================================
	VoxelSubstanceProperties VoxelSubstanceDatabase::s_Properties[static_cast<int>(VoxelMaterialType::Count)];
	bool VoxelSubstanceDatabase::s_Loaded = false;

	// Fallback: basically indestructible — used when yaml is missing or type unknown
	VoxelSubstanceProperties VoxelSubstanceDatabase::s_Fallback = []
	{
		VoxelSubstanceProperties p;
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
		return p;
	}();

	// =========================================================================
	// Helpers
	// =========================================================================
	static float SafeFloat(const YAML::Node& node,const char* key,float def)
	{
		if (node[key]) return node[key].as<float>();
		return def;
	}

	static bool SafeBool(const YAML::Node& node,const char* key,bool def)
	{
		if (node[key]) return node[key].as<bool>();
		return def;
	}

	static int SafeInt(const YAML::Node& node,const char* key,int def)
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

	static VoxelSubstanceProperties ParseEntry(const YAML::Node& node)
	{
		VoxelSubstanceProperties p;

		p.MassPerVoxel = SafeFloat(node, "mass_per_voxel", 0.02f);
		p.Health = SafeFloat(node, "health", 100.0f);
		p.Hardness = SafeFloat(node, "hardness", 50.0f);
		p.ExplosionResistance = SafeFloat(node, "explosion_resistance", 0.5f);
		p.SliceResistance = SafeFloat(node, "slice_resistance", 0.5f);
		p.TensileStrength = SafeFloat(node, "tensile_strength", 100.0f);
		p.ImpactThreshold = SafeFloat(node, "impact_threshold", 10.0f);

		p.Fracture = FractureBehaviorFromString(
			node["fracture_behavior"] ? node["fracture_behavior"].as<std::string>() : "CHIP");
		p.FractureThreshold = SafeFloat(node, "fracture_threshold", 0.3f);
		p.FragmentCount = SafeInt(node, "fragment_count", 5);
		p.FragmentsHavePhysics = SafeBool(node, "fragments_have_physics", true);

		p.Flammable = SafeBool(node, "flammable", false);
		p.IgnitionTemperature = SafeFloat(node, "ignition_temperature", 0.0f);
		p.BurnDuration = SafeFloat(node, "burn_duration", 0.0f);
		p.PropagatesDamage = SafeBool(node, "propagates_damage", false);
		p.DamageSpreadFactor = SafeFloat(node, "damage_spread_factor", 0.5f);

		// Parse damage tints
		if (node["damage_tints"] && node["damage_tints"].IsMap()) {
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
				p.DamageTints[dmgType] = cfg;
			}
		}

		return p;
	}

	// =========================================================================
	// VoxelSubstanceDatabase::Load
	// =========================================================================
	void VoxelSubstanceDatabase::Load(const std::string& path)
	{
		// Seed every slot with the fallback so any unspecified type is safe
		for (int i = 0; i < static_cast<int>(VoxelMaterialType::Count); ++i)
			s_Properties[i] = s_Fallback;

		std::ifstream file(path);
		if (!file.is_open()) {
			CB_CORE_WARN("VoxelSubstanceDatabase: Could not open '{0}'. All substances will use fallback.", path);
			return;
		}

		YAML::Node root;
		try { root = YAML::Load(file); }
		catch (const YAML::Exception& e) {
			CB_CORE_ERROR("VoxelSubstanceDatabase: YAML parse error in '{0}': {1}", path, e.what());
			return;
		}

		const YAML::Node substances = root["substances"];
		if (!substances || !substances.IsMap()) {
			CB_CORE_WARN("VoxelSubstanceDatabase: No 'substances' map found in '{0}'", path);
			return;
		}

		int loaded = 0;
		for (auto it = substances.begin(); it != substances.end(); ++it) {
			auto name = it->first.as<std::string>();
			VoxelMaterialType type = VoxelMaterialTypeFromString(name);

			int idx = static_cast<int>(type);
			if (idx < 0 || idx >= static_cast<int>(VoxelMaterialType::Count)) {
				CB_CORE_WARN("VoxelSubstanceDatabase: Unknown substance name '{0}', skipping.", name);
				continue;
			}

			s_Properties[idx] = ParseEntry(it->second);
			CB_CORE_TRACE("VoxelSubstanceDatabase: Loaded substance '{0}' (health={1})",
			              name, s_Properties[idx].Health);
			++loaded;
		}

		s_Loaded = true;
		CB_CORE_INFO("VoxelSubstanceDatabase: Loaded {0} substances from '{1}'", loaded, path);
	}

	// =========================================================================
	// VoxelSubstanceDatabase::Get
	// =========================================================================
	const VoxelSubstanceProperties& VoxelSubstanceDatabase::Get(VoxelMaterialType type)
	{
		int idx = static_cast<int>(type);
		if (!s_Loaded || idx < 0 || idx >= static_cast<int>(VoxelMaterialType::Count))
			return s_Fallback;
		return s_Properties[idx];
	}

	const VoxelSubstanceProperties& VoxelSubstanceDatabase::GetFallback() { return s_Fallback; }

	VoxelMaterialType VoxelSubstanceDatabase::ResolveSubstanceName(const std::string& name)
	{
		return VoxelMaterialTypeFromString(name);
	}

	VoxelSubstanceProperties& VoxelSubstanceDatabase::GetMutable(VoxelMaterialType type)
	{
		int idx = static_cast<int>(type);
		if (idx < 0 || idx >= static_cast<int>(VoxelMaterialType::Count))
			return s_Fallback;
		return s_Properties[idx];
	}

	void VoxelSubstanceDatabase::Save(const std::string& path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "substances" << YAML::Value << YAML::BeginMap;

		for (int i = 0; i < static_cast<int>(VoxelMaterialType::Count); ++i) {
			auto type = static_cast<VoxelMaterialType>(i);
			const auto& p = s_Properties[i];
			out << YAML::Key << VoxelMaterialTypeToString(type);
			out << YAML::Value << YAML::BeginMap;

			out << YAML::Key << "mass_per_voxel" << YAML::Value << p.MassPerVoxel;
			out << YAML::Key << "health" << YAML::Value << p.Health;
			out << YAML::Key << "hardness" << YAML::Value << p.Hardness;
			out << YAML::Key << "explosion_resistance" << YAML::Value << p.ExplosionResistance;
			out << YAML::Key << "slice_resistance" << YAML::Value << p.SliceResistance;
			out << YAML::Key << "tensile_strength" << YAML::Value << p.TensileStrength;
			out << YAML::Key << "impact_threshold" << YAML::Value << p.ImpactThreshold;

			const char* fracNames[] = {"NONE", "CHIP", "CRACK", "SHATTER", "CRUMBLE"};
			out << YAML::Key << "fracture_behavior" << YAML::Value << fracNames[static_cast<int>(p.Fracture)];
			out << YAML::Key << "fracture_threshold" << YAML::Value << p.FractureThreshold;
			out << YAML::Key << "fragment_count" << YAML::Value << p.FragmentCount;
			out << YAML::Key << "fragments_have_physics" << YAML::Value << p.FragmentsHavePhysics;

			out << YAML::Key << "flammable" << YAML::Value << p.Flammable;
			out << YAML::Key << "ignition_temperature" << YAML::Value << p.IgnitionTemperature;
			out << YAML::Key << "burn_duration" << YAML::Value << p.BurnDuration;
			out << YAML::Key << "propagates_damage" << YAML::Value << p.PropagatesDamage;
			out << YAML::Key << "damage_spread_factor" << YAML::Value << p.DamageSpreadFactor;

			if (!p.DamageTints.empty()) {
				out << YAML::Key << "damage_tints" << YAML::Value << YAML::BeginMap;
				for (const auto& [dmgType, cfg] : p.DamageTints) {
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

			out << YAML::EndMap;
		}

		out << YAML::EndMap << YAML::EndMap;

		std::ofstream file(path);
		file << out.c_str();
	}
}