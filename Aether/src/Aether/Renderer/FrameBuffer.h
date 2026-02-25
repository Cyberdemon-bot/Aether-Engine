#pragma once

#include "Aether/Renderer/Texture.h"
#include <vector>

namespace Aether {

	struct FramebufferSpec
	{
		uint32_t Width = 0, Height = 0;
		uint32_t Samples = 1;
		std::vector<TextureSpec> Attachments;
		bool SwapChainTarget = false;
	};

	class AETHER_API FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Invalidate() = 0;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

        virtual void BindDepthTexture(uint32_t slot = 0) const = 0;
		virtual void BindColorTexture(uint32_t slot = 0, uint32_t index = 0) const = 0;

		virtual Ref<Texture2D> GetColorAttachment(uint32_t index = 0) const = 0;
        virtual Ref<Texture2D> GetDepthAttachment() const = 0;

		virtual const FramebufferSpec& GetSpecification() const = 0;

        static Ref<FrameBuffer> Create(const FramebufferSpec& spec);
	};

}
