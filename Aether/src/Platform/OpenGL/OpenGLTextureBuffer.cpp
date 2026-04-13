#include "Platform/OpenGL/OpenGLTextureBuffer.h"
#include "Aether/Core/Assert.h"

namespace Aether {
	OpenGLTextureBuffer::OpenGLTextureBuffer(uint32_t size)
		: m_Size(size)
	{
		glGenBuffers(1, &m_BufferID);
		glBindBuffer(GL_TEXTURE_BUFFER, m_BufferID);
		glBufferData(GL_TEXTURE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_BUFFER, m_RendererID);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_BufferID);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}

	OpenGLTextureBuffer::~OpenGLTextureBuffer()
	{
		glDeleteTextures(1, &m_RendererID);
		glDeleteBuffers(1, &m_BufferID);
	}

	void OpenGLTextureBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		AE_CORE_ASSERT(offset + size <= m_Size, "TextureBuffer overflow!");
		glBindBuffer(GL_TEXTURE_BUFFER, m_BufferID);
		glBufferSubData(GL_TEXTURE_BUFFER, offset, size, data);
	}

	void OpenGLTextureBuffer::Bind(uint32_t slot) const
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_BUFFER, m_RendererID);
	}

    void OpenGLTextureBuffer::Resize(uint32_t size)
    {
        if (size == m_Size)
            return;

        m_Size = size;

        glBindBuffer(GL_TEXTURE_BUFFER, m_BufferID);
        glBufferData(GL_TEXTURE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindTexture(GL_TEXTURE_BUFFER, m_RendererID);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_BufferID);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
}