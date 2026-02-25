#pragma once

#include "Aether/Renderer/FrameBuffer.h" 
#include "Platform/OpenGL/OpenGLBase.h"

namespace Aether {

	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FramebufferSpec& spec);
		virtual ~OpenGLFrameBuffer();

		virtual void Invalidate() override;

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

        virtual void BindDepthTexture(uint32_t slot = 0) const override { m_DepthAttachment->Bind(slot); }
		virtual void BindColorTexture(uint32_t slot = 0, uint32_t index = 0) const override;

		virtual Ref<Texture2D> GetColorAttachment(uint32_t index = 0) const override 
		{ AE_CORE_ASSERT(index < m_ColorAttachments.size(), "Color attachment index out of range!"); return m_ColorAttachments[index]; }

        virtual Ref<Texture2D> GetDepthAttachment() const override { return m_DepthAttachment; }

		virtual const FramebufferSpec& GetSpecification() const override { return m_Spec; }
	private:
		uint32_t m_RendererID = 0;
		FramebufferSpec m_Spec;

		std::vector<TextureSpec> m_ColorAttachmentSpec;
		TextureSpec m_DepthAttachmentSpec;

		std::vector<Ref<Texture2D>> m_ColorAttachments;
		Ref<Texture2D> m_DepthAttachment = 0;
	};

}
