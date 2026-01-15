#include "TextureCube.h"
#include <stb_image.h>
#include <vector>
#include <iostream>

namespace Aether::Legacy {

    TextureCube::TextureCube(const std::string& path)
        : m_RendererID(0)
    {
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

        int width, height, channels;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            std::cerr << "Cubemap load failed: " << path << std::endl;
            return;
        }

        int faceSize = width / 4;
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        std::cout << "Loading cubemap: " << width << "x" << height 
                  << ", face size: " << faceSize << std::endl;

        // Standard cross layout positions:
        //     [top]
        // [left][front][right][back]
        //     [bottom]
        
        struct FaceExtract {
            GLenum target;
            int gridX, gridY;  // Position in the cross
            bool flipX, flipY, rotate90;
        };

        FaceExtract faces[6] = {
            { GL_TEXTURE_CUBE_MAP_POSITIVE_X, 2, 1, false, false, false },  // Right
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 1, false, false, false },  // Left
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 1, 0, false, false, false },  // Top
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 1, 2, false, false, false },  // Bottom
            { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 1, 1, false, false, false },  // Front
            { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 3, 1, false, false, false }   // Back
        };

        std::vector<unsigned char> faceData(faceSize * faceSize * channels);

        for (int i = 0; i < 6; i++) {
            const auto& f = faces[i];
            
            // Extract face from cross layout
            for (int y = 0; y < faceSize; y++) {
                for (int x = 0; x < faceSize; x++) {
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
        
        std::cout << "Cubemap loaded successfully!" << std::endl;
    }

    TextureCube::~TextureCube() {
        glDeleteTextures(1, &m_RendererID);
    }

    void TextureCube::Bind(unsigned int slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
    }

    void TextureCube::Unbind() const {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

}