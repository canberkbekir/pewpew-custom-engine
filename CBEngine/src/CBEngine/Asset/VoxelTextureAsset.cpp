#include "cbpch.h"
#include "VoxelTextureAsset.h"
#include "ProcessedMeshAsset.h"
#include "AssetManager.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Renderer/Resources/Material.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
	SubstanceID VoxelTextureAsset::GetSubstanceID(uint32_t filledVoxelIndex, uint8_t paletteIndex) const
	{
		auto it = VoxelOverrides.find(filledVoxelIndex);
		if (it != VoxelOverrides.end())
			return it->second;

		auto pit = PaletteMapping.find(paletteIndex);
		if (pit != PaletteMapping.end())
			return pit->second;

		return "Stone";
	}

	VoxelMaterialType VoxelTextureAsset::GetMaterialType(uint32_t filledVoxelIndex, uint8_t paletteIndex) const
	{
		return VoxelMaterialTypeFromString(GetSubstanceID(filledVoxelIndex, paletteIndex));
	}

	void VoxelTextureAsset::SetSubstanceID(uint32_t filledVoxelIndex, const SubstanceID& id)
	{
		VoxelOverrides[filledVoxelIndex] = id;
	}

	void VoxelTextureAsset::SetMaterialType(uint32_t filledVoxelIndex, VoxelMaterialType type)
	{
		VoxelOverrides[filledVoxelIndex] = VoxelMaterialTypeToString(type);
	}

	void VoxelTextureAsset::SetPaletteSubstance(uint8_t paletteIndex, const SubstanceID& id)
	{
		PaletteMapping[paletteIndex] = id;
	}

	void VoxelTextureAsset::SetPaletteType(uint8_t paletteIndex, VoxelMaterialType type)
	{
		PaletteMapping[paletteIndex] = VoxelMaterialTypeToString(type);
	}

	Ref<VoxelTextureAsset> VoxelTextureAsset::GenerateFromVmesh(const Ref<VoxelMeshAsset>& vmesh)
	{
		if (!vmesh)
			return nullptr;

		auto vtex = CreateRef<VoxelTextureAsset>();
		vtex->SourceVmeshUUID = vmesh->GetUUID();

		const auto& grid = vmesh->GridData;
		vtex->GridSize = grid.size;
		vtex->VoxelCount = vmesh->VoxelCount;

		if (vmesh->HasPalette) {
			uint32_t usedCount = vmesh->Palette.GetUsedCount();
			for (uint8_t i = 0; i < usedCount; i++) {
				const auto& entry = vmesh->Palette.GetEntry(i);
				VoxelMaterialType detected = AutoDetectMaterialType(entry.Color);
				vtex->PaletteMapping[i] = VoxelMaterialTypeToString(detected);
			}
		}
		else {
			vtex->PaletteMapping[0] = "Stone";
		}

		if (!vmesh->MaterialSlots.empty() && vmesh->MaterialSlots[0].MaterialUUID.IsValid()) {
			auto mat = AssetManager::GetAsset<Material>(vmesh->MaterialSlots[0].MaterialUUID);
			if (mat)
				vtex->GenerateFromMaterial(mat, vmesh);
		}

		return vtex;
	}

	void VoxelTextureAsset::GenerateMappingFromTexture(const Ref<VoxelMeshAsset>& vmesh,
	                                                   const std::filesystem::path& texturePath)
	{
		if (!vmesh)
			return;

		TextureSampler sampler;
		if (!sampler.Load(texturePath.string())) {
			CB_CORE_ERROR("VoxelTextureAsset: Failed to load texture: {0}", texturePath.string());
			return;
		}

		PaletteMapping.clear();

		if (vmesh->HasUVs && !vmesh->VoxelUVs.empty() && vmesh->HasPalette && !vmesh->PaletteIndices.empty()) {
			uint32_t paletteCount = vmesh->Palette.GetUsedCount();
			std::vector<Vector3> colorSums(paletteCount, Vector3(0.0f));
			std::vector<uint32_t> colorCounts(paletteCount, 0);

			size_t voxelCount = std::min(vmesh->VoxelUVs.size(), vmesh->PaletteIndices.size());
			for (size_t i = 0; i < voxelCount; i++) {
				uint8_t palIdx = vmesh->PaletteIndices[i];
				if (palIdx < paletteCount) {
					Vector3 sampledColor = sampler.SampleBilinear(vmesh->VoxelUVs[i]);
					colorSums[palIdx] += sampledColor;
					colorCounts[palIdx]++;
				}
			}

			for (uint8_t i = 0; i < paletteCount; i++) {
				Vector3 avgColor = (colorCounts[i] > 0)
					                   ? colorSums[i] / static_cast<float>(colorCounts[i])
					                   : vmesh->Palette.GetEntry(i).Color;

				PaletteMapping[i] = VoxelMaterialTypeToString(AutoDetectMaterialType(avgColor));
			}
		}
		else if (vmesh->HasPalette) {
			uint32_t paletteCount = vmesh->Palette.GetUsedCount();
			for (uint8_t i = 0; i < paletteCount; i++) {
				const auto& entry = vmesh->Palette.GetEntry(i);
				PaletteMapping[i] = VoxelMaterialTypeToString(AutoDetectMaterialType(entry.Color));
			}
			CB_CORE_WARN("VoxelTextureAsset: VMesh has no UVs, falling back to palette-based detection");
		}

		CB_CORE_INFO("VoxelTextureAsset: Generated mapping from texture ({0} palette entries)", PaletteMapping.size());
	}

	void VoxelTextureAsset::GenerateFromMaterial(const Ref<Material>& material,
	                                             const Ref<VoxelMeshAsset>& vmesh)
	{
		if (!material || !vmesh)
			return;

		uint32_t usedCount = vmesh->HasPalette ? vmesh->Palette.GetUsedCount() : 0;
		if (usedCount == 0)
			return;

		if (material->HasAlbedoMap() && !material->GetPath().empty()) {
			// Placeholder for CPU-side texture sampling
		}

		HasMetallicOverrides = true;
		MetallicOverrides.clear();
		float matMetallic = material->GetMetallic();
		for (uint8_t i = 0; i < usedCount; i++)
			MetallicOverrides[i] = matMetallic;

		HasRoughnessOverrides = true;
		RoughnessOverrides.clear();
		float matRoughness = material->GetRoughness();
		for (uint8_t i = 0; i < usedCount; i++)
			RoughnessOverrides[i] = matRoughness;

		const Vector3& matAlbedo = material->GetAlbedo();
		if (matAlbedo != Vector3(1.0f, 1.0f, 1.0f)) {
			HasAlbedoOverrides = true;
			AlbedoOverrides.clear();
			for (uint8_t i = 0; i < usedCount; i++) {
				const auto& entry = vmesh->Palette.GetEntry(i);
				AlbedoOverrides[i] = entry.Color * matAlbedo;
			}
		}

		CB_CORE_INFO("VoxelTextureAsset: Generated PBR overrides from material (metallic={0}, roughness={1})",
		             matMetallic, matRoughness);
	}

	VoxelPalette VoxelTextureAsset::ApplyOverrides(const VoxelPalette& basePalette) const
	{
		VoxelPalette result;
		uint32_t usedCount = basePalette.GetUsedCount();

		for (uint8_t i = 0; i < usedCount; i++) {
			VoxelPaletteEntry entry = basePalette.GetEntry(i);

			if (HasMetallicOverrides) {
				auto it = MetallicOverrides.find(i);
				if (it != MetallicOverrides.end())
					entry.Metallic = it->second;
			}
			if (HasRoughnessOverrides) {
				auto it = RoughnessOverrides.find(i);
				if (it != RoughnessOverrides.end())
					entry.Roughness = it->second;
			}
			if (HasEmissionOverrides) {
				auto it = EmissionOverrides.find(i);
				if (it != EmissionOverrides.end())
					entry.Emission = it->second;
			}
			if (HasAlbedoOverrides) {
				auto it = AlbedoOverrides.find(i);
				if (it != AlbedoOverrides.end())
					entry.Color = it->second;
			}

			if (i == 0)
				result.SetEntry(0, entry);
			else
				result.AddEntry(entry);
		}

		return result;
	}

	bool VoxelTextureAsset::Save(const std::filesystem::path& filePath)
	{
		YAML::Emitter out;
		out << YAML::Comment("CBEngine Voxel Texture");
		out << YAML::BeginMap;

		out << YAML::Key << "version" << YAML::Value << s_VtexVersion;
		out << YAML::Key << "sourceVmesh" << YAML::Value << SourceVmeshUUID;
		out << YAML::Key << "gridSize" << YAML::Value << YAML::Flow
		<< YAML::BeginSeq << GridSize.x << GridSize.y << GridSize.z << YAML::EndSeq;
		out << YAML::Key << "voxelCount" << YAML::Value << VoxelCount;

		// Palette mapping (now SubstanceID strings)
		out << YAML::Key << "paletteMapping" << YAML::Value << YAML::BeginMap;
		for (const auto& [index, id] : PaletteMapping) {
			out << YAML::Key << static_cast<int>(index) << YAML::Value << id;
		}
		out << YAML::EndMap;

		// Voxel overrides (sparse, now SubstanceID strings)
		if (!VoxelOverrides.empty()) {
			out << YAML::Key << "voxelOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, id] : VoxelOverrides) {
				out << YAML::Key << index << YAML::Value << id;
			}
			out << YAML::EndMap;
		}

		// Custom brushes
		if (!CustomBrushes.empty()) {
			out << YAML::Key << "customBrushes" << YAML::Value << YAML::BeginSeq;
			for (const auto& brush : CustomBrushes) {
				out << YAML::BeginMap;
				out << YAML::Key << "color" << YAML::Value << YAML::Flow
				<< YAML::BeginSeq << brush.Entry.Color.r << brush.Entry.Color.g << brush.Entry.Color.b <<
				YAML::EndSeq;
				out << YAML::Key << "metallic" << YAML::Value << brush.Entry.Metallic;
				out << YAML::Key << "roughness" << YAML::Value << brush.Entry.Roughness;
				out << YAML::Key << "emission" << YAML::Value << brush.Entry.Emission;
				out << YAML::Key << "substance" << YAML::Value << brush.MaterialSubstance;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		// Palette index overrides
		if (!PaletteIndexOverrides.empty()) {
			out << YAML::Key << "paletteIndexOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, palIdx] : PaletteIndexOverrides) {
				out << YAML::Key << index << YAML::Value << static_cast<int>(palIdx);
			}
			out << YAML::EndMap;
		}

		// PBR overrides (v3+)
		if (HasMetallicOverrides && !MetallicOverrides.empty()) {
			out << YAML::Key << "hasMetallicOverrides" << YAML::Value << true;
			out << YAML::Key << "metallicOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, value] : MetallicOverrides)
				out << YAML::Key << static_cast<int>(index) << YAML::Value << value;
			out << YAML::EndMap;
		}

		if (HasRoughnessOverrides && !RoughnessOverrides.empty()) {
			out << YAML::Key << "hasRoughnessOverrides" << YAML::Value << true;
			out << YAML::Key << "roughnessOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, value] : RoughnessOverrides)
				out << YAML::Key << static_cast<int>(index) << YAML::Value << value;
			out << YAML::EndMap;
		}

		if (HasEmissionOverrides && !EmissionOverrides.empty()) {
			out << YAML::Key << "hasEmissionOverrides" << YAML::Value << true;
			out << YAML::Key << "emissionOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, value] : EmissionOverrides)
				out << YAML::Key << static_cast<int>(index) << YAML::Value << value;
			out << YAML::EndMap;
		}

		if (HasAlbedoOverrides && !AlbedoOverrides.empty()) {
			out << YAML::Key << "hasAlbedoOverrides" << YAML::Value << true;
			out << YAML::Key << "albedoOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, color] : AlbedoOverrides) {
				out << YAML::Key << static_cast<int>(index) << YAML::Value << YAML::Flow
				<< YAML::BeginSeq << color.r << color.g << color.b << YAML::EndSeq;
			}
			out << YAML::EndMap;
		}

		out << YAML::EndMap;

		std::ofstream file(filePath);
		if (!file.is_open()) {
			CB_CORE_ERROR("VoxelTextureAsset: Failed to save to {0}", filePath.string());
			return false;
		}

		file << out.c_str();
		file.close();

		CB_CORE_INFO("VoxelTextureAsset: Saved to {0}", filePath.string());
		return true;
	}

	Ref<VoxelTextureAsset> VoxelTextureAsset::Load(const std::filesystem::path& filePath)
	{
		if (!exists(filePath)) {
			CB_CORE_ERROR("VoxelTextureAsset: File not found: {0}", filePath.string());
			return nullptr;
		}

		YAML::Node root;
		try { root = YAML::LoadFile(filePath.string()); }
		catch (const YAML::Exception& e) {
			CB_CORE_ERROR("VoxelTextureAsset: YAML parse error in {0}: {1}", filePath.string(), e.what());
			return nullptr;
		}

		auto vtex = CreateRef<VoxelTextureAsset>();

		uint32_t version = 3;
		if (root["version"])
			version = root["version"].as<uint32_t>();

		if (root["sourceVmesh"])
			vtex->SourceVmeshUUID = UUID(root["sourceVmesh"].as<uint64_t>());

		if (root["gridSize"] && root["gridSize"].IsSequence() && root["gridSize"].size() == 3) {
			vtex->GridSize.x = root["gridSize"][0].as<int>();
			vtex->GridSize.y = root["gridSize"][1].as<int>();
			vtex->GridSize.z = root["gridSize"][2].as<int>();
		}

		if (root["voxelCount"])
			vtex->VoxelCount = root["voxelCount"].as<uint64_t>();

		// Palette mapping — v3 stored VoxelMaterialType names, v4+ stores SubstanceID strings
		// Both are string values, so the format is identical (auto-compatible)
		if (root["paletteMapping"] && root["paletteMapping"].IsMap()) {
			for (auto it = root["paletteMapping"].begin(); it != root["paletteMapping"].end(); ++it) {
				uint8_t index = static_cast<uint8_t>(it->first.as<int>());
				vtex->PaletteMapping[index] = it->second.as<std::string>();
			}
		}

		// Voxel overrides
		if (root["voxelOverrides"] && root["voxelOverrides"].IsMap()) {
			for (auto it = root["voxelOverrides"].begin(); it != root["voxelOverrides"].end(); ++it) {
				uint32_t index = it->first.as<uint32_t>();
				vtex->VoxelOverrides[index] = it->second.as<std::string>();
			}
		}

		// Custom brushes
		if (root["customBrushes"] && root["customBrushes"].IsSequence()) {
			for (size_t i = 0; i < root["customBrushes"].size(); i++) {
				const auto& node = root["customBrushes"][i];
				CustomBrush brush;

				if (node["color"] && node["color"].IsSequence() && node["color"].size() == 3) {
					brush.Entry.Color.r = node["color"][0].as<float>();
					brush.Entry.Color.g = node["color"][1].as<float>();
					brush.Entry.Color.b = node["color"][2].as<float>();
				}
				if (node["metallic"])
					brush.Entry.Metallic = node["metallic"].as<float>();
				if (node["roughness"])
					brush.Entry.Roughness = node["roughness"].as<float>();
				if (node["emission"])
					brush.Entry.Emission = node["emission"].as<float>();

				// v4+: "substance" key; legacy v3: "materialType" key
				if (node["substance"])
					brush.MaterialSubstance = node["substance"].as<std::string>();
				else if (node["materialType"])
					brush.MaterialSubstance = node["materialType"].as<std::string>();

				vtex->CustomBrushes.push_back(brush);
			}
		}

		// Palette index overrides
		if (root["paletteIndexOverrides"] && root["paletteIndexOverrides"].IsMap()) {
			for (auto it = root["paletteIndexOverrides"].begin(); it != root["paletteIndexOverrides"].end(); ++it) {
				uint32_t index = it->first.as<uint32_t>();
				uint8_t palIdx = static_cast<uint8_t>(it->second.as<int>());
				vtex->PaletteIndexOverrides[index] = palIdx;
			}
		}

		// PBR overrides (v3+)
		if (root["hasMetallicOverrides"]) {
			vtex->HasMetallicOverrides = root["hasMetallicOverrides"].as<bool>();
			if (root["metallicOverrides"] && root["metallicOverrides"].IsMap()) {
				for (auto it = root["metallicOverrides"].begin(); it != root["metallicOverrides"].end(); ++it) {
					uint8_t index = static_cast<uint8_t>(it->first.as<int>());
					vtex->MetallicOverrides[index] = it->second.as<float>();
				}
			}
		}

		if (root["hasRoughnessOverrides"]) {
			vtex->HasRoughnessOverrides = root["hasRoughnessOverrides"].as<bool>();
			if (root["roughnessOverrides"] && root["roughnessOverrides"].IsMap()) {
				for (auto it = root["roughnessOverrides"].begin(); it != root["roughnessOverrides"].end(); ++it) {
					uint8_t index = static_cast<uint8_t>(it->first.as<int>());
					vtex->RoughnessOverrides[index] = it->second.as<float>();
				}
			}
		}

		if (root["hasEmissionOverrides"]) {
			vtex->HasEmissionOverrides = root["hasEmissionOverrides"].as<bool>();
			if (root["emissionOverrides"] && root["emissionOverrides"].IsMap()) {
				for (auto it = root["emissionOverrides"].begin(); it != root["emissionOverrides"].end(); ++it) {
					uint8_t index = static_cast<uint8_t>(it->first.as<int>());
					vtex->EmissionOverrides[index] = it->second.as<float>();
				}
			}
		}

		if (root["hasAlbedoOverrides"]) {
			vtex->HasAlbedoOverrides = root["hasAlbedoOverrides"].as<bool>();
			if (root["albedoOverrides"] && root["albedoOverrides"].IsMap()) {
				for (auto it = root["albedoOverrides"].begin(); it != root["albedoOverrides"].end(); ++it) {
					uint8_t index = static_cast<uint8_t>(it->first.as<int>());
					auto seq = it->second;
					if (seq.IsSequence() && seq.size() == 3)
						vtex->AlbedoOverrides[index] = Vector3(seq[0].as<float>(), seq[1].as<float>(),
						                                       seq[2].as<float>());
				}
			}
		}

		vtex->m_Path = filePath;
		CB_CORE_INFO("VoxelTextureAsset: Loaded from {0}", filePath.string());
		return vtex;
	}

	bool VoxelTextureAsset::Reload()
	{
		if (m_Path.empty() || !exists(m_Path))
			return false;

		auto reloaded = Load(m_Path);
		if (!reloaded)
			return false;

		SourceVmeshUUID = reloaded->SourceVmeshUUID;
		GridSize = reloaded->GridSize;
		VoxelCount = reloaded->VoxelCount;
		PaletteMapping = std::move(reloaded->PaletteMapping);
		VoxelOverrides = std::move(reloaded->VoxelOverrides);
		CustomBrushes = std::move(reloaded->CustomBrushes);
		PaletteIndexOverrides = std::move(reloaded->PaletteIndexOverrides);

		HasMetallicOverrides = reloaded->HasMetallicOverrides;
		HasRoughnessOverrides = reloaded->HasRoughnessOverrides;
		HasEmissionOverrides = reloaded->HasEmissionOverrides;
		HasAlbedoOverrides = reloaded->HasAlbedoOverrides;
		MetallicOverrides = std::move(reloaded->MetallicOverrides);
		RoughnessOverrides = std::move(reloaded->RoughnessOverrides);
		EmissionOverrides = std::move(reloaded->EmissionOverrides);
		AlbedoOverrides = std::move(reloaded->AlbedoOverrides);

		return true;
	}
}
