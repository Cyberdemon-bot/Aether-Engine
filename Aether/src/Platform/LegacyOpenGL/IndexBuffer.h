#pragma once

#include "LegacyBase.h"

namespace Aether::Legacy {
    class AETHER_API IndexBuffer
    {
    private:
        unsigned int m_RendererID;
        unsigned int m_Count;
    public:
        IndexBuffer(const unsigned int* data, unsigned int count);
        ~IndexBuffer();

        void Bind() const;
        void Unbind() const;

        void SetData(const void* data, unsigned int size, unsigned int offset = 0);
        inline unsigned int GetCount() const { return m_Count; }
    };
}