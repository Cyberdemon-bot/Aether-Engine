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
        GetTextures().reserve(128);
        AE_CORE_INFO("TextureLibrary initialized");
    }

    void Texture2DLibrary::Shutdown()
    {
        GetTextures().clear();
    }

    Ref<Texture2D> Texture2DLibrary::Load(const std::string& filepath, UUID id, bool wrapMode, bool flip)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
            return textures[id];

        auto texture = Texture2D::Create(filepath, wrapMode, flip);
        
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to load '{0}'", filepath);
            return nullptr;
        }
        textures[id] = texture;
        return texture;
    }
    Ref<Texture2D> Texture2DLibrary::Load(void* data, size_t size, UUID id)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
            return textures[id];

        auto texture = Texture2D::Create(data, size);
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to load from raw packed data");
            return nullptr;
        }
        textures[id] = texture;
        return texture;
    }
	Ref<Texture2D> Texture2DLibrary::Load(const TextureSpec& spec, UUID id)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
            return textures[id];

        auto texture = Texture2D::Create(spec);
        if (!texture || !texture->IsLoaded())
        {
            AE_CORE_ERROR("Texture Library: Failed to create empty texture");
            return nullptr;
        }
        textures[id] = texture;
        return texture;
    }

    Ref<Texture2D> Texture2DLibrary::Get(UUID id)
    {
        auto& textures = GetTextures();
        if (textures.find(id) != textures.end())
            return textures[id];
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
