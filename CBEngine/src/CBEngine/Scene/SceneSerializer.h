#pragma once
#include "Scene.h"
#include "Entity.h"
#include "CBEngine/Asset/Asset.h"

#include <vector>

namespace CB
{
    class SceneSerializer
    {
    public:
        SceneSerializer(const Ref<Scene>& ActiveScene);

        void Serialize(const String& FilePath);
        void Deserialize(const String& FilePath);

        //YAML format
        void SerializeText(const String& FilePath);
        void DeserializeText(const String& FilePath);

        // In-memory snapshot (for play mode save/restore)
        String SerializeToString();
        void DeserializeFromString(const String& yamlString);

        // Blueprint (prefab) support
        // Serializes root entity + all descendants to a YAML string
        static String SerializeEntityHierarchy(Entity root);

        // Instantiates blueprint YAML into current scene with NEW UUIDs.
        // Returns created entities (root first).
        // blueprintPath and blueprintUUID are stored on the root entity for tracking.
        std::vector<Entity> InstantiateBlueprint(const String& yamlData,
            const String& blueprintPath = "", UUID blueprintUUID = UUID(0));

    private:
        Ref<Scene> m_Scene;
    };
}
