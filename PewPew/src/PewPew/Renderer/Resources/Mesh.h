#pragma once
#include "VertexArray.h"
#include "PewPew/Core/Core.h"
#include "PewPew/Core/String.h"
#include "PewPew/Math/CoreMath.h"

#include <vector>

namespace PewPew
{
    struct Vertex
    {
        Vector3 Position;
        Vector3 Normal;
        Vector2 TexCoords;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector3 Color = Vector3(1.0f);  // Vertex color (default white)
    };

    class Mesh
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        ~Mesh() = default;

        static Ref<Mesh> Load(const String& filePath);

        void Bind() const;
        uint32_t GetIndexCount() const { return m_IndexCount; }

        const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }

    private:
        Ref<VertexArray> m_VertexArray;
        uint32_t m_IndexCount = 0;
    };
}
