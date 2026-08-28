#include "aepch.h"
#include "GLTFConverter.h"
#include "GLTFUtils.h"
#include "Aether/Core/Assert.h"

#include <cgltf.h>

namespace Aether {

    static void CalculateStaticBoundsAOS(AMeshCreateInfo& spec)
    {
        if (spec.streams.empty() || spec.streams[0].VertexCount == 0)
        {
            spec.boundsMin = glm::vec3(0.0f);
            spec.boundsMax = glm::vec3(0.0f);
            return;
        }

        const uint8_t* byteData = static_cast<const uint8_t*>(spec.streams[0].Data);
        uint32_t stride = spec.streams[0].Layout.GetStride();
        uint32_t vertexCount = spec.streams[0].VertexCount;

        glm::vec3 boundsMin(FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            const glm::vec3& pos = *reinterpret_cast<const glm::vec3*>(byteData + i * stride);
            boundsMin = glm::min(boundsMin, pos);
            boundsMax = glm::max(boundsMax, pos);
        }

        spec.boundsMin = boundsMin;
        spec.boundsMax = boundsMax;
    }

    void GLTFConverter::ParseMeshes(cgltf_data* gltf, ParsedScene& scene)
    {
        scene.Meshes.reserve(gltf->meshes_count);
        scene.MeshVertexBytes.reserve(gltf->meshes_count);
        scene.MeshIndexData.reserve(gltf->meshes_count);
        scene.MeshStreamData.reserve(gltf->meshes_count);
        scene.MeshSubmeshData.reserve(gltf->meshes_count);

        for (size_t meshIdx = 0; meshIdx < gltf->meshes_count; meshIdx++)
        {
            const cgltf_mesh* mesh = &gltf->meshes[meshIdx];

            AMeshCreateInfo meshInfo;
            meshInfo.id = UUID();
            meshInfo.debugName = mesh->name ? mesh->name : ("Mesh_" + std::to_string(meshIdx));

            bool isSkinned = false;
            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];
                bool hasJoints = false, hasWeights = false;
                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_joints) hasJoints = true;
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_weights) hasWeights = true;
                }
                if (hasJoints && hasWeights) { isSkinned = true; break; }
            }

            std::vector<uint8_t> vertexBytes;
            std::vector<uint32_t> indices;
            std::vector<SubMesh> submeshes;

            uint32_t totalVertices = 0;
            uint32_t totalIndices = 0;

            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];

                SubMesh subInfo;
                subInfo.BaseVertex = totalVertices;
                subInfo.BaseIndex = totalIndices;

                if (prim->material)
                    subInfo.MaterialIdx = (int)(prim->material - gltf->materials);

                std::vector<float> positions, normals, tangents, texCoords, weights;
                std::vector<uint32_t> joints, primIndices;
                uint32_t primVertexCount = 0;

                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    const cgltf_attribute* attr = &prim->attributes[attrIdx];
                    cgltf_accessor* accessor = attr->data;

                    if (attr->type == cgltf_attribute_type_position)
                    {
                        primVertexCount = (uint32_t)accessor->count;
                        subInfo.VertexCount = primVertexCount;
                        GLTFUtils::ReadAccessorFloat(accessor, positions);

                        if (accessor->has_min && accessor->has_max)
                        {
                            subInfo.BoundsMin = glm::vec3(accessor->min[0], accessor->min[1], accessor->min[2]);
                            subInfo.BoundsMax = glm::vec3(accessor->max[0], accessor->max[1], accessor->max[2]);
                        }
                    }
                    else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0)
                        GLTFUtils::ReadAccessorFloat(accessor, texCoords);
                    else if (attr->type == cgltf_attribute_type_normal)
                        GLTFUtils::ReadAccessorFloat(accessor, normals);
                    else if (attr->type == cgltf_attribute_type_tangent)
                        GLTFUtils::ReadAccessorFloat(accessor, tangents);
                    else if (attr->type == cgltf_attribute_type_weights)
                        GLTFUtils::ReadAccessorFloat(accessor, weights);
                    else if (attr->type == cgltf_attribute_type_joints)
                        GLTFUtils::ReadAccessorUint(accessor, joints);
                }

                if (normals.empty() && !positions.empty())
                {
                    normals.resize(positions.size(), 0.0f);
                    for (size_t i = 0; i < positions.size() / 3; i++)
                        normals[i * 3 + 1] = 1.0f;
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
                    texCoords.resize((positions.size() / 3) * 2, 0.0f);

                if (prim->indices)
                {
                    cgltf_accessor* accessor = prim->indices;
                    subInfo.IndexCount = (uint32_t)accessor->count;
                    primIndices.resize(accessor->count);
                    for (size_t i = 0; i < accessor->count; i++)
                        primIndices[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
                }

                if (isSkinned)
                {
                    if (joints.empty()) joints.resize(primVertexCount * 4, 0);
                    if (weights.empty())
                    {
                        weights.resize(primVertexCount * 4, 0.0f);
                        for (size_t i = 0; i < primVertexCount; i++) weights[i * 4 + 0] = 1.0f;
                    }
                }

                size_t currOffset = vertexBytes.size();
                if (isSkinned)
                {
                    vertexBytes.resize(currOffset + primVertexCount * sizeof(SkinnedVertex));
                    SkinnedVertex* dest = reinterpret_cast<SkinnedVertex*>(vertexBytes.data() + currOffset);
                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal   = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent  = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                        dest[i].Joints   = glm::uvec4(joints[i*4],   joints[i*4+1],   joints[i*4+2],   joints[i*4+3]);
                        dest[i].Weights  = glm::vec4(weights[i*4],   weights[i*4+1],   weights[i*4+2],  weights[i*4+3]);
                    }
                }
                else
                {
                    vertexBytes.resize(currOffset + primVertexCount * sizeof(Vertex));
                    Vertex* dest = reinterpret_cast<Vertex*>(vertexBytes.data() + currOffset);
                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal   = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent  = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                    }
                }

                indices.insert(indices.end(), primIndices.begin(), primIndices.end());
                totalVertices += subInfo.VertexCount;
                totalIndices  += subInfo.IndexCount;
                submeshes.push_back(subInfo);
            }

            AE_CORE_INFO("GLTFConverter: parsed mesh '{0}': {1} vertices, {2} indices, {3} submeshes",
                meshInfo.debugName, totalVertices, totalIndices, submeshes.size());

            scene.MeshVertexBytes.push_back(std::move(vertexBytes));
            scene.MeshIndexData.push_back(std::move(indices));
            scene.MeshSubmeshData.push_back(std::move(submeshes));

            VertexStream stream;
            stream.Data = scene.MeshVertexBytes.back().data();
            stream.VertexCount = totalVertices;
            stream.Layout = isSkinned ? MeshLayout::PBRSkinned() : MeshLayout::PBR();
            scene.MeshStreamData.push_back(std::vector<VertexStream>{ stream }); 

            meshInfo.streams = std::span<const VertexStream>(scene.MeshStreamData.back());
            meshInfo.indicies = std::span<const uint32_t>(scene.MeshIndexData.back());
            meshInfo.submeshes = std::span<const SubMesh>(scene.MeshSubmeshData.back());

            CalculateStaticBoundsAOS(meshInfo);
            scene.Meshes.push_back(std::move(meshInfo));
        }
    }
}