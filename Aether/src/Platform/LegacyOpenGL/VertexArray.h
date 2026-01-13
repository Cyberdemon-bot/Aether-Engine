#pragma once

#include "VertexBuffer.h"

namespace Aether::Legacy {
    class VertexBufferLayout;

    class AETHER_API VertexArray
    {
    private:
        unsigned int m_RendererID;
    public:
        VertexArray();
        ~VertexArray();

        void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

        void Bind() const;
        void Unbind() const;
    };
}