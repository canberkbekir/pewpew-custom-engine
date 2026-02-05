#pragma once
#include "CBEngine/Utils/VoxelizerAPI.h"

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
    
    };
}
