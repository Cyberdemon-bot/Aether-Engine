#pragma once

#include "Aether/Renderer/FrameBuffer.h" 
#include "Platform/OpenGL/OpenGLBase.h"
#include "Aether/Core/Assert.h"

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

		virtual Texture2D* GetColorAttachment(uint32_t index = 0) const override 
		{ AE_CORE_ASSERT(index < m_ColorAttachments.size(), "Color attachment index out of range!"); return m_ColorAttachments[index].get(); }

        virtual Texture2D* GetDepthAttachment() const override { return m_DepthAttachment.get(); }

		virtual const FramebufferSpec& GetSpecification() const override { return m_Spec; }
	private:
		uint32_t m_RendererID = 0;
		FramebufferSpec m_Spec;

		std::vector<TextureCreateInfo> m_ColorAttachmentSpec;
		TextureCreateInfo m_DepthAttachmentSpec;

		std::vector<Scope<Texture2D>> m_ColorAttachments;
		Scope<Texture2D> m_DepthAttachment = 0;
	};

}
