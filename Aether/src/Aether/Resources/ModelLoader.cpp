#include "aepch.h"
#include "ModelLoader.h"
#include "Aether/Core/AssetsRegister.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <stb_image.h>

namespace Aether {
    ModelLoadResult ModelLoader::Parsing(const std::string& filepath)
    {
        ModelLoadResult modelData = {.FilePath = filepath};
        cgltf_options options = {};
        cgltf_data* data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to parse GLB file: {0}", filepath);
            return modelData;
        }

        result = cgltf_load_buffers(&options, data, filepath.c_str());
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to load GLB buffers: {0}", filepath);
            cgltf_free(data);
            return modelData;
        }

        AE_CORE_INFO("Loaded GLB file: {0}", filepath);
        AE_CORE_INFO("  Meshes: {0}", data->meshes_count);
        AE_CORE_INFO("  Materials: {0}", data->materials_count);
        AE_CORE_INFO("  Textures: {0}", data->textures_count);
        AE_CORE_INFO("  Images: {0}", data->images_count);

        for (size_t i = 0; i < data->images_count; i++)
        {
            cgltf_image* image = &data->images[i];
            TextureCreateInfo texInfo = {.DebugName = std::string("Tex_") + (image->name ? image->name : std::to_string(i))};

            if (image->buffer_view)
            {
                cgltf_buffer_view* view = image->buffer_view;
                const uint8_t* bufferPtr = (const uint8_t*)view->buffer->data + view->offset;
                size_t bufferSize = view->size;
                
                if (bufferPtr)
                {
                    int width, height, channels;
                    stbi_set_flip_vertically_on_load(0);
                    stbi_uc* pixels = stbi_load_from_memory(bufferPtr, (int)bufferSize, &width, &height, &channels, 4);
                    if (pixels)
                    {
                        texInfo.Spec.Width = width;
                        texInfo.Spec.Height = height;
                        texInfo.Spec.Format = ImageFormat::RGBA8; 
                        texInfo.Spec.GenerateMips = true;
                        texInfo.Spec.WrapMode = true; 
                        texInfo.RawData.assign(pixels, pixels + (width * height * 4));
                        stbi_image_free(pixels);
                    }
                }
            
                AE_CORE_INFO("  Loaded embedded texture [{0}]: {1}", i, image->name ? image->name : "unnamed");
            }
            
            modelData.Textures.push_back(std::move(texInfo));
        }

        for (size_t i = 0; i < data->materials_count; i++)
        {
            cgltf_material* mat = &data->materials[i];
            MaterialCreateInfo matInfo = {.DebugName = std::string("Mat_") + (mat->name ? mat->name : std::to_string(i))};
            // Base color texture
            if (mat->pbr_metallic_roughness.base_color_texture.texture)
            {
                cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
                size_t texIndex = tex->image - data->images;
                if (texIndex < modelData.Textures.size()) matInfo.AlbedoMapIdx = texIndex;
            }

            // Base color factor
            glm::vec4 baseColor(
                mat->pbr_metallic_roughness.base_color_factor[0],
                mat->pbr_metallic_roughness.base_color_factor[1],
                mat->pbr_metallic_roughness.base_color_factor[2],
                mat->pbr_metallic_roughness.base_color_factor[3]
            );
            matInfo.AlbedoColor = baseColor;

            // Metallic-Roughness texture
            if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
            {
                cgltf_texture* tex = mat->pbr_metallic_roughness.metallic_roughness_texture.texture;
                size_t texIndex = tex->image - data->images;
                if (texIndex < modelData.Textures.size()) matInfo.MetallicRoughnessMapIdx = texIndex;
            }

            matInfo.Metallic = mat->pbr_metallic_roughness.metallic_factor;
            matInfo.Roughness = mat->pbr_metallic_roughness.roughness_factor;

            // Normal map
            if (mat->normal_texture.texture)
            {
                cgltf_texture* tex = mat->normal_texture.texture;
                size_t texIndex = tex->image - data->images;
                if (texIndex < modelData.Textures.size())  matInfo.NormalMapIdx = texIndex;
            }

            modelData.Materials.push_back(matInfo);
            AE_CORE_INFO("  Created material [{0}]: {1}", i, mat->name ? mat->name : "unnamed");
        }

        for (size_t meshIdx = 0; meshIdx < data->meshes_count; meshIdx++)
        {
            cgltf_mesh* mesh = &data->meshes[meshIdx];
            
            MeshCreateInfo meshInfo = {.DebugName = mesh->name ? mesh->name : ("Mesh_" + std::to_string(meshIdx))};

            AE_CORE_INFO("Processing mesh [{0}]: {1} with {2} primitives", 
                meshIdx, meshInfo.DebugName, mesh->primitives_count);

            uint32_t& totalVertices = meshInfo.totalVertices;
            uint32_t& totalIndices = meshInfo.totalIndices;

            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                cgltf_primitive* prim = &mesh->primitives[primIdx];

                SubMeshCreateInfo subInfo;
                subInfo.BaseVertex = totalVertices;
                subInfo.BaseIndex = totalIndices;
                subInfo.NodeName = meshInfo.DebugName + "_Prim" + std::to_string(primIdx);
                
                // Get material
                if (prim->material)
                {
                    size_t matIndex = prim->material - data->materials;
                    if (matIndex < modelData.Materials.size()) subInfo.MaterialIdx = matIndex;
                }

                std::vector<float> positions;
                std::vector<float> normals;
                std::vector<float> tangents;
                std::vector<float> texCoords;

                // Extract attributes
                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    cgltf_attribute* attr = &prim->attributes[attrIdx];
                    cgltf_accessor* accessor = attr->data;
                    
                    if (attr->type == cgltf_attribute_type_position)
                    {
                        subInfo.VertexCount = (uint32_t)accessor->count;
                        positions.resize(accessor->count * 3);
                        
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            float pos[3];
                            cgltf_accessor_read_float(accessor, v, pos, 3);
                            positions[v * 3 + 0] = pos[0];
                            positions[v * 3 + 1] = pos[1];
                            positions[v * 3 + 2] = pos[2];

                            // Update bounds
                            if (v == 0)
                            {
                                subInfo.BoundsMin = glm::vec3(pos[0], pos[1], pos[2]);
                                subInfo.BoundsMax = subInfo.BoundsMin;
                            }
                            else
                            {
                                subInfo.BoundsMin = glm::min(subInfo.BoundsMin, glm::vec3(pos[0], pos[1], pos[2]));
                                subInfo.BoundsMax = glm::max(subInfo.BoundsMax, glm::vec3(pos[0], pos[1], pos[2]));
                            }
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_normal)
                    {
                        normals.resize(accessor->count * 3);
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            float norm[3];
                            cgltf_accessor_read_float(accessor, v, norm, 3);
                            normals[v * 3 + 0] = norm[0];
                            normals[v * 3 + 1] = norm[1];
                            normals[v * 3 + 2] = norm[2];
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_tangent)
                    {
                        tangents.resize(accessor->count * 4);
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            float tan[4];
                            cgltf_accessor_read_float(accessor, v, tan, 4);
                            tangents[v * 4 + 0] = tan[0];
                            tangents[v * 4 + 1] = tan[1];
                            tangents[v * 4 + 2] = tan[2];
                            tangents[v * 4 + 3] = tan[3];
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0)
                    {
                        texCoords.resize(accessor->count * 2);
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            float uv[2];
                            cgltf_accessor_read_float(accessor, v, uv, 2);
                            texCoords[v * 2 + 0] = uv[0];
                            texCoords[v * 2 + 1] = uv[1];
                        }
                    }
                }

                // Generate missing data if needed
                if (normals.empty() && !positions.empty())
                {
                    normals.resize(positions.size(), 0.0f);
                    for (size_t i = 0; i < positions.size() / 3; i++)
                    {
                        normals[i * 3 + 1] = 1.0f; // Default up
                    }
                }

                if (tangents.empty() && !positions.empty())
                {
                    tangents.resize((positions.size() / 3) * 4, 0.0f);
                    for (size_t i = 0; i < positions.size() / 3; i++)
                    {
                        tangents[i * 4 + 0] = 1.0f; // Default right
                        tangents[i * 4 + 3] = 1.0f; // Handedness
                    }
                }

                if (texCoords.empty() && !positions.empty())
                {
                    texCoords.resize((positions.size() / 3) * 2, 0.0f);
                }

                // Extract indices
                std::vector<uint32_t> indices;
                if (prim->indices)
                {
                    cgltf_accessor* accessor = prim->indices;
                    subInfo.IndexCount = (uint32_t)accessor->count;
                    indices.resize(accessor->count);
                    
                    for (size_t i = 0; i < accessor->count; i++)
                    {
                        indices[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
                    }
                }

                meshInfo.Positions.insert(meshInfo.Positions.end(), positions.begin(), positions.end());
                meshInfo.Normals.insert(meshInfo.Normals.end(), normals.begin(), normals.end());
                meshInfo.Tangents.insert(meshInfo.Tangents.end(), tangents.begin(), tangents.end());
                meshInfo.TexCoords.insert(meshInfo.TexCoords.end(), texCoords.begin(), texCoords.end());
                meshInfo.Indices.insert(meshInfo.Indices.end(), indices.begin(), indices.end());
                
                totalVertices += subInfo.VertexCount;
                totalIndices += subInfo.IndexCount;
                meshInfo.SubMeshes.push_back(subInfo);
            }
            AE_CORE_INFO("Parsed mesh with {0} vertices, {1} indices, {2} submeshes", 
                totalVertices, totalIndices, meshInfo.SubMeshes.size());
            modelData.Meshes.push_back(meshInfo);
        }

        cgltf_free(data);
        return modelData;
    }

   std::vector<UUID> ModelLoader::UploadModel(const ModelLoadResult& modelData, UUID shaderID)
    {
        std::vector<UUID> meshIDs;
        
        // Upload textures
        std::vector<UUID> texIDs;
        for (const auto& texInfo : modelData.Textures)
        {
            UUID texID = AssetsRegister::Register(texInfo.DebugName);
            auto tex = Texture2DLibrary::Load(texInfo.Spec, texID);
            tex->SetData((void*)texInfo.RawData.data(), texInfo.RawData.size());
            texIDs.push_back(texID);
        }
        
        // Upload materials
        std::vector<UUID> matIDs;
        for (const auto& matInfo : modelData.Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName);
            auto material = MaterialLibrary::Load(shaderID, matID);
            
            // Set textures
            if (matInfo.AlbedoMapIdx >= 0 && matInfo.AlbedoMapIdx < texIDs.size())
                material->SetTexture("u_AlbedoMap", texIDs[matInfo.AlbedoMapIdx]);
            
            if (matInfo.NormalMapIdx >= 0 && matInfo.NormalMapIdx < texIDs.size())
            {
                material->SetTexture("u_NormalMap", texIDs[matInfo.NormalMapIdx]);
                material->SetInt("u_HasNormalMap", 1);
            }
            else
            {
                material->SetInt("u_HasNormalMap", 0);
            }
            
            if (matInfo.MetallicRoughnessMapIdx >= 0 && matInfo.MetallicRoughnessMapIdx < texIDs.size())
                material->SetTexture("u_MetallicRoughnessMap", texIDs[matInfo.MetallicRoughnessMapIdx]);
            
            // Set material properties
            material->SetFloat4("u_AlbedoColor", matInfo.AlbedoColor);
            material->SetFloat("u_Metallic", matInfo.Metallic);
            material->SetFloat("u_Roughness", matInfo.Roughness);
            
            matIDs.push_back(matID);
        }
        
        // Upload meshes
        for (const auto& meshInfo : modelData.Meshes)
        {
            UUID meshID = AssetsRegister::Register(meshInfo.DebugName);
            
            // Convert SubMeshCreateInfo to SubMesh
            std::vector<SubMesh> submeshes;
            for (const auto& subInfo : meshInfo.SubMeshes)
            {
                SubMesh sm;
                sm.NodeName = subInfo.NodeName;
                sm.VertexCount = subInfo.VertexCount;
                sm.IndexCount = subInfo.IndexCount;
                sm.BaseVertex = subInfo.BaseVertex;
                sm.BaseIndex = subInfo.BaseIndex;
                sm.BoundsMin = subInfo.BoundsMin;
                sm.BoundsMax = subInfo.BoundsMax;
                sm.LocalTransform = glm::mat4(1.0f);
                
                // Assign material
                if (subInfo.MaterialIdx >= 0 && subInfo.MaterialIdx < matIDs.size())
                    sm.MaterialID = matIDs[subInfo.MaterialIdx];
                
                submeshes.push_back(sm);
            }
            
            // Create mesh spec
            MeshSpec spec;
            spec.Streams = {
                {meshInfo.Positions.data(), meshInfo.totalVertices, {{"a_Position", ShaderDataType::Float3}}},
                {meshInfo.Normals.data(), meshInfo.totalVertices, {{"a_Normal", ShaderDataType::Float3}}},
                {meshInfo.Tangents.data(), meshInfo.totalVertices, {{"a_Tangent", ShaderDataType::Float4}}},
                {meshInfo.TexCoords.data(), meshInfo.totalVertices, {{"a_TexCoord", ShaderDataType::Float2}}}
            };
            spec.IndexData = meshInfo.Indices.data();
            spec.IndexCount = meshInfo.totalIndices;
            spec.Submeshes = submeshes;
            
            MeshLibrary::Load(spec, meshID);
            meshIDs.push_back(meshID);
        }
        
        AE_CORE_INFO("Uploaded model: {0} meshes, {1} materials, {2} textures", 
            meshIDs.size(), matIDs.size(), texIDs.size());
        
        return meshIDs;
    }
}