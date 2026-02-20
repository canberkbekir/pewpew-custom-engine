#pragma once
#include "CBEngine/Core/Core.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Renderer/Resources/Mesh.h"
#include "CBEngine/Renderer/Resources/Shader.h"
#include "CBEngine/Renderer/Resources/Material.h"
#include "CBEngine/Renderer/Resources/Texture.h"
#include "CBEngine/Renderer/Camera/Camera.h"

#include "VoxelPalette.h"

#include <Voxelizer.h>
#include <unordered_map>

namespace CB
{
    /// @brief Result of voxelization operations
    struct VoxelMeshData
    {
        /// Renderable mesh containing one cube per filled voxel
        Ref<Mesh> Mesh;
        /// Raw voxel grid data for spatial queries
        voxelizer::VoxelGrid Grid;
        /// Total number of filled voxels
        uint64_t VoxelCount = 0;
    };

    /// @brief Configuration settings for voxelization
    struct VoxelizeSettings
    {
        /// Number of voxels along longest mesh axis (8-128 recommended)
        int GridSize = 32;
        /// If > 0, use fixed voxel size in world units instead of GridSize
        float VoxelSize = 0.0f;
        /// true = fill interior (solid), false = surface only (hollow)
        bool Solid = true;
        /// Padding around mesh bounds in world units
        float Padding = 0.01f;
        /// If false, skip GPU mesh creation (for background thread usage)
        bool CreateGPUMesh = true;
    };

    /// @brief Helper class for sampling colors from texture files
    /// @details Loads image data and provides UV-based color sampling
    class TextureSampler
    {
    public:
        TextureSampler() = default;
        ~TextureSampler();

        /// @brief Load a texture from file
        /// @param filePath Path to image file (PNG, JPG, TGA supported)
        /// @return true if loaded successfully
        bool Load(const String& filePath);

        /// @brief Sample color at UV coordinates using nearest-neighbor
        /// @param uv Texture coordinates (0-1 range, wraps automatically)
        /// @return RGB color (0-1 range)
        Vector3 Sample(const Vector2& uv) const;

        /// @brief Sample color at UV coordinates using bilinear interpolation
        /// @param uv Texture coordinates (0-1 range, wraps automatically)
        /// @return RGB color (0-1 range)
        Vector3 SampleBilinear(const Vector2& uv) const;

        /// @brief Get texture width in pixels
        uint32_t GetWidth() const { return m_Width; }

        /// @brief Get texture height in pixels
        uint32_t GetHeight() const { return m_Height; }

        /// @brief Check if texture data is loaded
        bool IsLoaded() const { return m_Data != nullptr; }

    private:
        unsigned char* m_Data = nullptr;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        int m_Channels = 0;
    };

    struct VoxelPaletteData
    {
        VoxelPalette Palette;
        std::vector<uint8_t> PaletteIndices;
    };

    /// @brief Static utility API for converting 3D meshes into voxel representations
    /// @details Supports solid/surface voxelization with optional texture color sampling.
    /// Call Init() once at startup before using any voxelization methods.
    ///
    /// @example Basic usage:
    /// @code
    /// CB::VoxelizerAPI::Init();
    ///
    /// CB::VoxelizeSettings settings;
    /// settings.GridSize = 32;
    /// settings.Solid = true;
    ///
    /// auto data = CB::VoxelizerAPI::VoxelizeFromFileWithColors(
    ///     "assets/models/Model.fbx",
    ///     "assets/textures/Texture.png",
    ///     settings
    /// );
    ///
    /// // Render with Renderer3D
    /// Renderer3D::Submit(shader, material, data.Mesh, transform);
    /// @endcode
    class VoxelizerAPI
    {
    public:
        // ============ INITIALIZATION ============

        /// @brief Initialize the VoxelizerAPI (call once at startup)
        /// @details Creates cached unit cube geometry. Safe to call multiple times.
        static void Init();

        /// @brief Shutdown and cleanup cached resources
        /// @details Releases cached shaders and geometry data
        static void Shutdown();

        // ============ BASIC VOXELIZATION ============

        /// @brief Voxelize a mesh from vertex/index arrays
        /// @param mesh Original mesh reference (unused, for API consistency)
        /// @param vertices Vertex data array
        /// @param indices Triangle index array
        /// @param settings Voxelization configuration
        /// @return VoxelMeshData containing renderable mesh and grid data
        static VoxelMeshData Voxelize(const Ref<Mesh>& mesh,
                                      const std::vector<Vertex>& vertices,
                                      const std::vector<uint32_t>& indices,
                                      const VoxelizeSettings& settings = {});

        /// @brief Voxelize directly from vertex/index arrays
        /// @param vertices Vertex data array
        /// @param indices Triangle index array (every 3 indices = 1 triangle)
        /// @param settings Voxelization configuration
        /// @return VoxelMeshData containing renderable mesh and grid data
        static VoxelMeshData Voxelize(const std::vector<Vertex>& vertices,
                                      const std::vector<uint32_t>& indices,
                                      const VoxelizeSettings& settings = {});

        /// @brief Load and voxelize a mesh from file
        /// @param filePath Path to mesh file (FBX, OBJ, GLTF supported via Assimp)
        /// @param settings Voxelization configuration
        /// @return VoxelMeshData with white vertex colors
        static VoxelMeshData VoxelizeFromFile(const String& filePath,
                                              const VoxelizeSettings& settings = {});

        // ============ TEXTURE-COLORED VOXELIZATION ============

        /// @brief Voxelize with colors sampled from a texture
        /// @details For each voxel, samples the texture at UV coordinates of
        /// intersecting triangles and averages the colors.
        /// @param vertices Vertex data (must include valid TexCoords)
        /// @param indices Triangle index array
        /// @param texturePath Path to texture file (PNG, JPG, TGA)
        /// @param settings Voxelization configuration
        /// @return VoxelMeshData with per-voxel colors from texture
        static VoxelMeshData VoxelizeWithColors(const std::vector<Vertex>& vertices,
                                                const std::vector<uint32_t>& indices,
                                                const String& texturePath,
                                                const VoxelizeSettings& settings = {});

        /// @brief Load mesh and voxelize with texture colors
        /// @param meshPath Path to mesh file (FBX, OBJ, GLTF)
        /// @param texturePath Path to texture file (PNG, JPG, TGA)
        /// @param settings Voxelization configuration
        /// @return VoxelMeshData with per-voxel colors sampled from texture
        static VoxelMeshData VoxelizeFromFileWithColors(const String& meshPath,
                                                        const String& texturePath,
                                                        const VoxelizeSettings& settings = {});

        /// @brief Voxelize with colors and return per-voxel color array
        /// @param vertices Vertex data (must include valid TexCoords)
        /// @param indices Triangle index array
        /// @param texturePath Path to texture file
        /// @param settings Voxelization configuration
        /// @param outVoxelColors Receives per-filled-voxel RGB colors
        /// @return VoxelMeshData (Mesh may be null if CreateGPUMesh is false)
        static VoxelMeshData VoxelizeWithColorsAndOutput(const std::vector<Vertex>& vertices,
                                                         const std::vector<uint32_t>& indices,
                                                         const String& texturePath,
                                                         const VoxelizeSettings& settings,
                                                         std::vector<Vector3>& outVoxelColors);

        // ============ MESH CREATION ============

        /// @brief Create renderable mesh from voxel grid with per-voxel colors
        /// @param grid Voxel grid data
        /// @param voxelColors One RGB color per filled voxel (must match filled count)
        /// @return Renderable Mesh with vertex colors, or nullptr if empty
        static Ref<Mesh> CreateColoredMeshFromGrid(const voxelizer::VoxelGrid& grid,
                                                   const std::vector<Vector3>& voxelColors);

        /// @brief Create renderable mesh from voxel grid with default white color
        /// @param grid Voxel grid data
        /// @return Renderable Mesh, or nullptr if no filled voxels
        static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid);

        /// @brief Create renderable mesh from voxel grid with uniform color
        /// @param grid Voxel grid data
        /// @param color RGB color for all voxels (0-1 range)
        /// @return Renderable Mesh, or nullptr if no filled voxels
        static Ref<Mesh> CreateMeshFromGrid(const voxelizer::VoxelGrid& grid, const Vector3& color);

        // ============ UV COMPUTATION ============

        /// @brief Voxelize and compute per-voxel average UVs
        /// @param vertices Vertex data (must include valid TexCoords)
        /// @param indices Triangle index array
        /// @param settings Voxelization configuration
        /// @param outVoxelUVs Receives per-filled-voxel average UV coordinates
        /// @return VoxelMeshData (Mesh may be null if CreateGPUMesh is false)
        static VoxelMeshData VoxelizeWithUVs(const std::vector<Vertex>& vertices,
                                             const std::vector<uint32_t>& indices,
                                             const VoxelizeSettings& settings,
                                             std::vector<Vector2>& outVoxelUVs);

        /// @brief Build palette by sampling a texture at per-voxel UVs
        /// @param voxelUVs One UV per filled voxel
        /// @param texturePath Path to texture file (PNG, JPG, TGA)
        /// @return Palette and per-voxel palette indices (empty if texture fails to load)
        static VoxelPaletteData BuildPaletteFromUVs(const std::vector<Vector2>& voxelUVs,
                                                    const String& texturePath);

        // ============ PALETTE METHODS ============

        /// @brief Quantize per-voxel colors into a 256-entry palette using median-cut
        /// @param voxelColors One RGB color per filled voxel
        /// @return Palette and per-voxel palette indices
        static VoxelPaletteData BuildPaletteFromColors(const std::vector<Vector3>& voxelColors);

        /// @brief Median-cut quantization into up to maxColors palette entries
        /// @param colors Input colors to quantize
        /// @param maxColors Maximum palette size (default 256)
        /// @return Palette entries (average color per bucket)
        static std::vector<Vector3> BuildPaletteMedianCut(const std::vector<Vector3>& colors, int maxColors = 256);

        /// @brief Create renderable mesh from voxel grid with palette indices
        /// @param grid Voxel grid data
        /// @param paletteIndices One palette index per filled voxel
        /// @return Renderable Mesh with PaletteIndex set per vertex, or nullptr if empty
        static Ref<Mesh> CreatePaletteMeshFromGrid(const voxelizer::VoxelGrid& grid,
                                                   const std::vector<uint8_t>& paletteIndices);

        // ============ QUERY METHODS ============

        /// @brief Get world-space positions of all filled voxels
        /// @param grid Voxel grid data
        /// @return Vector of world-space center positions
        static std::vector<Vector3> GetVoxelPositions(const voxelizer::VoxelGrid& grid);

        /// @brief Get the size of each voxel in world units
        /// @param grid Voxel grid data
        /// @return Voxel dimensions (x, y, z)
        static Vector3 GetVoxelSize(const voxelizer::VoxelGrid& grid);

        /// @brief Check if a world position is inside a filled voxel
        /// @param grid Voxel grid data
        /// @param worldPos World-space position to test
        /// @return true if position is inside a filled voxel
        static bool IsPointInside(const voxelizer::VoxelGrid& grid, const Vector3& worldPos);

        // ============ HELPER METHODS ============

        /// @brief Get the default PBR shader for voxel rendering
        /// @return Cached shader instance (loads on first call)
        static Ref<Shader> GetVoxelShader();

        /// @brief Create a simple PBR material for voxel rendering
        /// @param color Albedo RGB color (default: light gray)
        /// @return Material with roughness=0.7, metallic=0.0
        static Ref<Material> CreateVoxelMaterial(const Vector3& color = Vector3(0.8f, 0.8f, 0.8f));

        /// @brief Compute per-voxel average UVs from triangle-voxel intersections
        static void ComputeVoxelUVs(const voxelizer::VoxelGrid& grid,
                                    const std::vector<Vertex>& vertices,
                                    const std::vector<uint32_t>& indices,
                                    std::unordered_map<uint64_t, Vector2>& uvSumMap,
                                    std::unordered_map<uint64_t, int>& uvCountMap);

    private:
        /// @brief Generate unit cube geometry (called by Init)
        static void CreateUnitCube(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

        /// @brief Efficiently compute voxel colors by processing triangles
        /// @details O(triangles * voxels_in_AABB) instead of O(voxels * triangles)
        static void ComputeVoxelColors(const voxelizer::VoxelGrid& grid,
                                       const std::vector<Vertex>& vertices,
                                       const std::vector<uint32_t>& indices,
                                       const TextureSampler& texture,
                                       std::unordered_map<uint64_t, Vector3>& colorSumMap,
                                       std::unordered_map<uint64_t, int>& colorCountMap);

        static bool s_Initialized;
        static Ref<Shader> s_VoxelShader;

        // Cached unit cube geometry
        static std::vector<Vertex> s_UnitCubeVertices;
        static std::vector<uint32_t> s_UnitCubeIndices;
    };
}
