#include "aepch.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Aether {
    // vertex buffer
    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
        : m_Size(size)
    {
        GLCall(glGenBuffers(1, &m_RendererID));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW));
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
        : m_Size(size)
    {
        GLCall(glGenBuffers(1, &m_RendererID));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW));
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }

    void OpenGLVertexBuffer::Bind() const
    {
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    }

    void OpenGLVertexBuffer::Unbind() const
    {
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }

    void OpenGLVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        if (offset + size > m_Size) {
            AE_CORE_ERROR("OpenGLVertexBuffer::SetData - Out of bounds! offset={0}, size={1}, buffer_size={2}", 
                offset, size, m_Size);
            AE_CORE_ASSERT(false, "Buffer overflow detected!");
        }
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    }

    void OpenGLVertexBuffer::Resize(uint32_t size)
    {
        m_Size = size;
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    // index buffer
    OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
        : m_Count(count)
    {
        GLCall(glGenBuffers(1, &m_RendererID));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
        GLCall(glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW));
    }
    OpenGLIndexBuffer:: ~OpenGLIndexBuffer()
    {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }

    void OpenGLIndexBuffer::Bind() const
    {
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
    }
    void OpenGLIndexBuffer::Unbind() const
    {
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
}