#pragma once

#include <vector>
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/ResourceManager.h"

namespace Aether {

	struct FramebufferSpec
	{
		uint32_t Width = 0, Height = 0;
		uint32_t Samples = 1;
		std::vector<TextureCreateInfo> Attachments;
		bool SwapChainTarget = false;
	};

	class AETHER_API FrameBuffer : public Resource
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

		virtual Texture2D* GetColorAttachment(uint32_t index = 0) const = 0;
        virtual Texture2D* GetDepthAttachment() const = 0;

		virtual const FramebufferSpec& GetSpecification() const = 0;

		template<typename... Args>
        static Ref<FrameBuffer> Create(Args&&... args)
        {
            Scope<FrameBuffer> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<FrameBuffer>(std::move(scope));
        }

	private:
		static Scope<FrameBuffer> CreateImpl(const FramebufferSpec& spec);
		friend class ResourceManager;

	protected:
		Scope<Texture2D> CreateTexture(TextureCreateInfo spec) 
        {
            return Texture2D::CreateImpl(spec); 
        }
	};

}
