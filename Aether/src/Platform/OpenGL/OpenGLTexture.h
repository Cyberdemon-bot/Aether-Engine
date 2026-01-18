#pragma once

#include "Aether/Renderer/Texture.h"
#include "Platform/OpenGL/OpenGLBase.h"

namespace Aether {
    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(const TextureSpec& spec);
		OpenGLTexture2D(const std::string& path, bool wrapMode = false);
        virtual ~OpenGLTexture2D();

        virtual const TextureSpec& GetSpec() const override { return m_Spec; }

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }

        virtual const std::string& GetPath() const override { return m_Path; }
        virtual bool IsLoaded() const override { return m_IsLoaded; }

        virtual void SetData(const void* data, uint32_t size) override;
        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool operator==(const Texture& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
    private:
        TextureSpec m_Spec;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		GLenum m_InternalFormat, m_DataFormat;
    };

    // Thêm vào OpenGLTexture.h

    class OpenGLTextureCube : public TextureCube
    {
    public:
        OpenGLTextureCube(const std::string& path);
        virtual ~OpenGLTextureCube();

        // Triển khai các hàm từ class cha Texture
        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        virtual const std::string& GetPath() const override { return m_Path; }
        
        // TextureCube thường ít khi SetData thủ công sau khi load, ta có thể assert false hoặc để trống
        virtual void SetData(const void* data, uint32_t size) override { AE_CORE_ASSERT(false, "Not implemented for Cubemap!"); }

        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool IsLoaded() const override { return m_IsLoaded; }

        virtual const TextureSpec& GetSpec() const override { return m_Spec; } // Trả về spec dummy hoặc lưu spec nếu cần

        virtual bool operator==(const Texture& other) const override
        {
            return m_RendererID == other.GetRendererID();
        }

    private:
        uint32_t m_RendererID;
        std::string m_Path;
        uint32_t m_Width, m_Height; // Kích thước của 1 mặt (Face)
        bool m_IsLoaded = false;
        TextureSpec m_Spec; // Để thỏa mãn interface
    };
}