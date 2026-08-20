#include "Platform/OpenGL/OpenGLTexture.h"
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
                case ImageFormat::RED_INTEGER: return GL_RED_INTEGER;
                case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL;
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
                case ImageFormat::RED_INTEGER: return GL_R32I;
                case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
			}

			AE_CORE_ASSERT(false, "Unknown ImageFormat GL internal type!");
			return 0;
		}

        static GLenum WrapModeToGLMode(WrapMode mode)
        {
            switch (mode)
            {
                case WrapMode::None: break;
                case WrapMode::REPEAT: return GL_REPEAT;
                case WrapMode::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
            }

            AE_CORE_ASSERT(false, "Unknown WrapMode GL type!");
            return 0;
        }

        static GLenum ImageFormatToDataType(ImageFormat format)
        {
            switch (format)
			{
                case ImageFormat::None:  break;
                case ImageFormat::RGB8:  
                case ImageFormat::RGBA8: return GL_UNSIGNED_BYTE;
                case ImageFormat::RGBA16F: 
                case ImageFormat::RGBA32F: return GL_FLOAT;
                case ImageFormat::RED_INTEGER: return GL_INT;
                case ImageFormat::DEPTH24STENCIL8: return GL_UNSIGNED_INT_24_8;
			}

			AE_CORE_ASSERT(false, "Unknown ImageFormat GL internal type!");
			return 0;
        }
    }

    static void CreateTexture(uint32_t& rendererID, uint32_t width, uint32_t height, int samples,
        GLenum format, GLenum internal_format, GLenum datatype, GLenum wrapmode, bool gen_mipmap, void* data)
    {
        if (samples > 1)
        {
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internal_format, width, height, GL_FALSE);
            return;
        }
        GLCall(glGenTextures(1, &rendererID));
        GLCall(glBindTexture(GL_TEXTURE_2D, rendererID));
        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, datatype, data));

        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapmode));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapmode));

        if (gen_mipmap)
        {
            GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            GLCall(glGenerateMipmap(GL_TEXTURE_2D));
        }
        else GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    }

    //texture
    OpenGLTexture2D::OpenGLTexture2D(const TextureCreateInfo& spec)
        : m_Width(spec.Width), m_Height(spec.Height)
    {
        m_InternalFormat = Utils::ImageFormatToGLInternalFormat(spec.Format);
        m_DataFormat = Utils::ImageFormatToGLDataFormat(spec.Format);

        GLenum dataType = Utils::ImageFormatToDataType(spec.Format);
        GLenum glWrapMode = Utils::WrapModeToGLMode(spec.Mode);
        CreateTexture(m_RendererID, m_Width, m_Height, spec.Samples, m_DataFormat, m_InternalFormat, dataType, glWrapMode, spec.GenerateMips, nullptr);
    }

    OpenGLTexture2D::OpenGLTexture2D(void* data, size_t size)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(0);

        bool isHDR = stbi_is_hdr_from_memory((const stbi_uc*)data, (int)size);

        void* pixelData = nullptr;
        ImageFormat format;
        
        if (isHDR)
        {
           
            pixelData = stbi_loadf_from_memory((const stbi_uc*)data, (int)size, &width, &height, &channels, 4);
            format = ImageFormat::RGBA16F;
        }
        else
        {
            pixelData = stbi_load_from_memory((const stbi_uc*)data, (int)size, &width, &height, &channels, 4);
            format = ImageFormat::RGBA8;
        }

        if (pixelData)
        {
            m_IsLoaded = true;
            m_Width = width;
            m_Height = height;
            m_InternalFormat = Utils::ImageFormatToGLInternalFormat(format);
            m_DataFormat = Utils::ImageFormatToGLDataFormat(format);
            GLenum dataType = Utils::ImageFormatToDataType(format);
            CreateTexture(m_RendererID, m_Width, m_Height, 1, m_DataFormat, m_InternalFormat, dataType, GL_REPEAT, false, pixelData);
            stbi_image_free(pixelData);
        }
        else AE_CORE_ERROR("Fail to create texture from packed data");
    }

    OpenGLTexture2D::OpenGLTexture2D(const std::string& path, WrapMode mode, bool flip)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(flip);
        bool isHDR = stbi_is_hdr(path.c_str());
        
        void* data = nullptr;
        ImageFormat format;

        if (isHDR)
        {
            data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
            format = ImageFormat::RGBA16F;
        }
        else
        {
            data = stbi_load(path.c_str(), &width, &height, &channels, 0); 
            if (channels == 4) format = ImageFormat::RGBA8;
            else if (channels == 3) format = ImageFormat::RGB8;
        }

        if (data)
        {
            m_IsLoaded = true;
            m_Width = width;
            m_Height = height;
            m_InternalFormat = Utils::ImageFormatToGLInternalFormat(format);
            m_DataFormat = Utils::ImageFormatToGLDataFormat(format);

            GLenum dataType = Utils::ImageFormatToDataType(format);
            GLenum glWrapMode = Utils::WrapModeToGLMode(mode);
            CreateTexture(m_RendererID, m_Width, m_Height, 1, m_DataFormat, m_InternalFormat, dataType, glWrapMode, false, data);
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