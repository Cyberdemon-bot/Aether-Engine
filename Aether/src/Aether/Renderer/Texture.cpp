#include "aepch.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

	Scope<Texture2D> Texture2D::CreateImpl(const TextureCreateInfo& specification)
	{
		switch (ServiceManager::GetService<Renderer>()->GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLTexture2D>(specification);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Scope<Texture2D> Texture2D::CreateImpl(const std::string& path, WrapMode mode, bool flip)
	{
		switch (ServiceManager::GetService<Renderer>()->GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLTexture2D>(path, mode, flip);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Scope<TextureCube> TextureCube::CreateImpl(const std::string& path)
    {
        switch (ServiceManager::GetService<Renderer>()->GetAPI())
        {
            case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
            case RendererAPI::API::OpenGL:  return CreateScope<OpenGLTextureCube>(path);
        }

        AE_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}
