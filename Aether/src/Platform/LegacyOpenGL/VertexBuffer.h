#pragma once

#include "LegacyBase.h"

namespace Aether::Legacy { 
    class AETHER_API VertexBuffer
    {
    private:
        unsigned int m_RendererID;

    public:
        VertexBuffer(const void* data, unsigned int size);
        ~VertexBuffer();

        void Bind() const;
        void Unbind() const;
    };
}