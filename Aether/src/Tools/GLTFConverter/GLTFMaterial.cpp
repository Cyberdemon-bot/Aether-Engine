#include "aepch.h"
#include "GLTFConverter.h"
#include "Aether/Core/Assert.h"
#include "Aether/Renderer/Texture.h"

#include <cgltf.h>
#include <stb_image.h>

namespace Aether {

    void GLTFConverter::ParseMaterials(cgltf_data* gltf, GLTFResult& result)
    {
        result.ImagePixels.reserve(gltf->images_count);
        stbi_set_flip_vertically_on_load(0);

        result.AppendKind<AImageCreateInfo>(AssetType::Image, gltf->images_count, [&](size_t index)
        {
            const cgltf_image* image = &gltf->images[index];
            AImageCreateInfo imgInfo;
            imgInfo.id = UUID();
            imgInfo.debugName = image->name ? image->name : ("Texture_" + std::to_string(index));

            std::vector<uint8_t> pixels;

            if (image->buffer_view)
            {
                const cgltf_buffer_view* view = image->buffer_view;
                const uint8_t* imageData = (const uint8_t*)view->buffer->data + view->offset;

                int width = 0, height = 0, channels = 0;
                stbi_uc* decoded = stbi_load_from_memory(imageData, (int)view->size, &width, &height, &channels, 4);

                if (decoded)
                {
                    imgInfo.layout.Width = width;
                    imgInfo.layout.Height = height;
                    imgInfo.layout.Format = ImageFormat::RGBA8;
                    imgInfo.layout.GenerateMips = false;
                    imgInfo.layout.Mode = WrapMode::CLAMP_TO_EDGE;

                    size_t dataSize = (size_t)width * (size_t)height * 4;
                    pixels.resize(dataSize);
                    memcpy(pixels.data(), decoded, dataSize);

                    stbi_image_free(decoded);
                }
            }
            else if (image->uri) AE_CORE_ERROR("GLTFConverter: texture URI loading not supported yet ('{0}')", image->uri);

            result.ImagePixels.push_back(std::move(pixels));
            imgInfo.raw = std::span<const uint8_t>(result.ImagePixels.back());
            return imgInfo;
        });

        result.AppendKind<AMaterialCreateInfo>(AssetType::Material, gltf->materials_count, [&](size_t index)
        {
            const cgltf_material* mat = &gltf->materials[index];
            AMaterialCreateInfo matInfo;
            matInfo.id = UUID();
            matInfo.debugName = mat->name ? mat->name : ("Material_" + std::to_string(index));

            if (mat->has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& pbr = mat->pbr_metallic_roughness;

                matInfo.albedo = glm::vec4(
                    pbr.base_color_factor[0],
                    pbr.base_color_factor[1],
                    pbr.base_color_factor[2],
                    pbr.base_color_factor[3]
                );
                matInfo.metallic = pbr.metallic_factor;
                matInfo.roughness = pbr.roughness_factor;

                if (pbr.base_color_texture.texture)
                {
                    size_t imgIdx = pbr.base_color_texture.texture->image - gltf->images;
                    if (const auto* img = result.GetAt<AImageCreateInfo>(AssetType::Image, imgIdx))
                        matInfo.albedoMap = img->id;
                }
                if (pbr.metallic_roughness_texture.texture)
                {
                    size_t imgIdx = pbr.metallic_roughness_texture.texture->image - gltf->images;
                    if (const auto* img = result.GetAt<AImageCreateInfo>(AssetType::Image, imgIdx))
                        matInfo.metallicRoughnessMap = img->id;
                }
            }

            if (mat->normal_texture.texture)
            {
                size_t imgIdx = mat->normal_texture.texture->image - gltf->images;
                if (const auto* img = result.GetAt<AImageCreateInfo>(AssetType::Image, imgIdx))
                    matInfo.normalMap = img->id;
            }
            return matInfo;
        });
    }
}