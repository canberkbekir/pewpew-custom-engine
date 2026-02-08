#include "cbpch.h"
#include "VoxelTextureAsset.h"
#include "ProcessedMeshAsset.h"
#include "AssetManager.h"
#include "CBEngine/Core/Log.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
	VoxelMaterialType VoxelTextureAsset::GetMaterialType(uint32_t filledVoxelIndex, uint8_t paletteIndex) const
	{
		// Check per-voxel overrides first
		auto it = VoxelOverrides.find(filledVoxelIndex);
		if (it != VoxelOverrides.end())
			return it->second;

		// Fall back to palette mapping
		auto pit = PaletteMapping.find(paletteIndex);
		if (pit != PaletteMapping.end())
			return pit->second;

		return VoxelMaterialType::Stone;
	}

	void VoxelTextureAsset::SetMaterialType(uint32_t filledVoxelIndex, VoxelMaterialType type)
	{
		VoxelOverrides[filledVoxelIndex] = type;
	}

	void VoxelTextureAsset::SetPaletteType(uint8_t paletteIndex, VoxelMaterialType type)
	{
		PaletteMapping[paletteIndex] = type;
	}

	Ref<VoxelTextureAsset> VoxelTextureAsset::GenerateFromVmesh(const Ref<VoxelMeshAsset>& vmesh)
	{
		if (!vmesh)
			return nullptr;

		auto vtex = CreateRef<VoxelTextureAsset>();
		vtex->SourceVmeshUUID = vmesh->GetUUID();

		// Copy grid info
		const auto& grid = vmesh->GridData;
		vtex->GridSize = grid.size;
		vtex->VoxelCount = vmesh->VoxelCount;

		// Auto-detect material types from palette entries
		if (vmesh->HasPalette)
		{
			uint32_t usedCount = vmesh->Palette.GetUsedCount();
			for (uint8_t i = 0; i < usedCount; i++)
			{
				const auto& entry = vmesh->Palette.GetEntry(i);
				VoxelMaterialType detected = AutoDetectMaterialType(entry.Color);
				vtex->PaletteMapping[i] = detected;
			}
		}
		else
		{
			// No palette data - default everything to Stone
			vtex->PaletteMapping[0] = VoxelMaterialType::Stone;
		}

		return vtex;
	}

	void VoxelTextureAsset::GenerateMappingFromTexture(const Ref<VoxelMeshAsset>& vmesh,
	                                                    const std::filesystem::path& texturePath)
	{
		if (!vmesh)
			return;

		TextureSampler sampler;
		if (!sampler.Load(texturePath.string()))
		{
			CB_CORE_ERROR("VoxelTextureAsset: Failed to load texture: {0}", texturePath.string());
			return;
		}

		PaletteMapping.clear();

		if (vmesh->HasUVs && !vmesh->VoxelUVs.empty() && vmesh->HasPalette && !vmesh->PaletteIndices.empty())
		{
			// Sample the texture for each voxel, accumulate colors per palette index
			uint32_t paletteCount = vmesh->Palette.GetUsedCount();
			std::vector<Vector3> colorSums(paletteCount, Vector3(0.0f));
			std::vector<uint32_t> colorCounts(paletteCount, 0);

			size_t voxelCount = std::min(vmesh->VoxelUVs.size(), vmesh->PaletteIndices.size());
			for (size_t i = 0; i < voxelCount; i++)
			{
				uint8_t palIdx = vmesh->PaletteIndices[i];
				if (palIdx < paletteCount)
				{
					Vector3 sampledColor = sampler.SampleBilinear(vmesh->VoxelUVs[i]);
					colorSums[palIdx] += sampledColor;
					colorCounts[palIdx]++;
				}
			}

			// Auto-detect material type from averaged sampled color
			for (uint8_t i = 0; i < paletteCount; i++)
			{
				Vector3 avgColor = (colorCounts[i] > 0)
					? colorSums[i] / static_cast<float>(colorCounts[i])
					: vmesh->Palette.GetEntry(i).Color;

				float metallic = vmesh->Palette.GetEntry(i).Metallic;
				float roughness = vmesh->Palette.GetEntry(i).Roughness;

				PaletteMapping[i] = AutoDetectMaterialType(avgColor);
			}
		}
		else if (vmesh->HasPalette)
		{
			// No UVs - just sample the texture at evenly spaced points and use palette data
			uint32_t paletteCount = vmesh->Palette.GetUsedCount();
			for (uint8_t i = 0; i < paletteCount; i++)
			{
				const auto& entry = vmesh->Palette.GetEntry(i);
				PaletteMapping[i] = AutoDetectMaterialType(entry.Color);
			}
			CB_CORE_WARN("VoxelTextureAsset: VMesh has no UVs, falling back to palette-based detection");
		}

		CB_CORE_INFO("VoxelTextureAsset: Generated mapping from texture ({0} palette entries)", PaletteMapping.size());
	}

	bool VoxelTextureAsset::Save(const std::filesystem::path& filePath)
	{
		YAML::Emitter out;
		out << YAML::Comment("CBEngine Voxel Texture");
		out << YAML::BeginMap;

		out << YAML::Key << "version" << YAML::Value << s_VtexVersion;
		out << YAML::Key << "sourceVmesh" << YAML::Value << (uint64_t)SourceVmeshUUID;
		out << YAML::Key << "gridSize" << YAML::Value << YAML::Flow
			<< YAML::BeginSeq << GridSize.x << GridSize.y << GridSize.z << YAML::EndSeq;
		out << YAML::Key << "voxelCount" << YAML::Value << VoxelCount;

		// Palette mapping
		out << YAML::Key << "paletteMapping" << YAML::Value << YAML::BeginMap;
		for (const auto& [index, type] : PaletteMapping)
		{
			out << YAML::Key << (int)index << YAML::Value << VoxelMaterialTypeToString(type);
		}
		out << YAML::EndMap;

		// Voxel overrides (sparse)
		if (!VoxelOverrides.empty())
		{
			out << YAML::Key << "voxelOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, type] : VoxelOverrides)
			{
				out << YAML::Key << index << YAML::Value << VoxelMaterialTypeToString(type);
			}
			out << YAML::EndMap;
		}

		// Custom brushes
		if (!CustomBrushes.empty())
		{
			out << YAML::Key << "customBrushes" << YAML::Value << YAML::BeginSeq;
			for (const auto& brush : CustomBrushes)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "color" << YAML::Value << YAML::Flow
					<< YAML::BeginSeq << brush.Entry.Color.r << brush.Entry.Color.g << brush.Entry.Color.b << YAML::EndSeq;
				out << YAML::Key << "metallic" << YAML::Value << brush.Entry.Metallic;
				out << YAML::Key << "roughness" << YAML::Value << brush.Entry.Roughness;
				out << YAML::Key << "emission" << YAML::Value << brush.Entry.Emission;
				out << YAML::Key << "materialType" << YAML::Value << VoxelMaterialTypeToString(brush.MaterialType);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		// Palette index overrides (sparse: filledVoxelIndex -> paletteIndex)
		if (!PaletteIndexOverrides.empty())
		{
			out << YAML::Key << "paletteIndexOverrides" << YAML::Value << YAML::BeginMap;
			for (const auto& [index, palIdx] : PaletteIndexOverrides)
			{
				out << YAML::Key << index << YAML::Value << (int)palIdx;
			}
			out << YAML::EndMap;
		}

		out << YAML::EndMap;

		std::ofstream file(filePath);
		if (!file.is_open())
		{
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
		if (!std::filesystem::exists(filePath))
		{
			CB_CORE_ERROR("VoxelTextureAsset: File not found: {0}", filePath.string());
			return nullptr;
		}

		YAML::Node root;
		try
		{
			root = YAML::LoadFile(filePath.string());
		}
		catch (const YAML::Exception& e)
		{
			CB_CORE_ERROR("VoxelTextureAsset: YAML parse error in {0}: {1}", filePath.string(), e.what());
			return nullptr;
		}

		auto vtex = CreateRef<VoxelTextureAsset>();

		if (root["sourceVmesh"])
			vtex->SourceVmeshUUID = UUID(root["sourceVmesh"].as<uint64_t>());

		if (root["gridSize"] && root["gridSize"].IsSequence() && root["gridSize"].size() == 3)
		{
			vtex->GridSize.x = root["gridSize"][0].as<int>();
			vtex->GridSize.y = root["gridSize"][1].as<int>();
			vtex->GridSize.z = root["gridSize"][2].as<int>();
		}

		if (root["voxelCount"])
			vtex->VoxelCount = root["voxelCount"].as<uint64_t>();

		// Palette mapping
		if (root["paletteMapping"] && root["paletteMapping"].IsMap())
		{
			for (auto it = root["paletteMapping"].begin(); it != root["paletteMapping"].end(); ++it)
			{
				uint8_t index = static_cast<uint8_t>(it->first.as<int>());
				VoxelMaterialType type = VoxelMaterialTypeFromString(it->second.as<std::string>());
				vtex->PaletteMapping[index] = type;
			}
		}

		// Voxel overrides
		if (root["voxelOverrides"] && root["voxelOverrides"].IsMap())
		{
			for (auto it = root["voxelOverrides"].begin(); it != root["voxelOverrides"].end(); ++it)
			{
				uint32_t index = it->first.as<uint32_t>();
				VoxelMaterialType type = VoxelMaterialTypeFromString(it->second.as<std::string>());
				vtex->VoxelOverrides[index] = type;
			}
		}

		// Custom brushes (v2+)
		if (root["customBrushes"] && root["customBrushes"].IsSequence())
		{
			for (size_t i = 0; i < root["customBrushes"].size(); i++)
			{
				const auto& node = root["customBrushes"][i];
				CustomBrush brush;

				if (node["color"] && node["color"].IsSequence() && node["color"].size() == 3)
				{
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
				if (node["materialType"])
					brush.MaterialType = VoxelMaterialTypeFromString(node["materialType"].as<std::string>());

				vtex->CustomBrushes.push_back(brush);
			}
		}

		// Palette index overrides (v2+)
		if (root["paletteIndexOverrides"] && root["paletteIndexOverrides"].IsMap())
		{
			for (auto it = root["paletteIndexOverrides"].begin(); it != root["paletteIndexOverrides"].end(); ++it)
			{
				uint32_t index = it->first.as<uint32_t>();
				uint8_t palIdx = static_cast<uint8_t>(it->second.as<int>());
				vtex->PaletteIndexOverrides[index] = palIdx;
			}
		}

		vtex->m_Path = filePath;
		CB_CORE_INFO("VoxelTextureAsset: Loaded from {0}", filePath.string());
		return vtex;
	}

	bool VoxelTextureAsset::Reload()
	{
		if (m_Path.empty() || !std::filesystem::exists(m_Path))
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

		return true;
	}
}
