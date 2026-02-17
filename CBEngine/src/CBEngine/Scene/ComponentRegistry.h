#pragma once

#include "Entity.h"
#include "CBEngine/Components/Components.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include <yaml-cpp/yaml.h>

namespace CB
{
    // Type list for fold expressions
    template <typename... Ts>
    struct ComponentList
    {
    };

    // All components that participate in YAML serialization.
    // To add a new component: add Serialize/Deserialize to the struct, then add it here.
    using SerializableComponents = ComponentList<
        TagComponent,
        TransformComponent,
        MeshRendererComponent,
        VoxelRendererComponent,
        DirectionalLightComponent,
        RigidBodyComponent,
        ColliderComponent
    >;

    // SFINAE trait: does T have a ResolveAssets() member?
    template <typename T, typename = void>
    struct HasResolveAssets : std::false_type
    {
    };

    template <typename T>
    struct HasResolveAssets<T, std::void_t<decltype(std::declval<T&>().ResolveAssets())>> : std::true_type
    {
    };

    // --- Serialize helpers ---

    template <typename T>
    void TrySerialize(YAML::Emitter& out, Entity entity)
    {
        if (!entity.HasComponent<T>())
            return;

        auto& comp = entity.GetComponent<T>();
        out << YAML::Key << T::YAMLKey;
        out << YAML::BeginMap;
        comp.Serialize(out);
        out << YAML::EndMap;
    }

    template <typename... Ts>
    void SerializeAll(YAML::Emitter& out, Entity entity, ComponentList<Ts...>)
    {
        (TrySerialize<Ts>(out, entity), ...);
    }

    inline void SerializeAll(YAML::Emitter& out, Entity entity)
    {
        SerializeAll(out, entity, SerializableComponents{});
    }

    // --- Deserialize helpers ---

    template <typename T>
    void TryDeserialize(const YAML::Node& entityNode, Entity entity)
    {
        auto node = entityNode[T::YAMLKey];
        if (!node)
            return;

        // Some components (Tag, Transform) are already added by CreateEntityWithUUID
        T& comp = entity.HasComponent<T>()
                      ? entity.GetComponent<T>()
                      : entity.AddComponent<T>();
        comp.Deserialize(node);
    }

    template <typename... Ts>
    void DeserializeAll(const YAML::Node& entityNode, Entity entity, ComponentList<Ts...>)
    {
        (TryDeserialize<Ts>(entityNode, entity), ...);
    }

    inline void DeserializeAll(const YAML::Node& entityNode, Entity entity)
    {
        DeserializeAll(entityNode, entity, SerializableComponents{});
    }

    // --- ResolveAssets helpers ---

    template <typename T>
    void TryResolveAssets(Entity entity)
    {
        if constexpr (HasResolveAssets<T>::value)
        {
            if (entity.HasComponent<T>())
                entity.GetComponent<T>().ResolveAssets();
        }
    }

    template <typename... Ts>
    void ResolveAssetsAll(Entity entity, ComponentList<Ts...>)
    {
        (TryResolveAssets<Ts>(entity), ...);
    }

    inline void ResolveAssetsAll(Entity entity)
    {
        ResolveAssetsAll(entity, SerializableComponents{});
    }
}
