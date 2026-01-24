#include "Aether/Resources/Texture.h"
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

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool wrapMode, bool flip)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLTexture2D>(path, wrapMode, flip);
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
        AE_CORE_INFO("TextureLibrary initialized");
    }

    void Texture2DLibrary::Shutdown()
    {
        s_Textures.clear();
    }

    Ref<Texture2D> Texture2DLibrary::Load(const std::string& filepath, UUID id, bool wrapMode, bool flip)
    {
        if (s_Textures.find(id) != s_Textures.end())
            return s_Textures[id];

        auto texture = Texture2D::Create(filepath, wrapMode, flip);
        
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to load '{0}'", filepath);
            return nullptr;
        }
        s_Textures[id] = texture;
        return texture;
    }
    Ref<Texture2D> Texture2DLibrary::Load(void* data, size_t size, UUID id)
    {
        if (s_Textures.find(id) != s_Textures.end())
            return s_Textures[id];

        auto texture = Texture2D::Create(data, size);
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to load from raw packed data");
            return nullptr;
        }
        s_Textures[id] = texture;
        return texture;
    }
	Ref<Texture2D> Texture2DLibrary::Load(const TextureSpec& spec, UUID id)
    {
        if (s_Textures.find(id) != s_Textures.end())
            return s_Textures[id];

        auto texture = Texture2D::Create(spec);
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to create empty texture");
            return nullptr;
        }
        s_Textures[id] = texture;
        return texture;
    }

    Ref<Texture2D> Texture2DLibrary::Get(UUID id)
    {
        if (s_Textures.find(id) != s_Textures.end())
            return s_Textures[id];
        return nullptr;
    }

    bool Texture2DLibrary::Exists(UUID id)
    {
        return s_Textures.find(id) != s_Textures.end();
    }

	std::unordered_map<UUID, Ref<Texture2D>> Texture2DLibrary::s_Textures;
}
