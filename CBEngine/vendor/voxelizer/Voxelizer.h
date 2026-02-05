/*
 * Voxelizer.h
 * Simplified mesh voxelization library
 *
 * Original: https://github.com/topskychen/voxelizer
 * Simplified for CBEngine - uses GLM and standard C++
 * No Boost, FCL, or Abseil dependencies
 */

#ifndef VOXELIZER_VOXELIZER_H
#define VOXELIZER_VOXELIZER_H

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <memory>

namespace voxelizer {

// Voxel data stored as bit-packed uint64_t for memory efficiency
// Each uint64_t stores 64 voxels
using VoxelData = std::vector<uint64_t>;

struct Triangle
{
    glm::vec3 v0, v1, v2;

    Triangle() = default;
    Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        : v0(a), v1(b), v2(c) {}
};

struct VoxelGrid
{
    VoxelData data;
    glm::ivec3 size;         // Grid dimensions (x, y, z)
    glm::vec3 origin;        // World-space origin (lower bound)
    glm::vec3 voxelSize;     // Size of each voxel in world units
    uint64_t totalVoxels;    // size.x * size.y * size.z

    // Check if a voxel is filled
    bool IsFilled(int x, int y, int z) const;
    bool IsFilled(const glm::ivec3& coord) const;
    bool IsFilled(uint64_t index) const;

    // Set a voxel as filled
    void SetFilled(int x, int y, int z);
    void SetFilled(const glm::ivec3& coord);
    void SetFilled(uint64_t index);

    // Clear a voxel
    void Clear(int x, int y, int z);
    void Clear(uint64_t index);

    // Convert between index and coordinates
    uint64_t CoordToIndex(int x, int y, int z) const;
    uint64_t CoordToIndex(const glm::ivec3& coord) const;
    glm::ivec3 IndexToCoord(uint64_t index) const;

    // Convert between world position and voxel coordinates
    glm::ivec3 WorldToVoxel(const glm::vec3& worldPos) const;
    glm::vec3 VoxelToWorld(const glm::ivec3& voxelCoord) const;
    glm::vec3 VoxelCenterToWorld(const glm::ivec3& voxelCoord) const;

    // Check if coordinates are valid
    bool IsValidCoord(int x, int y, int z) const;
    bool IsValidCoord(const glm::ivec3& coord) const;

    // Get count of filled voxels
    uint64_t CountFilled() const;

    // Get all filled voxel coordinates
    std::vector<glm::ivec3> GetFilledCoords() const;

    // Get all filled voxel world positions (centers)
    std::vector<glm::vec3> GetFilledPositions() const;
};

class Voxelizer
{
public:
    // Create voxelizer with specified grid resolution
    // gridSize: number of voxels along the longest axis of the mesh AABB
    explicit Voxelizer(int gridSize = 64);

    // Create voxelizer with specified voxel size in world units
    explicit Voxelizer(float voxelSize);

    // Voxelize a mesh from triangles
    // Returns a VoxelGrid containing the voxelized result
    VoxelGrid Voxelize(const std::vector<Triangle>& triangles, bool solid = false);

    // Voxelize from vertex and index buffers (common mesh format)
    // vertices: array of vertex positions
    // indices: triangle indices (every 3 indices = 1 triangle)
    VoxelGrid Voxelize(const std::vector<glm::vec3>& vertices,
                       const std::vector<uint32_t>& indices,
                       bool solid = false);

    // Settings
    void SetGridSize(int size) { m_GridSize = size; m_UseVoxelSize = false; }
    void SetVoxelSize(float size) { m_VoxelSize = size; m_UseVoxelSize = true; }
    void SetPadding(float padding) { m_Padding = padding; }

private:
    void ComputeBounds(const std::vector<Triangle>& triangles,
                       glm::vec3& outMin, glm::vec3& outMax);

    void VoxelizeSurface(const std::vector<Triangle>& triangles, VoxelGrid& grid);
    void VoxelizeSolid(VoxelGrid& grid);
    void FloodFillExterior(VoxelGrid& grid, std::vector<uint8_t>& visited);

    int m_GridSize = 64;
    float m_VoxelSize = 0.1f;
    float m_Padding = 0.01f;  // Small padding around mesh bounds
    bool m_UseVoxelSize = false;
};

} // namespace voxelizer

#endif // VOXELIZER_VOXELIZER_H
