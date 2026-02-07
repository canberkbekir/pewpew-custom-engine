#include "cbpch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "ComponentRegistry.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
	SceneSerializer::SceneSerializer(const Ref<Scene>& ActiveScene)
		: m_Scene(ActiveScene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap;

		// Entity UUID (special case — not a component block)
		out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetUUID();

		// Serialize all registered components via fold expression
		SerializeAll(out, entity);

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

			// Read name before creating entity (CreateEntityWithUUID needs it)
			String name = "Entity";
			auto tagComponent = entityNode["TagComponent"];
			if (tagComponent)
				name = tagComponent["Tag"].as<std::string>();

			Entity entity = m_Scene->CreateEntityWithUUID(UUID(uuid), name);

			// Deserialize all registered components via fold expression
			DeserializeAll(entityNode, entity);

			// Resolve asset UUIDs to loaded assets
			ResolveAssetsAll(entity);
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
