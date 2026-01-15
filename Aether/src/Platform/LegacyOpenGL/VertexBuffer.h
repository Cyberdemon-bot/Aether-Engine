#pragma once

#include "LegacyBase.h"

namespace Aether::Legacy { 
    class AETHER_API VertexBuffer
    {
    private:
        unsigned int m_RendererID;
        unsigned int m_Size;

    public:
        VertexBuffer(const void* data, unsigned int size);
        ~VertexBuffer();

        void Bind() const;
        void Unbind() const;
        void SetData(const void* data, unsigned int size, unsigned int offset = 0);
        inline unsigned int GetSize() const { return m_Size; }
    };
}