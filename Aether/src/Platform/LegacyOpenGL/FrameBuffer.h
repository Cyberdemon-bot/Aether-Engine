#pragma once

#include "LegacyBase.h"

namespace Aether::Legacy {


    enum class FramebufferTextureFormat
	{
		None = 0,
		RGBA8,
		RGBA16F,
		RED_INTEGER,
		DEPTH24STENCIL8,
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0, Height = 0;
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class AETHER_API FrameBuffer
	{
	public:
		FrameBuffer(const FramebufferSpecification& spec);
		 ~FrameBuffer();

		void Invalidate();

		void Bind() ;
		void Unbind() ;

		void Resize(uint32_t width, uint32_t height) ;
		int ReadPixel(uint32_t attachmentIndex, int x, int y) ;

		void ClearAttachment(uint32_t attachmentIndex, int value) ;

        void BindDepthTexture(uint32_t slot = 0) const;

		uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const  { AE_CORE_ASSERT(index < m_ColorAttachments.size()); return m_ColorAttachments[index]; }
        uint32_t GetDepthAttachmentRendererID() const { return m_DepthAttachment; }

		const FramebufferSpecification& GetSpecification() const  { return m_Specification; }
	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecification m_Specification;

		std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};

}
