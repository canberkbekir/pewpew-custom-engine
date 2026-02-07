#pragma once
#include "VertexArray.h"
#include "CBEngine/Core/Core.h"
#include "CBEngine/Core/String.h"
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Asset/Asset.h"

#include <vector>

namespace CB
{
    struct Vertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector2 TexCoords;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector3 Color = Vector3(1.0f);  // Vertex color (default white)
        float PaletteIndex = 0.0f;     // Palette index for voxel coloring (0-255)
    };

    class Mesh : public Asset
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, bool retainCPUData = false);
        ~Mesh() = default;

        static Ref<Mesh> Load(const String& filePath);
        static Ref<Mesh> Load(const String& filePath, bool retainCPUData);

        void Bind() const;
        uint32_t GetIndexCount() const { return m_IndexCount; }
        uint32_t GetVertexCount() const { return m_VertexCount; }

        const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }

        // CPU data access (only available if retainCPUData was true)
        bool HasCPUData() const { return !m_Vertices.empty(); }
        const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
        void ReleaseCPUData() { m_Vertices.clear(); m_Vertices.shrink_to_fit(); m_Indices.clear(); m_Indices.shrink_to_fit(); }

        bool Reload() override;

        static AssetType GetStaticType() { return AssetType::Mesh; }

    private:
        bool LoadFromFile(const String& filePath);

    private:
        Ref<VertexArray> m_VertexArray;
        uint32_t m_IndexCount = 0;
        uint32_t m_VertexCount = 0;
        String m_FilePath;
        bool m_IsFromFile = false;
        bool m_RetainCPUData = false;

        // Optional CPU-side data retention for voxelization etc.
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
    };
}
