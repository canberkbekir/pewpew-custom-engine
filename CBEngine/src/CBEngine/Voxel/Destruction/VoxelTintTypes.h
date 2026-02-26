#pragma once
#include <glm/glm.hpp>
#include <unordered_map>

namespace CB
{
    struct VoxelCoordHash
    {
        size_t operator()(const glm::ivec3& c) const noexcept
        {
            size_t h = std::hash<int>()(c.x);
            h ^= std::hash<int>()(c.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
            h ^= std::hash<int>()(c.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    struct VoxelTintEntry
    {
        glm::vec3 Color     = glm::vec3(1.0f);
        float     Intensity = 0.0f;
    };

    using VoxelTintMap = std::unordered_map<glm::ivec3, VoxelTintEntry, VoxelCoordHash>;
}
