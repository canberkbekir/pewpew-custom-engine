#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/Core.h"

#include <filesystem>

namespace CB
{
	enum class AssetType : uint16_t
	{
		None = 0,
		Texture2D,
		Mesh,
		Shader,
		Material,
		Scene,
		ProcessedMesh,
		VoxelMesh
	};

	inline const char* AssetTypeToString(AssetType type)
	{
		switch (type)
		{
			case AssetType::None:          return "None";
			case AssetType::Texture2D:     return "Texture2D";
			case AssetType::Mesh:          return "Mesh";
			case AssetType::Shader:        return "Shader";
			case AssetType::Material:      return "Material";
			case AssetType::Scene:         return "Scene";
			case AssetType::ProcessedMesh: return "ProcessedMesh";
			case AssetType::VoxelMesh:     return "VoxelMesh";
		}
		return "Unknown";
	}

	inline AssetType AssetTypeFromString(const std::string& str)
	{
		if (str == "Texture2D")     return AssetType::Texture2D;
		if (str == "Mesh")          return AssetType::Mesh;
		if (str == "Shader")        return AssetType::Shader;
		if (str == "Material")      return AssetType::Material;
		if (str == "Scene")         return AssetType::Scene;
		if (str == "ProcessedMesh") return AssetType::ProcessedMesh;
		if (str == "VoxelMesh")     return AssetType::VoxelMesh;
		return AssetType::None;
	}

	inline AssetType AssetTypeFromExtension(const std::string& ext)
	{
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
			return AssetType::Texture2D;
		if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb")
			return AssetType::Mesh;
		if (ext == ".glsl" || ext == ".hlsl")
			return AssetType::Shader;
		if (ext == ".mat")
			return AssetType::Material;
		if (ext == ".scene")
			return AssetType::Scene;
		if (ext == ".mesh")
			return AssetType::ProcessedMesh;
		if (ext == ".vmesh")
			return AssetType::VoxelMesh;
		return AssetType::None;
	}

	class Asset
	{
	public:
		virtual ~Asset() = default;

		UUID GetUUID() const { return m_UUID; }
		AssetType GetAssetType() const { return m_Type; }
		const std::filesystem::path& GetPath() const { return m_Path; }

		virtual bool Reload() = 0;

		static AssetType GetStaticType() { return AssetType::None; }

	protected:
		UUID m_UUID;
		AssetType m_Type = AssetType::None;
		std::filesystem::path m_Path;

		friend class AssetManager;
		friend class AssetImporter;
	};
}
