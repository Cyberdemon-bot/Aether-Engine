#include "Platform/OpenGL/OpenGLTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Aether {
    namespace Utils {
        static GLenum ImageFormatToGLDataFormat(ImageFormat format)
		{
			switch (format)
			{
                case ImageFormat::None:  break;
				case ImageFormat::RGB8:  return GL_RGB;
				case ImageFormat::RGBA8: return GL_RGBA;
                case ImageFormat::RGBA16F: return GL_RGBA;
                case ImageFormat::RGBA32F: return GL_RGBA;
			}

			AE_CORE_ASSERT(false, "Unknown ImageFormat GL type!");
			return 0;
		}
		
		static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
		{
			switch (format)
			{
                case ImageFormat::None:  break;
                case ImageFormat::RGB8:  return GL_RGB8;
                case ImageFormat::RGBA8: return GL_RGBA8;
                case ImageFormat::RGBA16F: return GL_RGBA16F;
                case ImageFormat::RGBA32F: return GL_RGBA32F;
			}

			AE_CORE_ASSERT(false, "Unknown ImageFormat GL internal type!");
			return 0;
		}
    }

    //texture
    OpenGLTexture2D::OpenGLTexture2D(const TextureSpec& spec)
        : m_Spec(spec), m_Width(m_Spec.Width), m_Height(m_Spec.Height)
    {
        m_InternalFormat = Utils::ImageFormatToGLInternalFormat(m_Spec.Format);
        m_DataFormat = Utils::ImageFormatToGLDataFormat(m_Spec.Format);

        GLenum dataType = GL_UNSIGNED_BYTE;
        if (m_Spec.Format == ImageFormat::RGBA16F || m_Spec.Format == ImageFormat::RGBA32F) dataType = GL_FLOAT;
        GLenum glWrapMode = m_Spec.WrapMode ? GL_CLAMP_TO_EDGE : GL_REPEAT;

        GLCall(glGenTextures(1, &m_RendererID));
        GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, dataType, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapMode);
    }

    OpenGLTexture2D::OpenGLTexture2D(void* data, size_t size)
    {
        int width, height, channels;

        stbi_set_flip_vertically_on_load(0); 
        stbi_uc* imgData = stbi_load_from_memory((const stbi_uc*)data, (int)size, &width, &height, &channels, 0);

        if (imgData)
        {
            m_IsLoaded = true;
            m_Width = width;
            m_Height = height;

            GLenum type = GL_UNSIGNED_BYTE;
            if (channels == 4)
            {
                m_InternalFormat = GL_RGBA8;
                m_DataFormat = GL_RGBA;
                m_Spec.Format = ImageFormat::RGBA8;
            }
            else if (channels == 3)
            {
                m_InternalFormat = GL_RGB8;
                m_DataFormat = GL_RGB;
                m_Spec.Format = ImageFormat::RGB8;
            }
            else
            {
                m_InternalFormat = GL_RGBA8;
                m_DataFormat = GL_RGBA;
                m_Spec.Format = ImageFormat::RGBA8;
            }

            m_Spec.Width = m_Width;
            m_Spec.Height = m_Height;

            GLCall(glGenTextures(1, &m_RendererID));
            GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

            glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, type, imgData);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            stbi_image_free(imgData);
        }
    }

    OpenGLTexture2D::OpenGLTexture2D(const std::string& path, bool wrapMode, bool flip)
    : m_Path(path)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(flip);
        
        bool isHDR = stbi_is_hdr(path.c_str());
        
        void* data = nullptr;
        GLenum type = 0;

        if (isHDR)
        {
            data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
            type = GL_FLOAT;
            m_InternalFormat = GL_RGBA16F; 
            m_DataFormat = GL_RGB; 
        }
        else
        {
            data = stbi_load(path.c_str(), &width, &height, &channels, 0); 
            type = GL_UNSIGNED_BYTE;
            
            if (channels == 4)
            {
                m_InternalFormat = GL_RGBA8;
                m_DataFormat = GL_RGBA;
            }
            else if (channels == 3)
            {
                m_InternalFormat = GL_RGB8;
                m_DataFormat = GL_RGB;
            }
        }

        if (data)
        {
            m_IsLoaded = true;
            m_Width = width;
            m_Height = height;

            GLCall(glGenTextures(1, &m_RendererID));
            GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

            glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, type, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLenum glWrapMode = wrapMode ? GL_CLAMP_TO_EDGE : GL_REPEAT;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapMode);

            stbi_image_free(data);
        }
    }

    OpenGLTexture2D::~OpenGLTexture2D()
	{
		GLCall(glDeleteTextures(1, &m_RendererID));
	}

	void OpenGLTexture2D::SetData(const void* data, uint32_t size)
    {
        GLenum type = GL_UNSIGNED_BYTE;
        uint32_t bpp = 0;
        
        if (m_DataFormat == GL_RGBA) bpp = 4;
        else if (m_DataFormat == GL_RGB) bpp = 3;

        if (m_InternalFormat == GL_RGBA16F || m_InternalFormat == GL_RGBA32F)
        {
            type = GL_FLOAT;
            bpp *= sizeof(float); 
        }

        AE_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
        
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, type, data);
    }

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
        GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
	}

    //cube
    OpenGLTextureCube::OpenGLTextureCube(const std::string& path)
    : m_Path(path)
    {
        GLCall(glGenTextures(1, &m_RendererID));
        GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));

        int width, height, channels;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data)
        {
            AE_CORE_ERROR("Cubemap load failed: {0}", path);
            return;
        }

        int faceSize = width / 4;
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        AE_CORE_INFO("Loading cubemap: {0}x{1}, face size: {2}", width, height, faceSize);

        
        struct FaceExtract {
            GLenum target;
            int gridX, gridY;  
        };

        FaceExtract faces[6] = {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, 2, 1 },  
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 1 },  
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 1, 0 },  
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 1, 2 },  
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 1, 1 },  
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 3, 1 }   
        };

        std::vector<unsigned char> faceData(faceSize * faceSize * channels);

        for (int i = 0; i < 6; i++)
        {
            const auto& f = faces[i];
            
            for (int y = 0; y < faceSize; y++)
            {
                for (int x = 0; x < faceSize; x++)
                {
                    int srcX = f.gridX * faceSize + x;
                    int srcY = f.gridY * faceSize + y;
                    
                    int srcIdx = (srcY * width + srcX) * channels;
                    int dstIdx = (y * faceSize + x) * channels;
                    
                    memcpy(&faceData[dstIdx], &data[srcIdx], channels);
                }
            }

            glTexImage2D(
                f.target,
                0,
                format,
                faceSize,
                faceSize,
                0,
                format,
                GL_UNSIGNED_BYTE,
                faceData.data()
            );
        }

        stbi_image_free(data);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        m_IsLoaded = true;
        m_Width = faceSize;
        m_Height = faceSize;
        
        AE_CORE_INFO("Cubemap loaded successfully!");
    }

    OpenGLTextureCube::~OpenGLTextureCube()
    {
        GLCall(glDeleteTextures(1, &m_RendererID));
    }

    void OpenGLTextureCube::Bind(uint32_t slot) const
    {
        GLCall(glActiveTexture(GL_TEXTURE0 + slot));
        GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));
    }
}