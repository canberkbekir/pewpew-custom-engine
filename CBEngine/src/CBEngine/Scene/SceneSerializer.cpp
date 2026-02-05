#include "cbpch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "CBEngine/Components/Components.h"
#include "CBEngine/Asset/AssetManager.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML
{
	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};
}

namespace CB
{
	static YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& ActiveScene)
		: m_Scene(ActiveScene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap;

		// Entity UUID
		out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetUUID();

		// TagComponent
		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;
			auto& tag = entity.GetComponent<TagComponent>();
			out << YAML::Key << "Tag" << YAML::Value << tag.Tag;
			out << YAML::EndMap;
		}

		// TransformComponent
		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;
			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << tc.Position;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
			out << YAML::Key << "Parent" << YAML::Value << (uint64_t)tc.Parent;
			out << YAML::EndMap;
		}

		// MeshRendererComponent
		if (entity.HasComponent<MeshRendererComponent>())
		{
			out << YAML::Key << "MeshRendererComponent";
			out << YAML::BeginMap;
			auto& mrc = entity.GetComponent<MeshRendererComponent>();
			out << YAML::Key << "MeshUUID" << YAML::Value << (uint64_t)mrc.MeshUUID;
			out << YAML::Key << "MaterialUUID" << YAML::Value << (uint64_t)mrc.MaterialUUID;
			out << YAML::Key << "ShaderUUID" << YAML::Value << (uint64_t)mrc.ShaderUUID;
			out << YAML::Key << "Visible" << YAML::Value << mrc.Visible;
			out << YAML::EndMap;
		}

		out << YAML::EndMap;
	}

	void SceneSerializer::Serialize(const String& FilePath)
	{
	}

	void SceneSerializer::Deserialize(const String& FilePath)
	{
	}

	void SceneSerializer::SerializeText(const String& FilePath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		// Scene name from filepath
		std::filesystem::path path(FilePath);
		out << YAML::Key << "Scene" << YAML::Value << path.stem().string();

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		auto view = m_Scene->GetEntitiesWith<IDComponent>();
		for (auto entityHandle : view)
		{
			Entity entity = { entityHandle, m_Scene.get() };
			SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(FilePath);
		if (!fout.is_open())
		{
			CB_CORE_ERROR("Failed to open scene file for writing: {0}", FilePath);
			return;
		}

		fout << out.c_str();

		if (fout.fail())
		{
			CB_CORE_ERROR("Failed to write scene file: {0}", FilePath);
			return;
		}

		CB_CORE_INFO("Serialized scene to: {0}", FilePath);
	}

	void SceneSerializer::DeserializeText(const String& FilePath)
	{
		std::ifstream fin(FilePath);
		if (!fin.is_open())
		{
			CB_CORE_ERROR("Failed to open scene file: {0}", FilePath);
			return;
		}

		std::stringstream ss;
		ss << fin.rdbuf();

		YAML::Node data;
		try
		{
			data = YAML::Load(ss.str());
		}
		catch (const YAML::ParserException& e)
		{
			CB_CORE_ERROR("Failed to parse scene file: {0}\n{1}", FilePath, e.what());
			return;
		}

		if (!data["Scene"])
		{
			CB_CORE_ERROR("Invalid scene file (missing 'Scene' key): {0}", FilePath);
			return;
		}

		auto entities = data["Entities"];
		if (!entities)
			return;

		for (auto entityNode : entities)
		{
			uint64_t uuid = entityNode["Entity"].as<uint64_t>();

			String name = "Entity";
			auto tagComponent = entityNode["TagComponent"];
			if (tagComponent)
				name = tagComponent["Tag"].as<std::string>();

			Entity entity = m_Scene->CreateEntityWithUUID(UUID(uuid), name);

			// TransformComponent (already added by CreateEntityWithUUID)
			auto transformComponent = entityNode["TransformComponent"];
			if (transformComponent)
			{
				auto& tc = entity.GetComponent<TransformComponent>();
				tc.Position = transformComponent["Position"].as<glm::vec3>();
				tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
				tc.Scale = transformComponent["Scale"].as<glm::vec3>();

				if (transformComponent["Parent"])
					tc.Parent = UUID(transformComponent["Parent"].as<uint64_t>());
			}

			// MeshRendererComponent
			auto meshRendererComponent = entityNode["MeshRendererComponent"];
			if (meshRendererComponent)
			{
				auto& mrc = entity.AddComponent<MeshRendererComponent>();
				mrc.MeshUUID = UUID(meshRendererComponent["MeshUUID"].as<uint64_t>());
				mrc.MaterialUUID = UUID(meshRendererComponent["MaterialUUID"].as<uint64_t>());
				mrc.ShaderUUID = UUID(meshRendererComponent["ShaderUUID"].as<uint64_t>());
				mrc.Visible = meshRendererComponent["Visible"].as<bool>();

				// Resolve UUIDs to loaded asset references
				if (mrc.MeshUUID.IsValid())
					mrc.MeshAsset = AssetManager::GetAsset<Mesh>(mrc.MeshUUID);
				if (mrc.MaterialUUID.IsValid())
					mrc.MaterialAsset = AssetManager::GetAsset<Material>(mrc.MaterialUUID);
				if (mrc.ShaderUUID.IsValid())
					mrc.ShaderAsset = AssetManager::GetAsset<Shader>(mrc.ShaderUUID);
			}
		}

		// Rebuild Children lists from Parent references
		auto view = m_Scene->GetEntitiesWith<TransformComponent>();
		for (auto entityHandle : view)
		{
			Entity entity = { entityHandle, m_Scene.get() };
			auto& tc = entity.GetComponent<TransformComponent>();
			if (tc.Parent.IsValid())
			{
				Entity parent = m_Scene->GetEntityByUUID(tc.Parent);
				if (parent)
				{
					parent.GetComponent<TransformComponent>().Children.push_back(entity.GetUUID());
				}
				else
				{
					CB_CORE_WARN("Entity '{0}' references missing parent UUID {1}, clearing parent",
						entity.GetName(), (uint64_t)tc.Parent);
					tc.Parent = UUID(0);
				}
			}
		}

		CB_CORE_INFO("Deserialized scene from: {0}", FilePath);
	}
}
