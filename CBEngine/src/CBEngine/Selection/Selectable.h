#pragma once

#include "CBEngine/Core/UUID.h"

namespace CB
{
    enum class SelectableType
    {
        None,
        Asset,
        Entity
    };

    struct Selectable
    {
        SelectableType Type = SelectableType::None;
        UUID ID;

        Selectable() = default;

        Selectable(SelectableType type, UUID id)
            : Type(type), ID(id)
        {
        }

        bool IsValid() const
        {
            return Type != SelectableType::None && ID.IsValid();
        }

        bool operator==(const Selectable& other) const
        {
            return Type == other.Type && ID == other.ID;
        }

        bool operator!=(const Selectable& other) const
        {
            return !(*this == other);
        }

        // Factory methods
        static Selectable Asset(UUID uuid)
        {
            return Selectable(SelectableType::Asset, uuid);
        }

        static Selectable Entity(UUID uuid)
        {
            return Selectable(SelectableType::Entity, uuid);
        }

        static Selectable Invalid()
        {
            return Selectable();
        }
    };
}
