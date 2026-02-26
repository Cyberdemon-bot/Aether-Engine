#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Aether {

	Ref<Texture2D> Texture2D::Create(const TextureSpec& specification)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(specification);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, WrapMode mode, bool flip)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path, mode, flip);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(void* data, size_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(data, size);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
            case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTextureCube>(path);
        }

        AE_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}
