#include "Platform/Cgltf/GLTF_MaterialParser.h"
#include "Platform/Cgltf/GLTF_Utils.h"
#include "aepch.h"
#include <cgltf.h>
#include <stb_image.h>

namespace Aether {
    Ref<ParsedMaterialInfo> GLTF_MaterialParser::Parsing(void* data)
    {
        cgltf_data* gltf = static_cast<cgltf_data*>(data);
        auto result = CreateRef<ParsedMaterialInfo>();
        result->matsInfo.resize(gltf->materials_count);
        result->texsInfo.resize(gltf->images_count);

        // Load textures
        for (size_t i = 0; i < gltf->images_count; i++)
        {
            const cgltf_image* image = &gltf->images[i];
            TextureCreateInfo& texInfo = result->texsInfo[i];
            texInfo.DebugName = image->name ? image->name : ("Texture_" + std::to_string(i));
            stbi_set_flip_vertically_on_load(0);
            
            if (image->buffer_view)
            {
                const cgltf_buffer_view* view = image->buffer_view;
                const uint8_t* imageData = (const uint8_t*)view->buffer->data + view->offset;
                
                int width, height, channels;
                stbi_uc* pixels = stbi_load_from_memory(imageData, (int)view->size, 
                    &width, &height, &channels, 4);
                
                if (pixels)
                {
                    texInfo.Spec.Width = width;
                    texInfo.Spec.Height = height;
                    texInfo.Spec.Format = ImageFormat::RGBA8;
                    texInfo.Spec.GenerateMips = false;
                    texInfo.Spec.Mode = WrapMode::CLAMP_TO_EDGE;
                    
                    size_t dataSize = width * height * 4;
                    texInfo.RawData.resize(dataSize);
                    memcpy(texInfo.RawData.data(), pixels, dataSize);
                    
                    stbi_image_free(pixels);
                }
            }
            else if (image->uri) AE_CORE_ERROR("Texture URI Loading not supported yet!");
        }

        for (size_t i = 0; i < gltf->materials_count; i++)
        {
            const cgltf_material* mat = &gltf->materials[i];
            MaterialCreateInfo& matInfo = result->matsInfo[i];;
            matInfo.DebugName = mat->name ? mat->name : ("Material_" + std::to_string(i));
            
            if (mat->has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& pbr = mat->pbr_metallic_roughness;
                
                matInfo.AlbedoColor = glm::vec4(
                    pbr.base_color_factor[0],
                    pbr.base_color_factor[1],
                    pbr.base_color_factor[2],
                    pbr.base_color_factor[3]
                );
                
                matInfo.Metallic = pbr.metallic_factor;
                matInfo.Roughness = pbr.roughness_factor;
                
                if (pbr.base_color_texture.texture)
                    matInfo.AlbedoMapIdx = (int)(pbr.base_color_texture.texture->image - gltf->images);
                
                if (pbr.metallic_roughness_texture.texture)
                    matInfo.MetallicRoughnessMapIdx = (int)(pbr.metallic_roughness_texture.texture->image - gltf->images);
            }
            
            if (mat->normal_texture.texture) 
                matInfo.NormalMapIdx = (int)(mat->normal_texture.texture->image - gltf->images);
        }

        return result;
    }
}