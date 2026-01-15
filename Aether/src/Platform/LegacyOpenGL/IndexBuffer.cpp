#include "IndexBuffer.h"

namespace Aether::Legacy {
    IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
        : m_Count(count)
    {
        ASSERT(sizeof(unsigned int) == sizeof(GLuint));

        GLCall(glGenBuffers(1, &m_RendererID));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_DYNAMIC_DRAW));
    }

    IndexBuffer::~IndexBuffer()
    {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }

    void IndexBuffer::Bind() const
    {
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
    }

    void IndexBuffer::Unbind() const
    {
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    void IndexBuffer::SetData(const void* data, unsigned int size, unsigned int offset)
    {
        AE_CORE_ASSERT(offset + size <= m_Count, 
            "IndexBuffer::SetData - Trying to write out of bounds!");
        
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(unsigned int), m_Count * sizeof(unsigned int), data));
    }
}