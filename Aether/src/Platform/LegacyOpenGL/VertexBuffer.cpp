#include "VertexBuffer.h"

namespace Aether::Legacy { 
    VertexBuffer::VertexBuffer(const void* data, unsigned int size)
    {
        GLCall(glGenBuffers(1, &m_RendererID));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW));
    }

    VertexBuffer::~VertexBuffer()
    {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }

    void VertexBuffer::Bind() const
    {
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    }

    void VertexBuffer::Unbind() const
    {
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }

    void VertexBuffer::SetData(const void* data, unsigned int size, unsigned int offset)
    {
        AE_CORE_ASSERT(offset + size <= m_Size, 
            "VertexBuffer::SetData - Trying to write out of bounds!");
        
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    }
}