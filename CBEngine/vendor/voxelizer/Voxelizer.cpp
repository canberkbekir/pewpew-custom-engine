/*
 * Voxelizer.cpp
 * Simplified mesh voxelization implementation
 *
 * Original: https://github.com/topskychen/voxelizer
 * Simplified for CBEngine
 */

#include "Voxelizer.h"
#include "TriBox.h"
#include <algorithm>
#include <queue>
#include <cmath>
#include <intrin.h> // __popcnt64

namespace voxelizer {

// ============================================================================
// VoxelGrid Implementation
// ============================================================================

bool VoxelGrid::IsFilled(int x, int y, int z) const
{
    if (!IsValidCoord(x, y, z)) return false;
    return IsFilled(CoordToIndex(x, y, z));
}

bool VoxelGrid::IsFilled(const glm::ivec3& coord) const
{
    return IsFilled(coord.x, coord.y, coord.z);
}

bool VoxelGrid::IsFilled(uint64_t index) const
{
    if (index >= totalVoxels) return false;
    uint64_t arrayIndex = index / 64;
    uint64_t bitIndex = index % 64;
    return (data[arrayIndex] & (uint64_t(1) << bitIndex)) != 0;
}

void VoxelGrid::SetFilled(int x, int y, int z)
{
    if (!IsValidCoord(x, y, z)) return;
    SetFilled(CoordToIndex(x, y, z));
}

void VoxelGrid::SetFilled(const glm::ivec3& coord)
{
    SetFilled(coord.x, coord.y, coord.z);
}

void VoxelGrid::SetFilled(uint64_t index)
{
    if (index >= totalVoxels) return;
    uint64_t arrayIndex = index / 64;
    uint64_t bitIndex = index % 64;
    data[arrayIndex] |= (uint64_t(1) << bitIndex);
}

void VoxelGrid::Clear(int x, int y, int z)
{
    if (!IsValidCoord(x, y, z)) return;
    Clear(CoordToIndex(x, y, z));
}

void VoxelGrid::Clear(uint64_t index)
{
    if (index >= totalVoxels) return;
    uint64_t arrayIndex = index / 64;
    uint64_t bitIndex = index % 64;
    data[arrayIndex] &= ~(uint64_t(1) << bitIndex);
}

uint64_t VoxelGrid::CoordToIndex(int x, int y, int z) const
{
    return static_cast<uint64_t>(x) * size.y * size.z +
           static_cast<uint64_t>(y) * size.z +
           static_cast<uint64_t>(z);
}

uint64_t VoxelGrid::CoordToIndex(const glm::ivec3& coord) const
{
    return CoordToIndex(coord.x, coord.y, coord.z);
}

glm::ivec3 VoxelGrid::IndexToCoord(uint64_t index) const
{
    int z = static_cast<int>(index % size.z);
    index /= size.z;
    int y = static_cast<int>(index % size.y);
    int x = static_cast<int>(index / size.y);
    return glm::ivec3(x, y, z);
}

glm::ivec3 VoxelGrid::WorldToVoxel(const glm::vec3& worldPos) const
{
    glm::vec3 local = (worldPos - origin) / voxelSize;
    return glm::ivec3(
        static_cast<int>(std::floor(local.x)),
        static_cast<int>(std::floor(local.y)),
        static_cast<int>(std::floor(local.z))
    );
}

glm::vec3 VoxelGrid::VoxelToWorld(const glm::ivec3& voxelCoord) const
{
    return origin + glm::vec3(voxelCoord) * voxelSize;
}

glm::vec3 VoxelGrid::VoxelCenterToWorld(const glm::ivec3& voxelCoord) const
{
    return origin + (glm::vec3(voxelCoord) + 0.5f) * voxelSize;
}

bool VoxelGrid::IsValidCoord(int x, int y, int z) const
{
    return x >= 0 && x < size.x &&
           y >= 0 && y < size.y &&
           z >= 0 && z < size.z;
}

bool VoxelGrid::IsValidCoord(const glm::ivec3& coord) const
{
    return IsValidCoord(coord.x, coord.y, coord.z);
}

uint64_t VoxelGrid::CountFilled() const
{
    uint64_t count = 0;
    for (size_t i = 0; i < data.size(); ++i)
        count += __popcnt64(data[i]);
    return count;
}

std::vector<glm::ivec3> VoxelGrid::GetFilledCoords() const
{
    std::vector<glm::ivec3> coords;
    coords.reserve(CountFilled());
    for (uint64_t i = 0; i < totalVoxels; ++i)
    {
        if (IsFilled(i))
        {
            coords.push_back(IndexToCoord(i));
        }
    }
    return coords;
}

std::vector<glm::vec3> VoxelGrid::GetFilledPositions() const
{
    std::vector<glm::vec3> positions;
    auto coords = GetFilledCoords();
    positions.reserve(coords.size());
    for (const auto& coord : coords)
    {
        positions.push_back(VoxelCenterToWorld(coord));
    }
    return positions;
}

// ============================================================================
// Voxelizer Implementation
// ============================================================================

Voxelizer::Voxelizer(int gridSize)
    : m_GridSize(gridSize), m_UseVoxelSize(false)
{
}

Voxelizer::Voxelizer(float voxelSize)
    : m_VoxelSize(voxelSize), m_UseVoxelSize(true)
{
}

void Voxelizer::ComputeBounds(const std::vector<Triangle>& triangles,
                               glm::vec3& outMin, glm::vec3& outMax)
{
    if (triangles.empty())
    {
        outMin = outMax = glm::vec3(0.0f);
        return;
    }

    outMin = glm::vec3(std::numeric_limits<float>::max());
    outMax = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& tri : triangles)
    {
        outMin = glm::min(outMin, tri.v0);
        outMin = glm::min(outMin, tri.v1);
        outMin = glm::min(outMin, tri.v2);
        outMax = glm::max(outMax, tri.v0);
        outMax = glm::max(outMax, tri.v1);
        outMax = glm::max(outMax, tri.v2);
    }
}

VoxelGrid Voxelizer::Voxelize(const std::vector<Triangle>& triangles, bool solid)
{
    VoxelGrid grid;

    if (triangles.empty())
    {
        grid.size = glm::ivec3(0);
        grid.totalVoxels = 0;
        return grid;
    }

    // Compute mesh bounds
    glm::vec3 meshMin, meshMax;
    ComputeBounds(triangles, meshMin, meshMax);

    // Add padding
    meshMin -= glm::vec3(m_Padding);
    meshMax += glm::vec3(m_Padding);

    glm::vec3 meshSize = meshMax - meshMin;

    // Compute grid dimensions
    if (m_UseVoxelSize)
    {
        grid.voxelSize = glm::vec3(m_VoxelSize);
        grid.size = glm::ivec3(glm::ceil(meshSize / m_VoxelSize));
    }
    else
    {
        float maxDim = std::max({meshSize.x, meshSize.y, meshSize.z});
        float voxelSize = maxDim / static_cast<float>(m_GridSize);
        grid.voxelSize = glm::vec3(voxelSize);
        grid.size = glm::ivec3(glm::ceil(meshSize / voxelSize));
    }

    // Ensure minimum size
    grid.size = glm::max(grid.size, glm::ivec3(1));
    grid.origin = meshMin;
    grid.totalVoxels = static_cast<uint64_t>(grid.size.x) *
                       static_cast<uint64_t>(grid.size.y) *
                       static_cast<uint64_t>(grid.size.z);

    // Allocate voxel data (bit-packed)
    uint64_t dataSize = (grid.totalVoxels + 63) / 64;
    grid.data.resize(dataSize, 0);

    // Surface voxelization
    VoxelizeSurface(triangles, grid);

    // Solid voxelization (fill interior)
    if (solid)
    {
        VoxelizeSolid(grid);
    }

    return grid;
}

VoxelGrid Voxelizer::Voxelize(const std::vector<glm::vec3>& vertices,
                               const std::vector<uint32_t>& indices,
                               bool solid)
{
    std::vector<Triangle> triangles;
    triangles.reserve(indices.size() / 3);

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        triangles.emplace_back(
            vertices[indices[i]],
            vertices[indices[i + 1]],
            vertices[indices[i + 2]]
        );
    }

    return Voxelize(triangles, solid);
}

void Voxelizer::VoxelizeSurface(const std::vector<Triangle>& triangles, VoxelGrid& grid)
{
    float halfSize[3] = {
        grid.voxelSize.x * 0.5f,
        grid.voxelSize.y * 0.5f,
        grid.voxelSize.z * 0.5f
    };

    for (const auto& tri : triangles)
    {
        // Compute triangle AABB in voxel coordinates
        glm::vec3 triMin = glm::min(glm::min(tri.v0, tri.v1), tri.v2);
        glm::vec3 triMax = glm::max(glm::max(tri.v0, tri.v1), tri.v2);

        glm::ivec3 voxMin = grid.WorldToVoxel(triMin);
        glm::ivec3 voxMax = grid.WorldToVoxel(triMax);

        // Clamp to grid bounds
        voxMin = glm::max(voxMin, glm::ivec3(0));
        voxMax = glm::min(voxMax, grid.size - 1);

        // Prepare triangle vertices for TriBoxOverlap
        float triverts[3][3] = {
            {tri.v0.x, tri.v0.y, tri.v0.z},
            {tri.v1.x, tri.v1.y, tri.v1.z},
            {tri.v2.x, tri.v2.y, tri.v2.z}
        };

        // Test each voxel in the triangle's AABB
        for (int x = voxMin.x; x <= voxMax.x; ++x)
        {
            for (int y = voxMin.y; y <= voxMax.y; ++y)
            {
                for (int z = voxMin.z; z <= voxMax.z; ++z)
                {
                    // Get voxel center in world space
                    glm::vec3 center = grid.VoxelCenterToWorld(glm::ivec3(x, y, z));
                    float boxcenter[3] = {center.x, center.y, center.z};

                    if (TriBoxOverlap(boxcenter, halfSize, triverts))
                    {
                        grid.SetFilled(x, y, z);
                    }
                }
            }
        }
    }
}

void Voxelizer::VoxelizeSolid(VoxelGrid& grid)
{
    // Use flood fill from exterior to mark all exterior voxels
    // Then invert: everything not exterior and not surface becomes solid

    std::vector<uint8_t> exterior(grid.totalVoxels, 0);

    // Flood fill from all boundary voxels that are not filled
    FloodFillExterior(grid, exterior);

    // Mark all non-exterior, non-surface voxels as filled (interior)
    for (uint64_t i = 0; i < grid.totalVoxels; ++i)
    {
        if (!exterior[i] && !grid.IsFilled(i))
        {
            grid.SetFilled(i);
        }
    }
}

void Voxelizer::FloodFillExterior(VoxelGrid& grid, std::vector<uint8_t>& exterior)
{
    // 6-connectivity neighbors
    const int dx[] = {-1, 1, 0, 0, 0, 0};
    const int dy[] = {0, 0, -1, 1, 0, 0};
    const int dz[] = {0, 0, 0, 0, -1, 1};

    std::queue<glm::ivec3> queue;

    // Start from all boundary voxels that are empty
    // X boundaries
    for (int y = 0; y < grid.size.y; ++y)
    {
        for (int z = 0; z < grid.size.z; ++z)
        {
            if (!grid.IsFilled(0, y, z))
            {
                uint64_t idx = grid.CoordToIndex(0, y, z);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(0, y, z));
                }
            }
            if (!grid.IsFilled(grid.size.x - 1, y, z))
            {
                uint64_t idx = grid.CoordToIndex(grid.size.x - 1, y, z);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(grid.size.x - 1, y, z));
                }
            }
        }
    }

    // Y boundaries
    for (int x = 0; x < grid.size.x; ++x)
    {
        for (int z = 0; z < grid.size.z; ++z)
        {
            if (!grid.IsFilled(x, 0, z))
            {
                uint64_t idx = grid.CoordToIndex(x, 0, z);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(x, 0, z));
                }
            }
            if (!grid.IsFilled(x, grid.size.y - 1, z))
            {
                uint64_t idx = grid.CoordToIndex(x, grid.size.y - 1, z);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(x, grid.size.y - 1, z));
                }
            }
        }
    }

    // Z boundaries
    for (int x = 0; x < grid.size.x; ++x)
    {
        for (int y = 0; y < grid.size.y; ++y)
        {
            if (!grid.IsFilled(x, y, 0))
            {
                uint64_t idx = grid.CoordToIndex(x, y, 0);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(x, y, 0));
                }
            }
            if (!grid.IsFilled(x, y, grid.size.z - 1))
            {
                uint64_t idx = grid.CoordToIndex(x, y, grid.size.z - 1);
                if (!exterior[idx])
                {
                    exterior[idx] = 1;
                    queue.push(glm::ivec3(x, y, grid.size.z - 1));
                }
            }
        }
    }

    // BFS flood fill
    while (!queue.empty())
    {
        glm::ivec3 current = queue.front();
        queue.pop();

        for (int i = 0; i < 6; ++i)
        {
            glm::ivec3 neighbor(current.x + dx[i], current.y + dy[i], current.z + dz[i]);

            if (!grid.IsValidCoord(neighbor)) continue;

            uint64_t idx = grid.CoordToIndex(neighbor);
            if (exterior[idx]) continue;  // Already visited
            if (grid.IsFilled(idx)) continue;  // Surface voxel, stop

            exterior[idx] = 1;
            queue.push(neighbor);
        }
    }
}

} // namespace voxelizer
