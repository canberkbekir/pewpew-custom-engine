#include "cbpch.h"
#include "VoxelizationTask.h"
#include "CBEngine/Core/Log.h"
#include "CBEngine/Debug/Instrumentor.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

namespace CB
{
    // Forward declaration - reuse the static functions from Mesh.cpp
    // We load mesh data via Assimp directly in the background thread
    static void LoadMeshData(const String& filePath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        Assimp::Importer importer;
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.01f);
        const aiScene* scene = importer.ReadFile(filePath,
                                                 aiProcess_Triangulate |
                                                 aiProcess_GenSmoothNormals |
                                                 aiProcess_CalcTangentSpace |
                                                 aiProcess_JoinIdenticalVertices |
                                                 aiProcess_PreTransformVertices |
                                                 aiProcess_GlobalScale
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            CB_CORE_ERROR("VoxelizationTask: Assimp Error: {0}", importer.GetErrorString());
            return;
        }

        std::function<void(aiNode*, const aiScene*)> processNode;
        processNode = [&](aiNode* node, const aiScene* scene)
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

                    if (mesh->HasTangentsAndBitangents())
                    {
                        vertex.Tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
                        vertex.Bitangent = {mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
                    }

                    vertices.push_back(vertex);
                }

                for (unsigned int j = 0; j < mesh->mNumFaces; j++)
                {
                    aiFace& face = mesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++)
                        indices.push_back(vertexOffset + face.mIndices[k]);
                }
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++)
                processNode(node->mChildren[i], scene);
        };

        processNode(scene->mRootNode, scene);
    }

    VoxelizationTask::~VoxelizationTask()
    {
        Cancel();
        WaitForCompletion();
    }

    void VoxelizationTask::WaitForCompletion()
    {
        if (m_Future.valid())
            m_Future.get();
    }

    void VoxelizationTask::Cancel()
    {
        m_CancelRequested = true;
        if (m_Future.valid())
        {
            m_Future.wait();
            m_Status = VoxelTaskStatus::Cancelled;
        }
        m_CancelRequested = false;
    }

    void VoxelizationTask::Start(const String& meshPath, const VoxelizeSettings& settings)
    {
        if (m_Status == VoxelTaskStatus::Running)
        {
            Cancel();
            WaitForCompletion();
        }

        m_Status = VoxelTaskStatus::Running;
        m_CancelRequested = false;

        m_Future = std::async(std::launch::async, [this, meshPath, settings]()
        {
            try
            {
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;
                LoadMeshData(meshPath, vertices, indices);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                if (vertices.empty())
                {
                    CB_CORE_ERROR("VoxelizationTask: No vertices loaded from {0}", meshPath);
                    m_Status = VoxelTaskStatus::Failed;
                    return;
                }

                // Skip GPU mesh creation - no OpenGL context on background thread
                auto taskSettings = settings;
                taskSettings.CreateGPUMesh = false;

                std::vector<Vector2> voxelUVs;
                auto data = VoxelizerAPI::VoxelizeWithUVs(vertices, indices, taskSettings, voxelUVs);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                m_Result.Grid = data.Grid;
                m_Result.VoxelCount = data.VoxelCount;
                m_Result.HasColors = false;
                m_Result.VoxelUVs = std::move(voxelUVs);
                m_Result.HasUVs = !m_Result.VoxelUVs.empty();
                m_Status = VoxelTaskStatus::Completed;
            }
            catch (const std::exception& e)
            {
                CB_CORE_ERROR("VoxelizationTask: Exception: {0}", e.what());
                m_Status = VoxelTaskStatus::Failed;
            }
        });
    }

    void VoxelizationTask::StartWithColors(const String& meshPath, const String& texturePath,
                                           const VoxelizeSettings& settings)
    {
        if (m_Status == VoxelTaskStatus::Running)
        {
            Cancel();
            WaitForCompletion();
        }

        m_Status = VoxelTaskStatus::Running;
        m_CancelRequested = false;

        m_Future = std::async(std::launch::async, [this, meshPath, texturePath, settings]()
        {
            try
            {
                // Load mesh data first
                std::vector<Vertex> vertices;
                std::vector<uint32_t> indices;
                LoadMeshData(meshPath, vertices, indices);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                if (vertices.empty())
                {
                    CB_CORE_ERROR("VoxelizationTask: No vertices loaded from {0}", meshPath);
                    m_Status = VoxelTaskStatus::Failed;
                    return;
                }

                // Skip GPU mesh creation - no OpenGL context on background thread
                auto taskSettings = settings;
                taskSettings.CreateGPUMesh = false;

                std::vector<Vector3> voxelColors;
                auto data = VoxelizerAPI::VoxelizeWithColorsAndOutput(
                    vertices, indices, texturePath, taskSettings, voxelColors);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                m_Result.Grid = data.Grid;
                m_Result.VoxelCount = data.VoxelCount;
                m_Result.VoxelColors = std::move(voxelColors);
                m_Result.HasColors = !m_Result.VoxelColors.empty();

                // Build palette from colors
                if (m_Result.HasColors)
                {
                    auto paletteData = VoxelizerAPI::BuildPaletteFromColors(m_Result.VoxelColors);
                    m_Result.Palette = paletteData.Palette;
                    m_Result.PaletteIndices = std::move(paletteData.PaletteIndices);
                    m_Result.HasPalette = true;
                }

                // Also compute UVs for later texture assignment
                std::unordered_map<uint64_t, Vector2> uvSumMap;
                std::unordered_map<uint64_t, int> uvCountMap;
                VoxelizerAPI::ComputeVoxelUVs(data.Grid, vertices, indices, uvSumMap, uvCountMap);

                auto filledCoords = data.Grid.GetFilledCoords();
                m_Result.VoxelUVs.clear();
                m_Result.VoxelUVs.reserve(filledCoords.size());
                for (const auto& coord : filledCoords)
                {
                    uint64_t idx = data.Grid.CoordToIndex(coord);
                    auto it = uvCountMap.find(idx);
                    if (it != uvCountMap.end() && it->second > 0)
                        m_Result.VoxelUVs.push_back(uvSumMap[idx] / static_cast<float>(it->second));
                    else
                        m_Result.VoxelUVs.push_back(Vector2(0.5f, 0.5f));
                }
                m_Result.HasUVs = !m_Result.VoxelUVs.empty();

                m_Status = VoxelTaskStatus::Completed;
            }
            catch (const std::exception& e)
            {
                CB_CORE_ERROR("VoxelizationTask: Exception: {0}", e.what());
                m_Status = VoxelTaskStatus::Failed;
            }
        });
    }

    void VoxelizationTask::StartFromData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                                         const VoxelizeSettings& settings)
    {
        if (m_Status == VoxelTaskStatus::Running)
        {
            Cancel();
            WaitForCompletion();
        }

        m_Status = VoxelTaskStatus::Running;
        m_CancelRequested = false;

        // Copy data for the background thread
        auto verticesCopy = std::make_shared<std::vector<Vertex>>(vertices);
        auto indicesCopy = std::make_shared<std::vector<uint32_t>>(indices);

        m_Future = std::async(std::launch::async, [this, verticesCopy, indicesCopy, settings]()
        {
            try
            {
                // Skip GPU mesh creation - no OpenGL context on background thread
                auto taskSettings = settings;
                taskSettings.CreateGPUMesh = false;

                std::vector<Vector2> voxelUVs;
                auto data = VoxelizerAPI::VoxelizeWithUVs(*verticesCopy, *indicesCopy, taskSettings, voxelUVs);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                m_Result.Grid = data.Grid;
                m_Result.VoxelCount = data.VoxelCount;
                m_Result.HasColors = false;
                m_Result.VoxelUVs = std::move(voxelUVs);
                m_Result.HasUVs = !m_Result.VoxelUVs.empty();
                m_Status = VoxelTaskStatus::Completed;
            }
            catch (const std::exception& e)
            {
                CB_CORE_ERROR("VoxelizationTask: Exception: {0}", e.what());
                m_Status = VoxelTaskStatus::Failed;
            }
        });
    }

    void VoxelizationTask::StartFromDataWithColors(const std::vector<Vertex>& vertices,
                                                   const std::vector<uint32_t>& indices,
                                                   const String& texturePath, const VoxelizeSettings& settings)
    {
        if (m_Status == VoxelTaskStatus::Running)
        {
            Cancel();
            WaitForCompletion();
        }

        m_Status = VoxelTaskStatus::Running;
        m_CancelRequested = false;

        auto verticesCopy = std::make_shared<std::vector<Vertex>>(vertices);
        auto indicesCopy = std::make_shared<std::vector<uint32_t>>(indices);

        m_Future = std::async(std::launch::async, [this, verticesCopy, indicesCopy, texturePath, settings]()
        {
            try
            {
                // Skip GPU mesh creation - no OpenGL context on background thread
                auto taskSettings = settings;
                taskSettings.CreateGPUMesh = false;

                std::vector<Vector3> voxelColors;
                auto data = VoxelizerAPI::VoxelizeWithColorsAndOutput(
                    *verticesCopy, *indicesCopy, texturePath, taskSettings, voxelColors);

                if (m_CancelRequested)
                {
                    m_Status = VoxelTaskStatus::Cancelled;
                    return;
                }

                m_Result.Grid = data.Grid;
                m_Result.VoxelCount = data.VoxelCount;
                m_Result.VoxelColors = std::move(voxelColors);
                m_Result.HasColors = !m_Result.VoxelColors.empty();

                // Build palette from colors
                if (m_Result.HasColors)
                {
                    auto paletteData = VoxelizerAPI::BuildPaletteFromColors(m_Result.VoxelColors);
                    m_Result.Palette = paletteData.Palette;
                    m_Result.PaletteIndices = std::move(paletteData.PaletteIndices);
                    m_Result.HasPalette = true;
                }

                // Also compute UVs for later texture assignment
                std::unordered_map<uint64_t, Vector2> uvSumMap;
                std::unordered_map<uint64_t, int> uvCountMap;
                VoxelizerAPI::ComputeVoxelUVs(data.Grid, *verticesCopy, *indicesCopy, uvSumMap, uvCountMap);

                auto filledCoords = data.Grid.GetFilledCoords();
                m_Result.VoxelUVs.clear();
                m_Result.VoxelUVs.reserve(filledCoords.size());
                for (const auto& coord : filledCoords)
                {
                    uint64_t idx = data.Grid.CoordToIndex(coord);
                    auto it = uvCountMap.find(idx);
                    if (it != uvCountMap.end() && it->second > 0)
                        m_Result.VoxelUVs.push_back(uvSumMap[idx] / static_cast<float>(it->second));
                    else
                        m_Result.VoxelUVs.push_back(Vector2(0.5f, 0.5f));
                }
                m_Result.HasUVs = !m_Result.VoxelUVs.empty();

                m_Status = VoxelTaskStatus::Completed;
            }
            catch (const std::exception& e)
            {
                CB_CORE_ERROR("VoxelizationTask: Exception: {0}", e.what());
                m_Status = VoxelTaskStatus::Failed;
            }
        });
    }

    Ref<Mesh> VoxelizationTask::CreateMeshFromResult()
    {
        if (m_Status != VoxelTaskStatus::Completed)
            return nullptr;

        return VoxelizerAPI::CreateMeshFromGrid(m_Result.Grid);
    }

    Ref<Mesh> VoxelizationTask::CreateColoredMeshFromResult()
    {
        if (m_Status != VoxelTaskStatus::Completed)
            return nullptr;

        if (m_Result.HasColors && !m_Result.VoxelColors.empty())
            return VoxelizerAPI::CreateColoredMeshFromGrid(m_Result.Grid, m_Result.VoxelColors);

        return VoxelizerAPI::CreateMeshFromGrid(m_Result.Grid);
    }

    Ref<Mesh> VoxelizationTask::CreatePaletteMeshFromResult()
    {
        if (m_Status != VoxelTaskStatus::Completed)
            return nullptr;

        if (m_Result.HasPalette && !m_Result.PaletteIndices.empty())
            return VoxelizerAPI::CreatePaletteMeshFromGrid(m_Result.Grid, m_Result.PaletteIndices);

        return CreateMeshFromResult();
    }
}
