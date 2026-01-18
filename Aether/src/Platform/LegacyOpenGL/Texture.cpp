#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include <stb_image.h>

namespace Aether::Legacy {
    Texture::Texture(const std::string& path, bool isHDR)
        : m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0)
    {
        stbi_set_flip_vertically_on_load(1);
        void* data = nullptr;
        GLenum internalFormat = 0;
        GLenum dataFormat = 0;
        GLenum type = 0;

        if (isHDR)
        {
            data = stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_BPP, 0);
            internalFormat = GL_RGB16F;
            dataFormat = GL_RGB;
            type = GL_FLOAT;
        }
        else
        {
            data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
        }
        
        m_LocalBuffer = (unsigned char*)data; 

        if (data)
        {
            GLCall(glGenTextures(1, &m_RendererID));
            GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

            GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, type, data));
            
            GLCall(glBindTexture(GL_TEXTURE_2D, 0));
            stbi_image_free(data);
        }
        else AE_CORE_ERROR("Failed to load texture: {0}", path);
    }

    Texture::~Texture()
    {
        GLCall(glDeleteTextures(1, &m_RendererID));
    }

    void Texture::Bind(unsigned int slot) const
    {
        GLCall(glActiveTexture(GL_TEXTURE0 + slot));
        GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
    }

    void Texture::Unbind() const
    {
        GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    }
}