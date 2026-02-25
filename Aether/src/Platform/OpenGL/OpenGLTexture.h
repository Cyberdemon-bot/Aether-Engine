#pragma once

#include "Aether/Renderer/Texture.h"
#include "Platform/OpenGL/OpenGLBase.h"

namespace Aether {
    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const TextureSpec& spec);
        OpenGLTexture2D(void* data, size_t size);
		OpenGLTexture2D(const std::string& path, WrapMode mode, bool flip);
        virtual ~OpenGLTexture2D();

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        virtual bool IsLoaded() const override { return m_IsLoaded; }

        virtual void SetData(const void* data, uint32_t size) override;
        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
    private:
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
    };

    class OpenGLTextureCube : public TextureCube
    {
    public:
        OpenGLTextureCube(const std::string& path);
        virtual ~OpenGLTextureCube();

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        
        virtual void SetData(const void* data, uint32_t size) override { AE_CORE_ASSERT(false, "Not implemented for Cubemap!"); }

        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool IsLoaded() const override { return m_IsLoaded; }
        virtual bool operator==(const Texture& other) const override
        {
            return m_RendererID == other.GetRendererID();
        }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height; 
        bool m_IsLoaded = false;
    };
}