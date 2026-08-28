#include "aepch.h"
#include "GLTFConverter.h"
#include "Aether/Core/Assert.h"
#include "Aether/Renderer/Texture.h" 

#include <cgltf.h>
#include <stb_image.h>

namespace Aether {

    void GLTFConverter::ParseMaterials(cgltf_data* gltf, ParsedScene& scene)
    {
        scene.Images.reserve(gltf->images_count);
        scene.ImagePixels.reserve(gltf->images_count);

        stbi_set_flip_vertically_on_load(0);

        for (size_t i = 0; i < gltf->images_count; i++)
        {
            const cgltf_image* image = &gltf->images[i];

            AImageCreateInfo imgInfo;
            imgInfo.id = UUID();
            imgInfo.debugName = image->name ? image->name : ("Texture_" + std::to_string(i));

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
            else if (image->uri)
            {
                AE_CORE_ERROR("GLTFConverter: texture URI loading not supported yet ('{0}')", image->uri);
            }

            scene.ImagePixels.push_back(std::move(pixels));
            imgInfo.raw = std::span<const uint8_t>(scene.ImagePixels.back());

            scene.Images.push_back(imgInfo);
        }

        scene.Materials.reserve(gltf->materials_count);

        for (size_t i = 0; i < gltf->materials_count; i++)
        {
            const cgltf_material* mat = &gltf->materials[i];

            AMaterialCreateInfo matInfo;
            matInfo.id = UUID();
            matInfo.debugName = mat->name ? mat->name : ("Material_" + std::to_string(i));

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
                    matInfo.albedoMap = scene.Images[imgIdx].id;
                }
                if (pbr.metallic_roughness_texture.texture)
                {
                    size_t imgIdx = pbr.metallic_roughness_texture.texture->image - gltf->images;
                    matInfo.metallicRoughnessMap = scene.Images[imgIdx].id;
                }
            }

            if (mat->normal_texture.texture)
            {
                size_t imgIdx = mat->normal_texture.texture->image - gltf->images;
                matInfo.normalMap = scene.Images[imgIdx].id;
            }

            scene.Materials.push_back(matInfo);
        }
    }
}