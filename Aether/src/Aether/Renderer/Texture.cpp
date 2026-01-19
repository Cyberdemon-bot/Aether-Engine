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

	Ref<Texture2D> Texture2DLibrary::Load(const std::string& name, const std::string& filepath, bool wrapMode, bool flip)
	{
		if (Exists(name)) return m_Textures[name];
		auto tex = Texture2D::Create(filepath, wrapMode, flip);
		if (!tex || !tex->IsLoaded()) 
		{
			AE_CORE_ERROR("Texture failed to load: {0}", filepath);
			return m_ErrorTexture; 
		}
        m_Textures[name] = tex;
        return tex;
	}

	Ref<Texture2D> Texture2DLibrary::Get(const std::string& name)
	{
		auto it = m_Textures.find(name);
        if (it == m_Textures.end()) return m_ErrorTexture; 
        return it->second;
	}

	bool Texture2DLibrary::Exists(const std::string& name) const
	{
		return m_Textures.find(name) != m_Textures.end();
	}
}
