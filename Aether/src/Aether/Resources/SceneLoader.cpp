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

    static int32_t FindSkeleton(const cgltf_animation* anim, const std::vector<SkeletonCreateInfo>& skeletons)
    {
        for (size_t c = 0; c < anim->channels_count; c++)
        {
            const cgltf_node* targetNode = anim->channels[c].target_node;
            if (!targetNode || !targetNode->name) continue;

            std::string nodeName = targetNode->name;

            for (size_t skelIdx = 0; skelIdx < skeletons.size(); skelIdx++)
            {
                const auto& skel = skeletons[skelIdx];
                for (const auto& boneName : skel.boneNames)
                {
                    if (boneName == nodeName)
                    {
                        return (int32_t)skelIdx;
                    }
                }
            }
        }
        return -1; 
    }

    static bool ParseSkeleton(const cgltf_data* gltfData, 
                        const cgltf_skin* skin,
                        SkeletonCreateInfo& outSkeleton)
    {
        if (!skin || skin->joints_count == 0)
            return false;

        size_t jointCount = skin->joints_count;
        outSkeleton.DebugName = skin->name ? skin->name : "Unnamed_Skeleton";
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

            // Read the node's actual local transform
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
                                Clip& outClip)
    {
        if (!gltfAnim || gltfAnim->channels_count == 0)
            return false;

        float duration = 0.0f;
        std::vector<Channel> channels;
        
        // Map: boneIdx -> Channel
        std::map<int32_t, Channel*> channelMap;

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
            Channel* channel = nullptr;
            if (channelMap.find(boneIdx) == channelMap.end())
            {
                channels.push_back(Channel());
                channel = &channels.back();
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
                duration = std::max(duration, maxTime);
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
                    
                    // Find existing keyframe at this time
                    for (auto& existing : channel->keyframes)
                    {
                        if (fabs(existing.time - times[i]) < 0.0001f)
                        {
                            kf = &existing;
                            break;
                        }
                    }
                    
                    // Create new keyframe if not found
                    if (!kf)
                    {
                        channel->keyframes.push_back(KeyFrame());
                        kf = &channel->keyframes.back();
                        kf->time = times[i];
                        kf->translation = defTranslation;
                        kf->rotation = defRotation;
                        kf->scale = defScale;
                    }
                    
                    kf->translation = glm::vec3(
                        values[i * 3 + 0],
                        values[i * 3 + 1],
                        values[i * 3 + 2]
                    );
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
                        if (fabs(existing.time - times[i]) < 0.0001f)
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
                        kf->scale = defScale;
                    }
                    
                    // GLTF stores quat as (x, y, z, w)
                    kf->rotation = glm::quat(
                        values[i * 4 + 3],  // w
                        values[i * 4 + 0],  // x
                        values[i * 4 + 1],  // y
                        values[i * 4 + 2]   // z
                    );
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
                        if (fabs(existing.time - times[i]) < 0.0001f)
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
                        kf->scale = defScale;
                    }
                    
                    kf->scale = glm::vec3(
                        values[i * 3 + 0],
                        values[i * 3 + 1],
                        values[i * 3 + 2]
                    );
                }
            }
        }

        // Sort keyframes by time for each channel
        for (auto& channel : channels)
        {
            std::sort(channel.keyframes.begin(), channel.keyframes.end(),
                [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        }

        outClip = Clip(channels, duration);
        return true;
    }

    SceneLoadResult SceneLoader::Parsing(const std::string& path)
    {
        SceneLoadResult modelData;
        modelData.FilePath = path;

        cgltf_options options = {};
        cgltf_data* data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
        
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to parse GLTF file: {0}", path);
            return modelData;
        }
        
        result = cgltf_load_buffers(&options, data, path.c_str());
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to load GLTF buffers: {0}", path);
            cgltf_free(data);
            return modelData;
        }

        // Load textures
        for (size_t i = 0; i < data->images_count; i++)
        {
            const cgltf_image* image = &data->images[i];
            TextureCreateInfo texInfo;
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
                    texInfo.Spec.GenerateMips = true;
                    texInfo.Spec.WrapMode = true;
                    
                    size_t dataSize = width * height * 4;
                    texInfo.RawData.resize(dataSize);
                    memcpy(texInfo.RawData.data(), pixels, dataSize);
                    
                    stbi_image_free(pixels);
                    modelData.Textures.push_back(texInfo);
                }
            }
            else if (image->uri)
            {
                std::string imagePath = path.substr(0, path.find_last_of("/\\") + 1) + image->uri;
                
                int width, height, channels;
                stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
                
                if (pixels)
                {
                    texInfo.Spec.Width = width;
                    texInfo.Spec.Height = height;
                    texInfo.Spec.Format = ImageFormat::RGBA8;
                    texInfo.Spec.GenerateMips = true;
                    texInfo.Spec.WrapMode = true;
                    
                    size_t dataSize = width * height * 4;
                    texInfo.RawData.resize(dataSize);
                    memcpy(texInfo.RawData.data(), pixels, dataSize);
                    
                    stbi_image_free(pixels);
                    modelData.Textures.push_back(texInfo);
                }
            }
        }

        // Load materials
        for (size_t i = 0; i < data->materials_count; i++)
        {
            const cgltf_material* mat = &data->materials[i];
            MaterialCreateInfo matInfo;
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
                {
                    matInfo.AlbedoMapIdx = (int)(pbr.base_color_texture.texture->image - data->images);
                }
                
                if (pbr.metallic_roughness_texture.texture)
                {
                    matInfo.MetallicRoughnessMapIdx = (int)(pbr.metallic_roughness_texture.texture->image - data->images);
                }
            }
            
            if (mat->normal_texture.texture)
            {
                matInfo.NormalMapIdx = (int)(mat->normal_texture.texture->image - data->images);
            }
            
            modelData.Materials.push_back(matInfo);
        }

        // Load meshes
        for (size_t meshIdx = 0; meshIdx < data->meshes_count; meshIdx++)
        {
            const cgltf_mesh* mesh = &data->meshes[meshIdx];
            
            MeshCreateInfo meshInfo;
            meshInfo.DebugName = mesh->name ? mesh->name : ("Mesh_" + std::to_string(meshIdx));
            
            uint32_t totalVertices = 0;
            uint32_t totalIndices = 0;
            
            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];
                
                SubMeshCreateInfo subInfo;
                subInfo.NodeName = meshInfo.DebugName + "_Sub_" + std::to_string(primIdx);
                subInfo.BaseVertex = totalVertices;
                subInfo.BaseIndex = totalIndices;
                
                if (prim->material)
                {
                    subInfo.MaterialIdx = (int)(prim->material - data->materials);
                }

                std::vector<float> positions, normals, tangents, texCoords, weights;
                std::vector<uint32_t> joints;

                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    const cgltf_attribute* attr = &prim->attributes[attrIdx];
                    cgltf_accessor* accessor = attr->data;

                    if (attr->type == cgltf_attribute_type_position)
                    {
                        subInfo.VertexCount = (uint32_t)accessor->count;
                        positions = ReadAccessorFloat(accessor);
                        
                        if (accessor->has_min && accessor->has_max)
                        {
                            subInfo.BoundsMin = glm::vec3(
                                accessor->min[0], accessor->min[1], accessor->min[2]);
                            subInfo.BoundsMax = glm::vec3(
                                accessor->max[0], accessor->max[1], accessor->max[2]);
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_normal)
                    {
                        normals = ReadAccessorFloat(accessor);
                    }
                    else if (attr->type == cgltf_attribute_type_tangent)
                    {
                        tangents = ReadAccessorFloat(accessor);
                    }
                    else if (attr->type == cgltf_attribute_type_texcoord)
                    {
                        texCoords = ReadAccessorFloat(accessor);
                    }
                    else if (attr->type == cgltf_attribute_type_weights)
                    {
                        weights = ReadAccessorFloat(accessor);
                    }
                    else if (attr->type == cgltf_attribute_type_joints)
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
            
            meshInfo.totalVertices = totalVertices;
            meshInfo.totalIndices = totalIndices;
            
            AE_CORE_INFO("Parsed mesh with {0} vertices, {1} indices, {2} submeshes", 
                totalVertices, totalIndices, meshInfo.SubMeshes.size());
            modelData.Meshes.push_back(meshInfo);
        }

        // Parse skeletons
        for (size_t i = 0; i < data->skins_count; i++)
        {
            SkeletonCreateInfo skeleton;
            if (ParseSkeleton(data, &data->skins[i], skeleton))
            {
                modelData.Skeletons.push_back(skeleton);
            }
        }

        // Parse animations
        for (size_t i = 0; i < data->animations_count; i++)
        {
            if (modelData.Skeletons.empty())
                break;

            cgltf_animation* gltfAnim = &data->animations[i];
            int32_t skeletonIdx = FindSkeleton(gltfAnim, modelData.Skeletons);

            ClipCreateInfo clipInfo;
            clipInfo.DebugName = data->animations[i].name ? 
                data->animations[i].name : ("Animation_" + std::to_string(i));
            
            if (ParseAnimationClip(data, gltfAnim, modelData.Skeletons[skeletonIdx], clipInfo.clip))
            {
                modelData.Clips.push_back(clipInfo);
            }
        }

        cgltf_free(data);
        return modelData;
    }

    SceneResult SceneLoader::UploadScene(const SceneLoadResult& sceneData, UUID shaderID)
    {
        SceneResult res;
        
        // Upload textures
        for (const auto& texInfo : sceneData.Textures)
        {
            UUID texID = AssetsRegister::Register(texInfo.DebugName);
            auto tex = Texture2D::Create(texInfo.Spec);
            tex->SetData((void*)texInfo.RawData.data(), texInfo.RawData.size());
            Texture2DLibrary::Add(tex, texID);
            res.texIDs.push_back(texID);
        }
        
        // Upload materials
        for (const auto& matInfo : sceneData.Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName);
            auto material = Material::Create(shaderID);
            
            // Set textures
            if (matInfo.AlbedoMapIdx >= 0 && matInfo.AlbedoMapIdx < res.texIDs.size())
                material->SetTexture("u_AlbedoMap", res.texIDs[matInfo.AlbedoMapIdx]);
            
            if (matInfo.NormalMapIdx >= 0 && matInfo.NormalMapIdx < res.texIDs.size())
            {
                material->SetTexture("u_NormalMap", res.texIDs[matInfo.NormalMapIdx]);
                material->SetInt("u_HasNormalMap", 1);
            }
            else
            {
                material->SetInt("u_HasNormalMap", 0);
            }
            
            if (matInfo.MetallicRoughnessMapIdx >= 0 && matInfo.MetallicRoughnessMapIdx < res.texIDs.size())
                material->SetTexture("u_MetallicRoughnessMap", res.texIDs[matInfo.MetallicRoughnessMapIdx]);
            
            // Set material properties
            material->SetFloat4("u_AlbedoColor", matInfo.AlbedoColor);
            material->SetFloat("u_Metallic", matInfo.Metallic);
            material->SetFloat("u_Roughness", matInfo.Roughness);
            
            MaterialLibrary::Add(material, matID);
            res.matIDs.push_back(matID);
        }
        
        // Upload meshes
        for (const auto& meshInfo : sceneData.Meshes)
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
                if (subInfo.MaterialIdx >= 0 && subInfo.MaterialIdx < res.matIDs.size())
                    sm.MaterialID = res.matIDs[subInfo.MaterialIdx];
                
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
            
            auto mesh = Mesh::Create(spec);
            MeshLibrary::Add(mesh, meshID);
            res.meshIDs.push_back(meshID);
        }

        // Upload skeletons
        for(const auto& skeletonInfo : sceneData.Skeletons)
        {
            UUID skeletonID = AssetsRegister::Register(skeletonInfo.DebugName);
            
            // Create SkeletonSpec from SkeletonCreateInfo
            SkeletonSpec spec{
                skeletonInfo.parentIndices,
                skeletonInfo.inverseBindMatrices,
                skeletonInfo.localBindPose
            };
            
            auto animator = CreateRef<SkeletalAnimator>(spec);
            SkeletalAnimatorLibrary::Add(animator, skeletonID);
            res.skeletonIDs.push_back(skeletonID);
        }

        // Upload clips
        for (const auto& clipInfo : sceneData.Clips)
        {
            UUID clipID = AssetsRegister::Register(clipInfo.DebugName);
            auto clip = CreateRef<Clip>(clipInfo.clip);
            ClipLibrary::Add(clip, clipID);
            res.clipIDs.push_back(clipID);
        }

        
        AE_CORE_INFO("Uploaded model: {0} meshes, {1} materials, {2} textures, {3} skeletons, {4} clips",
            res.meshIDs.size(), res.matIDs.size(), res.texIDs.size(), res.skeletonIDs.size(), res.clipIDs.size());
        
        return res;
    }
}