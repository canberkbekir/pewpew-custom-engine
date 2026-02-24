#include "cbpch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "RuntimeComponentRegistry.h"

#include "CBEngine/Components/BlueprintInstanceComponent.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/CoreComponents.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace CB
{
    SceneSerializer::SceneSerializer(const Ref<Scene>& ActiveScene)
        : m_Scene(ActiveScene)
    {
    }

    static void SerializeEntity(YAML::Emitter& out, Entity entity, entt::registry& registry)
    {
        out << YAML::BeginMap;

        // Entity UUID (special case — not a component block)
        out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

        // Physics layer (only serialize if non-default)
        uint8_t layer = entity.GetComponent<IDComponent>().Layer;
        if (layer != 0)
            out << YAML::Key << "Layer" << YAML::Value << static_cast<int>(layer);

        // Serialize all auto-registered components
        RuntimeComponentRegistry::SerializeAll(out, registry, entity);

        // BlueprintInstanceComponent is handled manually to prevent it from
        // leaking into .blueprint files (would cause YAML-in-YAML bloat and
        // duplicate-component crashes on re-instantiation).
        if (entity.HasComponent<BlueprintInstanceComponent>())
        {
            auto& bic = entity.GetComponent<BlueprintInstanceComponent>();
            out << YAML::Key << BlueprintInstanceComponent::YAMLKey;
            out << YAML::BeginMap;
            bic.Serialize(out);
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
            Entity entity = {entityHandle, m_Scene.get()};
            SerializeEntity(out, entity, m_Scene->GetRegistry());
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

            // Read name before creating entity
            String name = "Entity";
            auto tagComponent = entityNode["TagComponent"];
            if (tagComponent)
                name = tagComponent["Tag"].as<std::string>();

            Entity entity = m_Scene->CreateEntityWithUUID(UUID(uuid), name);

            // Physics layer
            if (entityNode["Layer"])
                entity.GetComponent<IDComponent>().Layer = static_cast<uint8_t>(entityNode["Layer"].as<int>());

            // Deserialize all auto-registered components
            RuntimeComponentRegistry::DeserializeAll(entityNode, m_Scene->GetRegistry(), entity);

            // BlueprintInstanceComponent handled manually
            auto bicNode = entityNode[BlueprintInstanceComponent::YAMLKey];
            if (bicNode)
            {
                auto& bic = entity.AddComponent<BlueprintInstanceComponent>();
                bic.Deserialize(bicNode);
            }

            // Resolve asset UUIDs → loaded Ref<> assets
            RuntimeComponentRegistry::ResolveAssetsAll(m_Scene->GetRegistry(), entity);
        }

        // Rebuild Children lists from Parent references
        auto view = m_Scene->GetEntitiesWith<TransformComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene.get()};
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
                                 entity.GetName(), static_cast<uint64_t>(tc.Parent));
                    tc.Parent = UUID(0);
                }
            }
        }

        CB_CORE_INFO("Deserialized scene from: {0}", FilePath);
    }

    String SceneSerializer::SerializeToString()
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Snapshot";

        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        auto view = m_Scene->GetEntitiesWith<IDComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene.get()};
            SerializeEntity(out, entity, m_Scene->GetRegistry());
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        return String(out.c_str());
    }

    void SceneSerializer::DeserializeFromString(const String& yamlString)
    {
        YAML::Node data;
        try
        {
            data = YAML::Load(yamlString);
        }
        catch (const YAML::ParserException& e)
        {
            CB_CORE_ERROR("Failed to parse scene snapshot: {0}", e.what());
            return;
        }

        if (!data["Scene"])
        {
            CB_CORE_ERROR("Invalid scene snapshot (missing 'Scene' key)");
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

            if (entityNode["Layer"])
                entity.GetComponent<IDComponent>().Layer = static_cast<uint8_t>(entityNode["Layer"].as<int>());

            RuntimeComponentRegistry::DeserializeAll(entityNode, m_Scene->GetRegistry(), entity);

            auto bicNode = entityNode[BlueprintInstanceComponent::YAMLKey];
            if (bicNode)
            {
                auto& bic = entity.AddComponent<BlueprintInstanceComponent>();
                bic.Deserialize(bicNode);
            }

            RuntimeComponentRegistry::ResolveAssetsAll(m_Scene->GetRegistry(), entity);
        }

        // Rebuild Children lists from Parent references
        auto view = m_Scene->GetEntitiesWith<TransformComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene.get()};
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
                    tc.Parent = UUID(0);
                }
            }
        }

        CB_CORE_TRACE("Deserialized scene from in-memory snapshot");
    }

    // =========================================================================
    // Blueprint (prefab) support
    // =========================================================================

    static void CollectEntityHierarchy(Entity entity, std::vector<Entity>& out)
    {
        out.push_back(entity);
        if (entity.HasChildren())
        {
            for (Entity child : entity.GetChildren())
                CollectEntityHierarchy(child, out);
        }
    }

    String SceneSerializer::SerializeEntityHierarchy(Entity root)
    {
        std::vector<Entity> entities;
        CollectEntityHierarchy(root, entities);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Blueprint" << YAML::Value << root.GetName();
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        for (Entity entity : entities)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();
            uint8_t layer = entity.GetComponent<IDComponent>().Layer;
            if (layer != 0)
                out << YAML::Key << "Layer" << YAML::Value << static_cast<int>(layer);
            RuntimeComponentRegistry::SerializeAll(out, entity.GetScene()->GetRegistry(), entity);
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        return String(out.c_str());
    }

    std::vector<Entity> SceneSerializer::InstantiateBlueprint(const String& yamlData,
        const String& blueprintPath, UUID blueprintUUID)
    {
        std::vector<Entity> createdEntities;

        YAML::Node data;
        try
        {
            data = YAML::Load(yamlData);
        }
        catch (const YAML::ParserException& e)
        {
            CB_CORE_ERROR("InstantiateBlueprint: Failed to parse YAML: {0}", e.what());
            return createdEntities;
        }

        auto entities = data["Entities"];
        if (!entities)
            return createdEntities;

        // Build old UUID -> new UUID remap table
        std::unordered_map<uint64_t, UUID> uuidRemap;

        // First pass: create all entities with new UUIDs
        for (auto entityNode : entities)
        {
            uint64_t oldUUID = entityNode["Entity"].as<uint64_t>();
            UUID newUUID;

            String name = "Entity";
            auto tagComponent = entityNode["TagComponent"];
            if (tagComponent)
                name = tagComponent["Tag"].as<std::string>();

            Entity entity = m_Scene->CreateEntityWithUUID(newUUID, name);
            uuidRemap[oldUUID] = newUUID;
            createdEntities.push_back(entity);
        }

        // Second pass: deserialize components
        int idx = 0;
        for (auto entityNode : entities)
        {
            Entity entity = createdEntities[idx++];

            if (entityNode["Layer"])
                entity.GetComponent<IDComponent>().Layer = static_cast<uint8_t>(entityNode["Layer"].as<int>());

            RuntimeComponentRegistry::DeserializeAll(entityNode, m_Scene->GetRegistry(), entity);
            RuntimeComponentRegistry::ResolveAssetsAll(m_Scene->GetRegistry(), entity);
        }

        // Third pass: remap parent UUIDs to new UUIDs, rebuild children lists
        for (Entity entity : createdEntities)
        {
            if (!entity.HasComponent<TransformComponent>())
                continue;
            entity.GetComponent<TransformComponent>().Children.clear();
        }

        for (Entity entity : createdEntities)
        {
            if (!entity.HasComponent<TransformComponent>())
                continue;

            auto& tc = entity.GetComponent<TransformComponent>();

            if (tc.Parent.IsValid())
            {
                uint64_t oldParentUUID = static_cast<uint64_t>(tc.Parent);
                auto it = uuidRemap.find(oldParentUUID);
                if (it != uuidRemap.end())
                {
                    tc.Parent = it->second;

                    Entity parent = m_Scene->GetEntityByUUID(it->second);
                    if (parent)
                        parent.GetComponent<TransformComponent>().Children.push_back(entity.GetUUID());
                }
                else
                {
                    tc.Parent = UUID(0);
                }
            }
        }

        // Attach BlueprintInstanceComponent to the root entity for tracking
        if (!createdEntities.empty())
        {
            auto& bic = createdEntities[0].AddComponent<BlueprintInstanceComponent>();
            bic.BlueprintAssetUUID = blueprintUUID;
            bic.BlueprintPath = blueprintPath;
            bic.OriginalYAML = yamlData;
        }

        CB_CORE_INFO("Instantiated blueprint with {0} entities", createdEntities.size());
        return createdEntities;
    }
}
