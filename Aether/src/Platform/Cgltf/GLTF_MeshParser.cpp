#include "Platform/Cgltf/GLTF_MeshParser.h"
#include "Platform/Cgltf/GLTF_Utils.h"
#include "aepch.h"
#include <cgltf.h>

namespace Aether {
    Ref<ParsedMeshInfo> GLTF_MeshParser::Parsing(void* data) 
    {
        cgltf_data* gltf = static_cast<cgltf_data*>(data);
        auto result = CreateRef<ParsedMeshInfo>();
        result->meshesInfo.resize(gltf->meshes_count);
        for (size_t meshIdx = 0; meshIdx < gltf->meshes_count; meshIdx++)
        {
            const cgltf_mesh* mesh = &gltf->meshes[meshIdx];
            
            LMeshCreateInfo& meshInfo = result->meshesInfo[meshIdx];
            meshInfo.AssetID = UUID();
            meshInfo.DebugName = mesh->name ? mesh->name : ("Mesh_" + std::to_string(meshIdx));

            bool isSkinnedMesh = false;
            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];
                bool hasJoints = false, hasWeights = false;
                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_joints) hasJoints = true;
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_weights) hasWeights = true;
                }
                if (hasJoints && hasWeights)
                {
                    isSkinnedMesh = true;
                    break;
                }
            }
            meshInfo.IsSkinned = isSkinnedMesh;
            
            uint32_t totalVertices = 0;
            uint32_t totalIndices = 0;

            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];
                
                SubAMeshCreateInfo subInfo;
                subInfo.NodeName = meshInfo.DebugName + "_Sub_" + std::to_string(primIdx);
                subInfo.BaseVertex = totalVertices;
                subInfo.BaseIndex = totalIndices;
                
                if (prim->material) subInfo.MaterialIdx = (int)(prim->material - gltf->materials);

                std::vector<float> positions, normals, tangents, texCoords, weights;
                std::vector<uint32_t> joints, indices;
                uint32_t primVertexCount = 0;

                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    const cgltf_attribute* attr = &prim->attributes[attrIdx];
                    cgltf_accessor* accessor = attr->data;

                    if (attr->type == cgltf_attribute_type_position)
                    {
                        primVertexCount = (uint32_t)accessor->count;
                        subInfo.VertexCount = (uint32_t)accessor->count;
                        ReadAccessorFloat(accessor, positions);
                        
                        if (accessor->has_min && accessor->has_max)
                        {
                            subInfo.BoundsMin = glm::vec3(accessor->min[0], accessor->min[1], accessor->min[2]);
                            subInfo.BoundsMax = glm::vec3(accessor->max[0], accessor->max[1], accessor->max[2]);
                        }
                    }

                    else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0) ReadAccessorFloat(accessor, texCoords);
                    else if (attr->type == cgltf_attribute_type_normal) ReadAccessorFloat(accessor, normals);
                    else if (attr->type == cgltf_attribute_type_tangent) ReadAccessorFloat(accessor, tangents);
                    else if (attr->type == cgltf_attribute_type_weights) ReadAccessorFloat(accessor, weights);
                    else if (attr->type == cgltf_attribute_type_joints) ReadAccessorUint(accessor, joints);
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

                if (prim->indices)
                {
                    cgltf_accessor* accessor = prim->indices;
                    subInfo.IndexCount = (uint32_t)accessor->count;
                    indices.resize(accessor->count);
                    
                    for (size_t i = 0; i < accessor->count; i++)
                        indices[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
                }

                if (isSkinnedMesh)
                {
                    if (joints.empty()) joints.resize(primVertexCount * 4, 0);
                    if (weights.empty())
                    {
                        weights.resize(primVertexCount * 4, 0.0f);
                        for (size_t i = 0; i < primVertexCount; i++) weights[i * 4 + 0] = 1.0f;
                    }
                }

                if (isSkinnedMesh)
                {
                    size_t currOffset = meshInfo.InterleavedVertices.size();
                    meshInfo.InterleavedVertices.resize(currOffset + primVertexCount * sizeof(SkinnedVertex));
                    SkinnedVertex* dest = reinterpret_cast<SkinnedVertex*>(meshInfo.InterleavedVertices.data() + currOffset);

                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal   = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent  = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                        dest[i].Joints   = glm::uvec4(joints[i*4],   joints[i*4+1],   joints[i*4+2],   joints[i*4+3]);
                        dest[i].Weights  = glm::vec4(weights[i*4],  weights[i*4+1],  weights[i*4+2],  weights[i*4+3]);
                    }
                }
                else
                {
                    size_t currOffset = meshInfo.InterleavedVertices.size();
                    meshInfo.InterleavedVertices.resize(currOffset + primVertexCount * sizeof(Vertex));
                    Vertex* dest = reinterpret_cast<Vertex*>(meshInfo.InterleavedVertices.data() + currOffset);

                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal   = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent  = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                    }
                }
                
                meshInfo.Indices.insert(meshInfo.Indices.end(), indices.begin(), indices.end());
                totalVertices += subInfo.VertexCount;
                totalIndices += subInfo.IndexCount;
                meshInfo.SubMeshes.push_back(subInfo);
            }
            
            meshInfo.totalVertices = totalVertices;
            meshInfo.totalIndices = totalIndices;
            
            AE_CORE_INFO("Parsed mesh '{0}': {1} vertices, {2} indices, {3} submeshes", 
                meshInfo.DebugName, totalVertices, totalIndices, meshInfo.SubMeshes.size());
        }

        return result;
    }
}