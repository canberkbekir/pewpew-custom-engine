#pragma once
#include "Buffer.h"

namespace CB
{
    class VertexArray
    {
    public:
        virtual ~VertexArray()
        {
        }

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
        virtual void AddInstancedVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, uint32_t divisor = 1) = 0;
        virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;

        virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

        static Ref<VertexArray> Create();
    };
}
