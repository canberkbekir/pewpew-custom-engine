#pragma once

#include "CBEngine/Utils/YAMLHelpers.h"

namespace YAML { class Emitter; class Node; }

namespace CB
{
    // =========================================================================
    // VoxelAnchorComponent
    //
    // Marks an entity as a structural anchor point for the voxel destruction
    // system's structural integrity checks. Voxels connected to an anchor are
    // considered "grounded" and will not collapse.
    //
    // Usage:
    //   - Place on floor slabs, wall bases, support pillars.
    //   - Set DestructibleVoxelComponent::Anchor = AnchorMode::Manual on the
    //     voxel object you want to use manual anchor points.
    //   - Set Anchor = AnchorMode::BottomFace to auto-anchor the bottom row
    //     (no VoxelAnchorComponent needed in that case).
    //
    // All DestructibleVoxel entities within Radius of this anchor entity are
    // treated as having a grounded connection at that proximity point.
    // =========================================================================
    struct VoxelAnchorComponent
    {
        static constexpr auto YAMLKey = "VoxelAnchorComponent";

        // World-space sphere of influence — all voxel objects within this
        // radius of the anchor entity are affected.
        float Radius     = 1.0f;

        // When true, influences every DestructibleVoxel entity in range.
        // (Future: when false, only affect objects listed in a UUID set.)
        bool  AffectsAll = true;

        VoxelAnchorComponent() = default;
        VoxelAnchorComponent(const VoxelAnchorComponent&) = default;

        void Serialize(YAML::Emitter& out) const
        {
            out << YAML::Key << "Radius"     << YAML::Value << Radius;
            out << YAML::Key << "AffectsAll" << YAML::Value << AffectsAll;
        }

        void Deserialize(const YAML::Node& node)
        {
            if (node["Radius"])     Radius     = node["Radius"].as<float>();
            if (node["AffectsAll"]) AffectsAll = node["AffectsAll"].as<bool>();
        }
    };
}
