#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Utils/YAMLHelpers.h"

namespace CB
{
    struct IDComponent
    {
        UUID ID;
        uint8_t Layer = 0;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;

        IDComponent(UUID id) : ID(id)
        {
        }
    };

    struct TagComponent
    {
        std::string Tag;

        static constexpr auto YAMLKey = "TagComponent";

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;

        TagComponent(const String& tag) : Tag(tag)
        {
        }

        void Serialize(YAML::Emitter& out) const
        {
            out << YAML::Key << "Tag" << YAML::Value << Tag;
        }

        void Deserialize(const YAML::Node& node)
        {
            Tag = node["Tag"].as<std::string>();
        }
    };
}
