#include "pewpch.h"
#include "VoxelizerAPI.h"
#include "PewPew/Core/Log.h"
#include "PewPew/Debug/Instrumentor.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>
#include <TriBox.h>

namespace PewPew
{
    bool VoxelizerAPI::s_Initialized = false;
    Ref<Shader> VoxelizerAPI::s_VoxelShader = nullptr;
    std::vector<Vertex> VoxelizerAPI::s_UnitCubeVertices;
    std::vector<uint32_t> VoxelizerAPI::s_UnitCubeIndices;

    // ============================================================================
    // TextureSampler Implementation
    // ============================================================================

    TextureSampler::~TextureSampler()
    {
        if (m_Data)
        {
            stbi_image_free(m_Data);
            m_Data = nullptr;
        }
    }

    bool TextureSampler::Load(const String& filePath)
    {
        if (m_Data)
        {
            stbi_image_free(m_Data);
            m_Data = nullptr;
        }

        stbi_set_flip_vertically_on_load(1);
        int width, height, channels;
        m_Data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

        if (!m_Data)
        {
            PEW_CORE_ERROR("TextureSampler: Failed to load texture: {0}", filePath);
            return false;
        }

        m_Width = static_cast<uint32_t>(width);
        m_Height = static_cast<uint32_t>(height);
        m_Channels = channels;

        PEW_CORE_INFO("TextureSampler: Loaded {0} ({1}x{2}, {3} channels)",
                      filePath, m_Width, m_Height, m_Channels);
        return true;
    }

    Vector3 TextureSampler::Sample(const Vector2& uv) const
    {
        if (!m_Data)
            return Vector3(1.0f);

        // Wrap UV coordinates
        float u = uv.x - std::floor(uv.x);
        float v = uv.y - std::floor(uv.y);

        uint32_t x = static_cast<uint32_t>(u * (m_Width - 1));
        uint32_t y = static_cast<uint32_t>(v * (m_Height - 1));

        x = std::min(x, m_Width - 1);
        y = std::min(y, m_Height - 1);

        size_t idx = (y * m_Width + x) * m_Channels;

        float r = m_Data[idx] / 255.0f;
        float g = (m_Channels > 1) ? m_Data[idx + 1] / 255.0f : r;
        float b = (m_Channels > 2) ? m_Data[idx + 2] / 255.0f : r;

        return Vector3(r, g, b);
    }

    Vector3 TextureSampler::SampleBilinear(const Vector2& uv) const
    {
        if (!m_Data)
            return Vector3(1.0f);

        // Wrap UV coordinates
        float u = uv.x - std::floor(uv.x);
        float v = uv.y - std::floor(uv.y);

        float fx = u * (m_Width - 1);
        float fy = v * (m_Height - 1);

        uint32_t x0 = static_cast<uint32_t>(fx);
        uint32_t y0 = static_cast<uint32_t>(fy);
        uint32_t x1 = std::min(x0 + 1, m_Width - 1);
        uint32_t y1 = std::min(y0 + 1, m_Height - 1);

        float sx = fx - x0;
        float sy = fy - y0;

        auto getPixel = [this](uint32_t x, uint32_t y) -> Vector3 {
            size_t idx = (y * m_Width + x) * m_Channels;
            float r = m_Data[idx] / 255.0f;
            float g = (m_Channels > 1) ? m_Data[idx + 1] / 255.0f : r;
            float b = (m_Channels > 2) ? m_Data[idx + 2] / 255.0f : r;
            return Vector3(r, g, b);
        };

        Vector3 c00 = getPixel(x0, y0);
        Vector3 c10 = getPixel(x1, y0);
        Vector3 c01 = getPixel(x0, y1);
        Vector3 c11 = getPixel(x1, y1);

        Vector3 c0 = glm::mix(c00, c10, sx);
        Vector3 c1 = glm::mix(c01, c11, sx);

        return glm::mix(c0, c1, sy);
    }

    // ============================================================================
    // VoxelizerAPI Implementation
    // ============================================================================

    void VoxelizerAPI::Init()
    {
        PEW_PROFILE_FUNCTION();

        if (s_Initialized)
            return;

        // Create unit cube geometry (centered at origin, size 1x1x1)
        CreateUnitCube(s_UnitCubeVertices, s_UnitCubeIndices);

        s_Initialized = true;
        PEW_CORE_INFO("VoxelizerAPI initialized");
    }

    void VoxelizerAPI::Shutdown()
    {
        s_VoxelShader = nullptr;
        s_UnitCubeVertices.clear();
        s_UnitCubeIndices.clear();
        s_Initialized = false;
    }

    void VoxelizerAPI::CreateUnitCube(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        vertices.clear();
        indices.clear();

        // Unit cube centered at origin (-0.5 to 0.5)
        // Each face has its own vertices for proper normals

        // Front face (Z+)
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});

        // Back face (Z-)
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});

        // Top face (Y+)
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}});

        // Bottom face (Y-)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}});

        // Right face (X+)
        vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});

        // Left face (X-)
        vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
        vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});

        // Indices for 6 faces (2 triangles per face)
        for (uint32_t face = 0; face < 6; ++face)
        {
            uint32_t base = face * 4;
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
            indices.push_back(base + 0);
        }
    }

    VoxelMeshData VoxelizerAPI::Voxelize(const Ref<Mesh>& mesh,
                                          const std::vector<Vertex>& vertices,
                                          const std::vector<uint32_t>& indices,
                                          const VoxelizeSettings& settings)
    {
        return Voxelize(vertices, indices, settings);
    }

    VoxelMeshData VoxelizerAPI::Voxelize(const std::vector<Vertex>& vertices,
                                          const std::vector<uint32_t>& indices,
                                          const VoxelizeSettings& settings)
    {
        PEW_PROFILE_FUNCTION();

        if (!s_Initialized)
        {
            PEW_CORE_WARN("VoxelizerAPI::Voxelize called before Init()! Initializing now...");
            Init();
        }

        VoxelMeshData result;

        // Convert to voxelizer format
        std::vector<glm::vec3> positions;
        positions.reserve(vertices.size());
        for (const auto& v : vertices)
        {
            positions.push_back(v.Position);
        }

        // Create voxelizer
        voxelizer::Voxelizer vox(settings.GridSize);
        if (settings.VoxelSize > 0.0f)
        {
            vox.SetVoxelSize(settings.VoxelSize);
        }
        vox.SetPadding(settings.Padding);

        // Perform voxelization
        {
            PEW_PROFILE_SCOPE("Voxelizer::Voxelize");
            result.Grid = vox.Voxelize(positions, indices, settings.Solid);
        }

        result.VoxelCount = result.Grid.CountFilled();

        PEW_CORE_INFO("Voxelized mesh: {0} voxels, grid size: ({1}, {2}, {3})",
                      result.VoxelCount, result.Grid.size.x, result.Grid.size.y, result.Grid.size.z);

        // Create renderable mesh
        {
            PEW_PROFILE_SCOPE("CreateMeshFromGrid");
            result.Mesh = CreateMeshFromGrid(result.Grid);
        }

        return result;
    }

    // Helper to load mesh from file
    static void LoadMeshFromFile(const String& filePath,
                                  std::vector<Vertex>& vertices,
                                  std::vector<uint32_t>& indices)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_PreTransformVertices
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            PEW_CORE_ERROR("VoxelizerAPI: Failed to load mesh: {0}", importer.GetErrorString());
            return;
        }

        std::function<void(aiNode*)> processNode = [&](aiNode* node)
        {
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

                for (unsigned int j = 0; j < mesh->mNumVertices; j++)
                {
                    Vertex vertex{};
                    vertex.Position = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};

                    if (mesh->HasNormals())
                        vertex.Normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
                    else
                        vertex.Normal = {0.0f, 1.0f, 0.0f};

                    if (mesh->mTextureCoords[0])
                        vertex.TexCoords = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
                    else
                        vertex.TexCoords = {0.0f, 0.0f};

                    if (mesh->HasTangentsAndBitangents())
                    {
                        vertex.Tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
                        vertex.Bitangent = {mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
                    }
                    else
                    {
                        vertex.Tangent = {1.0f, 0.0f, 0.0f};
                        vertex.Bitangent = {0.0f, 1.0f, 0.0f};
                    }

                    vertex.Color = {1.0f, 1.0f, 1.0f};
                    vertices.push_back(vertex);
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; j++)
                {
                    aiFace& face = mesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++)
                    {
                        indices.push_back(vertexOffset + face.mIndices[k]);
                    }
                }
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                processNode(node->mChildren[i]);
            }
        };

        processNode(scene->mRootNode);
    }

    VoxelMeshData VoxelizerAPI::VoxelizeFromFile(const String& filePath, const VoxelizeSettings& settings)
    {
        PEW_PROFILE_FUNCTION();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        LoadMeshFromFile(filePath, vertices, indices);

        if (vertices.empty())
            return {};

        return Voxelize(vertices, indices, settings);
    }

    // ============================================================================
    // Texture-Colored Voxelization (Optimized - processes triangles, not voxels)
    // ============================================================================

    void VoxelizerAPI::ComputeVoxelColors(const voxelizer::VoxelGrid& grid,
                                           const std::vector<Vertex>& vertices,
                                           const std::vector<uint32_t>& indices,
                                           const TextureSampler& texture,
                                           std::unordered_map<uint64_t, Vector3>& colorSumMap,
                                           std::unordered_map<uint64_t, int>& colorCountMap)
    {
        glm::vec3 halfSize = grid.voxelSize * 0.5f;
        float boxhalfsize[3] = {halfSize.x, halfSize.y, halfSize.z};

        // Process each triangle - find which voxels it touches
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const Vertex& v0 = vertices[indices[i]];
            const Vertex& v1 = vertices[indices[i + 1]];
            const Vertex& v2 = vertices[indices[i + 2]];

            // Sample color once per triangle (average of vertex UVs)
            Vector2 centerUV = (v0.TexCoords + v1.TexCoords + v2.TexCoords) / 3.0f;
            Vector3 triColor = texture.SampleBilinear(centerUV);

            // Compute triangle AABB
            glm::vec3 triMin = glm::min(glm::min(v0.Position, v1.Position), v2.Position);
            glm::vec3 triMax = glm::max(glm::max(v0.Position, v1.Position), v2.Position);

            // Convert to voxel coordinates (with bounds clamping)
            glm::ivec3 voxMin = grid.WorldToVoxel(triMin);
            glm::ivec3 voxMax = grid.WorldToVoxel(triMax);

            voxMin = glm::max(voxMin, glm::ivec3(0));
            voxMax = glm::min(voxMax, grid.size - 1);

            // Prepare triangle vertices for collision test
            float triverts[3][3] = {
                {v0.Position.x, v0.Position.y, v0.Position.z},
                {v1.Position.x, v1.Position.y, v1.Position.z},
                {v2.Position.x, v2.Position.y, v2.Position.z}
            };

            // Only check voxels within triangle's AABB (huge optimization!)
            for (int x = voxMin.x; x <= voxMax.x; ++x)
            {
                for (int y = voxMin.y; y <= voxMax.y; ++y)
                {
                    for (int z = voxMin.z; z <= voxMax.z; ++z)
                    {
                        uint64_t voxelIdx = grid.CoordToIndex(x, y, z);

                        // Only process filled voxels
                        if (!grid.IsFilled(voxelIdx))
                            continue;

                        glm::vec3 center = grid.VoxelCenterToWorld(glm::ivec3(x, y, z));
                        float boxcenter[3] = {center.x, center.y, center.z};

                        if (voxelizer::TriBoxOverlap(boxcenter, boxhalfsize, triverts))
                        {
                            colorSumMap[voxelIdx] += triColor;
                            colorCountMap[voxelIdx]++;
                        }
                    }
                }
            }
        }
    }

    VoxelMeshData VoxelizerAPI::VoxelizeWithColors(const std::vector<Vertex>& vertices,
                                                    const std::vector<uint32_t>& indices,
                                                    const String& texturePath,
                                                    const VoxelizeSettings& settings)
    {
        PEW_PROFILE_FUNCTION();

        if (!s_Initialized)
        {
            Init();
        }

        VoxelMeshData result;

        // Load texture for color sampling
        TextureSampler texture;
        if (!texture.Load(texturePath))
        {
            PEW_CORE_WARN("VoxelizerAPI: Failed to load texture, using default colors");
            return Voxelize(vertices, indices, settings);
        }

        // Convert to voxelizer format
        std::vector<glm::vec3> positions;
        positions.reserve(vertices.size());
        for (const auto& v : vertices)
        {
            positions.push_back(v.Position);
        }

        // Create voxelizer and perform voxelization
        voxelizer::Voxelizer vox(settings.GridSize);
        if (settings.VoxelSize > 0.0f)
        {
            vox.SetVoxelSize(settings.VoxelSize);
        }
        vox.SetPadding(settings.Padding);

        {
            PEW_PROFILE_SCOPE("Voxelizer::Voxelize");
            result.Grid = vox.Voxelize(positions, indices, settings.Solid);
        }

        result.VoxelCount = result.Grid.CountFilled();

        // Compute colors efficiently - process triangles, not voxels
        std::unordered_map<uint64_t, Vector3> colorSumMap;
        std::unordered_map<uint64_t, int> colorCountMap;

        {
            PEW_PROFILE_SCOPE("ComputeVoxelColors");
            ComputeVoxelColors(result.Grid, vertices, indices, texture, colorSumMap, colorCountMap);
        }

        // Build color array in same order as filled coords
        auto filledCoords = result.Grid.GetFilledCoords();
        std::vector<Vector3> voxelColors;
        voxelColors.reserve(filledCoords.size());

        for (const auto& coord : filledCoords)
        {
            uint64_t idx = result.Grid.CoordToIndex(coord);
            auto it = colorCountMap.find(idx);
            if (it != colorCountMap.end() && it->second > 0)
            {
                voxelColors.push_back(colorSumMap[idx] / static_cast<float>(it->second));
            }
            else
            {
                // Interior voxel with no triangle intersection - use average nearby color or default
                voxelColors.push_back(Vector3(0.5f, 0.5f, 0.5f));
            }
        }

        PEW_CORE_INFO("Voxelized mesh with colors: {0} voxels, grid size: ({1}, {2}, {3})",
                      result.VoxelCount, result.Grid.size.x, result.Grid.size.y, result.Grid.size.z);

        // Create colored mesh
        {
            PEW_PROFILE_SCOPE("CreateColoredMeshFromGrid");
            result.Mesh = CreateColoredMeshFromGrid(result.Grid, voxelColors);
        }

        return result;
    }

    VoxelMeshData VoxelizerAPI::VoxelizeFromFileWithColors(const String& meshPath,
                                                           const String& texturePath,
                                                           const VoxelizeSettings& settings)
    {
        PEW_PROFILE_FUNCTION();

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        LoadMeshFromFile(meshPath, vertices, indices);

        if (vertices.empty())
            return {};

        return VoxelizeWithColors(vertices, indices, texturePath, settings);
    }

    Ref<Mesh> VoxelizerAPI::CreateColoredMeshFromGrid(const voxelizer::VoxelGrid& grid,
                                                       const std::vector<Vector3>& voxelColors)
    {
        PEW_PROFILE_FUNCTION();

        if (!s_Initialized)
        {
            Init();
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        auto filledCoords = grid.GetFilledCoords();

        if (filledCoords.size() != voxelColors.size())
        {
            PEW_CORE_ERROR("VoxelizerAPI: Color count mismatch! {0} voxels, {1} colors",
                          filledCoords.size(), voxelColors.size());
            return nullptr;
        }

        vertices.reserve(filledCoords.size() * 24);
        indices.reserve(filledCoords.size() * 36);

        glm::vec3 voxelSize = grid.voxelSize;

        for (size_t i = 0; i < filledCoords.size(); ++i)
        {
            const auto& coord = filledCoords[i];
            const Vector3& color = voxelColors[i];

            glm::vec3 center = grid.VoxelCenterToWorld(coord);
            uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            // Add transformed unit cube vertices with color
            for (const auto& cubeVertex : s_UnitCubeVertices)
            {
                Vertex v = cubeVertex;
                v.Position = center + Vector3(cubeVertex.Position) * voxelSize;
                v.Color = color;
                vertices.push_back(v);
            }

            // Add indices with offset
            for (uint32_t idx : s_UnitCubeIndices)
            {
                indices.push_back(baseVertex + idx);
            }
        }

        if (vertices.empty())
        {
            PEW_CORE_WARN("VoxelizerAPI::CreateColoredMeshFromGrid: No filled voxels");
            return nullptr;
        }

        return CreateRef<Mesh>(vertices, indices);
    }

    // ============================================================================
    // Standard Methods
    // ============================================================================

    Ref<Mesh> VoxelizerAPI::CreateMeshFromGrid(const voxelizer::VoxelGrid& grid)
    {
        return CreateMeshFromGrid(grid, Vector3(1.0f, 1.0f, 1.0f));
    }

    Ref<Mesh> VoxelizerAPI::CreateMeshFromGrid(const voxelizer::VoxelGrid& grid, const Vector3& color)
    {
        PEW_PROFILE_FUNCTION();

        if (!s_Initialized)
            Init();

        auto filledCoords = grid.GetFilledCoords();
        if (filledCoords.empty())
        {
            PEW_CORE_WARN("VoxelizerAPI::CreateMeshFromGrid: No filled voxels");
            return nullptr;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vertices.reserve(filledCoords.size() * 24);
        indices.reserve(filledCoords.size() * 36);

        glm::vec3 voxelSize = grid.voxelSize;

        for (const auto& coord : filledCoords)
        {
            glm::vec3 center = grid.VoxelCenterToWorld(coord);
            uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            for (const auto& cubeVertex : s_UnitCubeVertices)
            {
                Vertex v = cubeVertex;
                v.Position = center + Vector3(cubeVertex.Position) * voxelSize;
                v.Color = color;
                vertices.push_back(v);
            }

            for (uint32_t idx : s_UnitCubeIndices)
                indices.push_back(baseVertex + idx);
        }

        return CreateRef<Mesh>(vertices, indices);
    }

    std::vector<Vector3> VoxelizerAPI::GetVoxelPositions(const voxelizer::VoxelGrid& grid)
    {
        auto glmPositions = grid.GetFilledPositions();
        std::vector<Vector3> positions;
        positions.reserve(glmPositions.size());
        for (const auto& p : glmPositions)
        {
            positions.push_back(p);
        }
        return positions;
    }

    Vector3 VoxelizerAPI::GetVoxelSize(const voxelizer::VoxelGrid& grid)
    {
        return grid.voxelSize;
    }

    bool VoxelizerAPI::IsPointInside(const voxelizer::VoxelGrid& grid, const Vector3& worldPos)
    {
        glm::ivec3 voxelCoord = grid.WorldToVoxel(worldPos);
        return grid.IsFilled(voxelCoord);
    }

    Ref<Shader> VoxelizerAPI::GetVoxelShader()
    {
        if (!s_VoxelShader)
        {
            s_VoxelShader = Shader::Create("assets/shaders/PBR.glsl");
        }
        return s_VoxelShader;
    }

    Ref<Material> VoxelizerAPI::CreateVoxelMaterial(const Vector3& color)
    {
        auto material = CreateRef<Material>();
        material->SetAlbedo(color);
        material->SetRoughness(0.7f);
        material->SetMetallic(0.0f);
        return material;
    }
}
