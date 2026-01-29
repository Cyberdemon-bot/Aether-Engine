#include "aepch.h"
#include "Aether/Resources/SceneLoader.h"
#include "Aether/Core/AssetsRegister.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <stb_image.h>

namespace Aether {

// Vibe coding animation parser, i dont know
    // Find joint index in skin's joints array
    static int32_t FindJointIndex(const cgltf_skin* skin, const cgltf_node* joint)
    {
        for (size_t i = 0; i < skin->joints_count; i++)
        {
            if (skin->joints[i] == joint)
                return static_cast<int32_t>(i);
        }
        return -1;
    }

    // Read accessor data as float array
    static std::vector<float> ReadAccessorFloat(const cgltf_accessor* accessor)
    {
        std::vector<float> data;
        if (!accessor) return data;

        size_t floatCount = accessor->count * cgltf_num_components(accessor->type);
        data.resize(floatCount);
        
        cgltf_accessor_unpack_floats(accessor, data.data(), floatCount);
        return data;
    }

    static glm::mat4 GetNodeLocalTransform(const cgltf_node* node)
    {
        if (node->has_matrix)
        {
            glm::mat4 mat;
            memcpy(glm::value_ptr(mat), node->matrix, sizeof(float) * 16);
            return mat;
        }
        else
        {
            glm::mat4 translation = glm::mat4(1.0f);
            glm::mat4 rotation = glm::mat4(1.0f);
            glm::mat4 scale = glm::mat4(1.0f);
            
            if (node->has_translation)
            {
                translation = glm::translate(glm::mat4(1.0f), 
                    glm::vec3(node->translation[0], node->translation[1], node->translation[2]));
            }
            
            if (node->has_rotation)
            {
                glm::quat quat(node->rotation[3], node->rotation[0], 
                            node->rotation[1], node->rotation[2]);
                rotation = glm::toMat4(quat);
            }
            
            if (node->has_scale)
            {
                scale = glm::scale(glm::mat4(1.0f), 
                    glm::vec3(node->scale[0], node->scale[1], node->scale[2]));
            }
            
            return translation * rotation * scale;
        }
    }

    static bool ParseSkeleton(const cgltf_data* gltfData, 
                        const cgltf_skin* skin,
                        SkeletonCreateInfo& outSkeleton)
    {
        if (!skin || skin->joints_count == 0)
            return false;

        size_t jointCount = skin->joints_count;
        outSkeleton.parentIndices.resize(jointCount);
        outSkeleton.inverseBindMatrices.resize(jointCount);
        outSkeleton.boneNames.resize(jointCount);
        outSkeleton.localBindPose.resize(jointCount); 

        // Read inverse bind matrices
        if (skin->inverse_bind_matrices)
        {
            std::vector<float> matrices = ReadAccessorFloat(skin->inverse_bind_matrices);
            
            for (size_t i = 0; i < jointCount; i++)
            {
                glm::mat4 mat;
                memcpy(glm::value_ptr(mat), &matrices[i * 16], sizeof(float) * 16);
                outSkeleton.inverseBindMatrices[i] = mat;
            }
        }
        else
        {
            // No inverse bind matrices provided, use identity
            for (size_t i = 0; i < jointCount; i++)
            {
                outSkeleton.inverseBindMatrices[i] = glm::mat4(1.0f);
            }
        }

        // Build parent hierarchy AND read local transforms
        for (size_t i = 0; i < jointCount; i++)
        {
            const cgltf_node* joint = skin->joints[i];
            outSkeleton.boneNames[i] = joint->name ? joint->name : "Bone_" + std::to_string(i);

            // *** THIS IS THE FIX *** - Read the node's actual local transform
            outSkeleton.localBindPose[i] = GetNodeLocalTransform(joint);

            // Find parent
            int32_t parentIdx = -1;
            if (joint->parent)
            {
                parentIdx = FindJointIndex(skin, joint->parent);
            }
            
            outSkeleton.parentIndices[i] = parentIdx;
        }

        return true;
    }


    // Parse animation clip from cgltf animation
    static bool ParseAnimationClip(const cgltf_data* gltfData,
                                const cgltf_animation* gltfAnim,
                                const SkeletonCreateInfo& skeleton,
                                AnimationClip& outClip)
    {
        if (!gltfAnim || gltfAnim->channels_count == 0)
            return false;

        outClip.name = gltfAnim->name ? gltfAnim->name : "Animation";
        outClip.duration = 0.0f;
        
        // Map: boneIdx -> BoneChannel
        std::map<int32_t, BoneChannel*> channelMap;

        // Process each channel
        for (size_t chanIdx = 0; chanIdx < gltfAnim->channels_count; chanIdx++)
        {
            const cgltf_animation_channel& gltfChannel = gltfAnim->channels[chanIdx];
            const cgltf_animation_sampler& sampler = *gltfChannel.sampler;
            
            // Find bone index
            int32_t boneIdx = -1;
            for (size_t i = 0; i < skeleton.boneNames.size(); i++)
            {
                if (skeleton.boneNames[i] == gltfChannel.target_node->name)
                {
                    boneIdx = static_cast<int32_t>(i);
                    break;
                }
            }
            
            if (boneIdx == -1)
                continue;  // Joint not in skeleton

            // Get or create bone channel
            BoneChannel* channel = nullptr;
            if (channelMap.find(boneIdx) == channelMap.end())
            {
                outClip.channels.push_back(BoneChannel());
                channel = &outClip.channels.back();
                channel->boneIdx = boneIdx;
                channelMap[boneIdx] = channel;
            }
            else
            {
                channel = channelMap[boneIdx];
            }

            // Read time stamps
            std::vector<float> times = ReadAccessorFloat(sampler.input);
            std::vector<float> values = ReadAccessorFloat(sampler.output);

            // Update duration
            if (!times.empty())
            {
                float maxTime = times.back();
                outClip.duration = std::max(outClip.duration, maxTime);
            }

            glm::vec3 defTranslation(0.0f);
            glm::quat defRotation(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 defScale(1.0f);
            glm::vec3 skew;
            glm::vec4 perspective;

            glm::decompose(skeleton.localBindPose[boneIdx], defScale, defRotation, defTranslation, skew, perspective);
            // Create keyframes based on channel type
            if (gltfChannel.target_path == cgltf_animation_path_type_translation)
            {
                // Translation (vec3)
                for (size_t i = 0; i < times.size(); i++)
                {
                    KeyFrame* kf = nullptr;
                    for (auto& existing : channel->keyframes)
                    {
                        if (std::abs(existing.time - times[i]) < 0.0001f)
                        {
                            kf = &existing;
                            break;
                        }
                    }
                    
                    if (!kf)
                    {
                        channel->keyframes.push_back(KeyFrame());
                        kf = &channel->keyframes.back();
                        kf->time = times[i];
                        kf->rotation = defRotation; 
                        kf->scale = defScale;
                    }
                    
                    kf->translation = glm::vec3(values[i * 3 + 0], 
                                            values[i * 3 + 1], 
                                            values[i * 3 + 2]);
                }
            }
            else if (gltfChannel.target_path == cgltf_animation_path_type_rotation)
            {
                // Rotation (quat: x, y, z, w)
                for (size_t i = 0; i < times.size(); i++)
                {
                    KeyFrame* kf = nullptr;
                    for (auto& existing : channel->keyframes)
                    {
                        if (std::abs(existing.time - times[i]) < 0.0001f)
                        {
                            kf = &existing;
                            break;
                        }
                    }
                    
                    if (!kf)
                    {
                        channel->keyframes.push_back(KeyFrame());
                        kf = &channel->keyframes.back();
                        kf->time = times[i];
                        kf->translation = defTranslation; 
                        kf->scale = defScale;
                    }
                    
                    // glTF quaternion order: x, y, z, w
                    // glm::quat constructor: w, x, y, z
                    kf->rotation = glm::quat(values[i * 4 + 3],  // w
                                            values[i * 4 + 0],   // x
                                            values[i * 4 + 1],   // y
                                            values[i * 4 + 2]);  // z
                }
            }
            else if (gltfChannel.target_path == cgltf_animation_path_type_scale)
            {
                // Scale (vec3)
                for (size_t i = 0; i < times.size(); i++)
                {
                    KeyFrame* kf = nullptr;
                    for (auto& existing : channel->keyframes)
                    {
                        if (std::abs(existing.time - times[i]) < 0.0001f)
                        {
                            kf = &existing;
                            break;
                        }
                    }
                    
                    if (!kf)
                    {
                        channel->keyframes.push_back(KeyFrame());
                        kf = &channel->keyframes.back();
                        kf->time = times[i];
                        kf->translation = defTranslation;
                        kf->rotation = defRotation;
                    }
                    
                    kf->scale = glm::vec3(values[i * 3 + 0], 
                                        values[i * 3 + 1], 
                                        values[i * 3 + 2]);
                }
            }
        }

        for (auto& channel : outClip.channels)
        {
            std::sort(channel.keyframes.begin(), channel.keyframes.end(),
                    [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        }

        return true;
    }
// end of animation parser

    SceneLoadResult SceneLoader::Parsing(const std::string& filepath)
    {
        SceneLoadResult modelData = {.FilePath = filepath};
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

        //extract textures
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

        //extract materials
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

        //extract mesh
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
                std::vector<float> weights;
                std::vector<uint32_t> joints;

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
                    else if (attr->type == cgltf_attribute_type_weights && attr->index == 0)
                    {
                        weights.resize(accessor->count * 4);
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            float w[4] = {1.0, 0.0, 0.0, 0.0};
                            cgltf_accessor_read_float(accessor, v, w, 4);
                            float sum = w[0] + w[1] + w[2] + w[3];
                            if (sum > 0.0f) // normalize
                            {
                                w[0] /= sum; w[1] /= sum;
                                w[2] /= sum; w[3] /= sum;
                            }
                            weights[v * 4 + 0] = w[0];
                            weights[v * 4 + 1] = w[1];
                            weights[v * 4 + 2] = w[2];
                            weights[v * 4 + 3] = w[3];
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_joints && attr->index == 0)
                    {
                        joints.resize(accessor->count * 4);
                        for (size_t v = 0; v < accessor->count; v++)
                        {
                            cgltf_uint j[4] = {0,0,0,0};
                            cgltf_accessor_read_uint(accessor, v, j, 4);
                            joints[v * 4 + 0] = j[0];
                            joints[v * 4 + 1] = j[1];
                            joints[v * 4 + 2] = j[2];
                            joints[v * 4 + 3] = j[3];
                        }
                    }
                }

                if (normals.empty() && !positions.empty())
                {
                    normals.resize(positions.size(), 0.0f);
                    for (size_t i = 0; i < positions.size() / 3; i++)
                    {
                        normals[i * 3 + 1] = 1.0f; 
                    }
                }

                if (tangents.empty() && !positions.empty())
                {
                    tangents.resize((positions.size() / 3) * 4, 0.0f);
                    for (size_t i = 0; i < positions.size() / 3; i++)
                    {
                        tangents[i * 4 + 0] = 1.0f; 
                        tangents[i * 4 + 3] = 1.0f; 
                    }
                }

                if (texCoords.empty() && !positions.empty())
                {
                    texCoords.resize((positions.size() / 3) * 2, 0.0f);
                }

                // extract indices
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
                meshInfo.Joints.insert(meshInfo.Joints.end(), joints.begin(), joints.end());
                meshInfo.Weights.insert(meshInfo.Weights.end(), weights.begin(), weights.end());
                
                totalVertices += subInfo.VertexCount;
                totalIndices += subInfo.IndexCount;
                meshInfo.SubMeshes.push_back(subInfo);
            }
            AE_CORE_INFO("Parsed mesh with {0} vertices, {1} indices, {2} submeshes", 
                totalVertices, totalIndices, meshInfo.SubMeshes.size());
            modelData.Meshes.push_back(meshInfo);
        }

        for (size_t i = 0; i < data->skins_count; i++)
        {
            SkeletonCreateInfo skeleton;
            if (ParseSkeleton(data, &data->skins[i], skeleton))
            {
                skeleton.meshIndex = static_cast<int32_t>(i); 
                modelData.Skeletons.push_back(skeleton);
            }
        }

        for (size_t i = 0; i < data->animations_count; i++)
        {
            if (modelData.Skeletons.empty())
                break;

            AnimationCreateInfo animInfo;
            if (ParseAnimationClip(data, &data->animations[i], modelData.Skeletons[0], animInfo.clip))
            {
                animInfo.skeletonIndex = 0; 
                modelData.Animations.push_back(animInfo);
            }
        }

        cgltf_free(data);
        return modelData;
    }

    std::vector<UUID> SceneLoader::UploadModel(const SceneLoadResult& modelData, UUID shaderID)
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
                {meshInfo.TexCoords.data(), meshInfo.totalVertices, {{"a_TexCoord", ShaderDataType::Float2}}},
                {meshInfo.Joints.data(), meshInfo.totalVertices, {{"a_Joints", ShaderDataType::Uint4}}},
                {meshInfo.Weights.data(), meshInfo.totalVertices, {{"a_Weights", ShaderDataType::Float4}}}
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