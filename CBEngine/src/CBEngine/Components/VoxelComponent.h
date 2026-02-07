#pragma once
#include "CBEngine/Utils/VoxelizerAPI.h"
#include "CBEngine/Utils/VoxelMaterialType.h"

#include <vector>

namespace CB
{
    struct VoxelComponent
    {
        VoxelMeshData Data;
        VoxelizeSettings Settings;
        UUID SourceMeshUUID;
        UUID SourceTextureUUID;
        bool Dirty;
        bool AutoVoxelize;

        // Runtime material type data (populated from VoxelTextureAsset)
        std::vector<uint8_t> MaterialTypes;

        VoxelMaterialType GetMaterialTypeAt(uint32_t filledVoxelIndex) const
        {
            if (filledVoxelIndex < MaterialTypes.size())
                return static_cast<VoxelMaterialType>(MaterialTypes[filledVoxelIndex]);
            return VoxelMaterialType::Stone;
        }
    };
}
