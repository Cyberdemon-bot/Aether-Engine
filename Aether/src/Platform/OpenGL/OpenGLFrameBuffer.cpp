#include "Platform/OpenGL/OpenGLFrameBuffer.h"

namespace Aether {

	static const uint32_t s_MaxFramebufferSize = 8192;

	OpenGLFrameBuffer::OpenGLFrameBuffer(const FramebufferSpec& spec)
		: m_Spec(spec)
	{
		for (auto spec : m_Spec.Attachments)
		{
			if (spec.Format != ImageFormat::Depth) m_ColorAttachmentSpec.emplace_back(spec);
			else m_DepthAttachmentSpec = spec;
		}

		Invalidate();
	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		for (auto& attachment : m_ColorAttachments) attachment.reset();
		m_DepthAttachment.reset();
	}

	void OpenGLFrameBuffer::BindColorTexture(uint32_t slot, uint32_t index) const
	{
		AE_CORE_ASSERT(index < m_ColorAttachments.size(), "Index out of range!");
		m_ColorAttachments[index]->Bind(slot);
	}

	void OpenGLFrameBuffer::Invalidate()
	{
		if (m_RendererID)
		{
			glDeleteFramebuffers(1, &m_RendererID);
			for (auto& attachment : m_ColorAttachments) attachment.reset();
			m_DepthAttachment.reset();
			m_ColorAttachments.clear();
		}

		glGenFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		if (m_ColorAttachmentSpec.size())
		{
			m_ColorAttachments.resize(m_ColorAttachmentSpec.size());
			for (size_t i = 0; i < m_ColorAttachments.size(); i++) 
			{
				auto& spec = m_ColorAttachmentSpec[i];
				spec.Width = m_Spec.Width;
				spec.Height = m_Spec.Height;
				spec.Samples = m_Spec.Samples;
				m_ColorAttachments[i] = CreateTexture(spec);
				GLenum textureTarget = spec.Samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textureTarget, m_ColorAttachments[i]->GetRendererID(), 0);
			}
		}

		if (m_DepthAttachmentSpec.Format == ImageFormat::Depth)
		{
			auto& spec = m_DepthAttachmentSpec;
			spec.Width = m_Spec.Width;
			spec.Height = m_Spec.Height;
			spec.Samples = m_Spec.Samples;
			m_DepthAttachment = CreateTexture(spec);
			GLenum textureTarget = spec.Samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureTarget, m_DepthAttachment->GetRendererID(), 0);
		}

		if (m_ColorAttachments.size() > 1)
		{
			AE_CORE_ASSERT(m_ColorAttachments.size() <= 4, "Too much m_ColorAttachments!");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_ColorAttachments.size(), buffers);
		}
		else if (m_ColorAttachments.empty())
		{
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
		}

		AE_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Spec.Width, m_Spec.Height);
	}

	void OpenGLFrameBuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			AE_CORE_WARN("Attempted to rezize framebuffer to {0}, {1}", width, height);
			return;
		}
		m_Spec.Width = width;
		m_Spec.Height = height;
		
		Invalidate();
	}

	int OpenGLFrameBuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		AE_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "attachmentIndex out of range!");

		GLint lastReadBuffer;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &lastReadBuffer);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
		
		glBindFramebuffer(GL_READ_FRAMEBUFFER, lastReadBuffer);
		return pixelData;
	}

	void OpenGLFrameBuffer::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		AE_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "attachmentIndex out of range!");

		GLint lastDrawFramebuffer;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastDrawFramebuffer);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_RendererID);
		
		glClearBufferiv(GL_COLOR, attachmentIndex, &value);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lastDrawFramebuffer);
	}

}