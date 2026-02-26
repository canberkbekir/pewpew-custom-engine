#pragma once

#include "VoxelSubstance.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <string>

namespace CB
{
    // =========================================================================
    // VoxelSubstanceDatabase — singleton that maps VoxelMaterialType to
    // VoxelSubstanceProperties. Loaded from assets/config/voxel_substances.yaml
    // at startup. Call Load() once from Application or editor startup.
    // =========================================================================
    class VoxelSubstanceDatabase
    {
    public:
        // Load (or reload) from a yaml file path.
        // Path is relative to the asset directory or absolute.
        static void Load(const std::string& path);

        // Look up properties for a given material type.
        // Returns the fallback (indestructible) entry if not loaded or type invalid.
        static const VoxelSubstanceProperties& Get(VoxelMaterialType type);

        // Fallback used when no substance is configured — very high health, all resistances = 1.
        static const VoxelSubstanceProperties& GetFallback();

        static bool IsLoaded() { return s_Loaded; }

        // Resolve a substance name string ("Stone", "Wood", ...) to a material type.
        // Returns VoxelMaterialType::Stone as default if not recognised.
        static VoxelMaterialType ResolveSubstanceName(const std::string& name);

    private:
        // One slot per VoxelMaterialType enum value
        static VoxelSubstanceProperties s_Properties[static_cast<int>(VoxelMaterialType::Count)];
        // Indestructible fallback — very high health, all resistances = 1
        static VoxelSubstanceProperties s_Fallback;
        static bool                     s_Loaded;
    };
}
