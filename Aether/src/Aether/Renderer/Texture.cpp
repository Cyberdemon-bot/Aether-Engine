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

	void Texture2DLibrary::Init()
    {
        GetTextures().reserve(128);
        AE_CORE_INFO("TextureLibrary initialized");
    }

    void Texture2DLibrary::Shutdown()
    {
        GetTextures().clear();
    }

    void Texture2DLibrary::Add(Ref<Texture2D> obj, UUID id)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
        {
            AE_CORE_ERROR("Texture2D Library: ID already exists");
            return;
        }

        if (!obj)
        {
            AE_CORE_ERROR("Texture2D Library: Cannot add null obj");
            return;
        }
        textures[id] = obj;
    }

    Ref<Texture2D> Texture2DLibrary::Get(UUID id)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
            return textures[id];
        
        AE_CORE_WARN("Texture2D Library: ID not found!");
        return nullptr;
    }

    bool Texture2DLibrary::Exists(UUID id)
    {
        auto& textures = GetTextures();
        return textures.find(id) != textures.end();
    }

    std::unordered_map<UUID, Ref<Texture2D>>& Texture2DLibrary::GetTextures()
    {
        static std::unordered_map<UUID, Ref<Texture2D>> s_Textures;
        return s_Textures;
    }
}
