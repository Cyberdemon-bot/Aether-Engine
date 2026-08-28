#include "aepch.h"
#include "Aether/Core/Log.h"
#include "GLTFConverter.h"
#include "GLTFUtils.h"
#include <cgltf.h>

namespace Aether {

    static void CalcRestPoseMatrices(const SkeletonCreateInfo& data, glm::mat4* arr, size_t size)
    {
        if (data.Joints.size() > size) return;
        for (size_t i = 0; i < data.Joints.size(); i++)
        {
            const auto& joint = data.Joints[i];
            glm::mat4 T = glm::translate(glm::mat4(1.0f), joint.Translation);
            glm::mat4 R = glm::mat4_cast(joint.Rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), joint.Scale);
            glm::mat4 local = T * R * S;
            int parent = joint.ParentIndex;

            if (parent == -1) arr[i] = local;
            else arr[i] = arr[parent] * local;
        }
    }

    static void CalculateStaticBounds(AMeshCreateInfo& spec)
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

    static void CalculateSkinnedBounds(AMeshCreateInfo& spec, std::vector<glm::mat4>& poseMats)
    {
        if (spec.streams.empty() || spec.streams[0].VertexCount == 0 || poseMats.empty()) return;

        const SkinnedVertex* verts = static_cast<const SkinnedVertex*>(spec.streams[0].Data);
        uint32_t vertexCount = spec.streams[0].VertexCount;

        glm::vec3 boundsMin(FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            const auto& v = verts[i];
            glm::vec4 skinnedPos =
                poseMats[v.Joints.x] * glm::vec4(v.Position, 1.0f) * v.Weights.x +
                poseMats[v.Joints.y] * glm::vec4(v.Position, 1.0f) * v.Weights.y +
                poseMats[v.Joints.z] * glm::vec4(v.Position, 1.0f) * v.Weights.z +
                poseMats[v.Joints.w] * glm::vec4(v.Position, 1.0f) * v.Weights.w;

            boundsMin = glm::min(boundsMin, glm::vec3(skinnedPos));
            boundsMax = glm::max(boundsMax, glm::vec3(skinnedPos));
        }

        spec.animatedBoundsMin = boundsMin;
        spec.animatedBoundsMax = boundsMax;
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

            meshInfo.hasJointData = false;
            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];
                bool hasJoints = false, hasWeights = false;
                for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
                {
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_joints) hasJoints = true;
                    if (prim->attributes[attrIdx].type == cgltf_attribute_type_weights) hasWeights = true;
                }
                if (hasJoints && hasWeights) { meshInfo.hasJointData = true; break; }
            }

            std::vector<uint8_t> vertexBytes;
            std::vector<uint32_t> indices;
            std::vector<Submesh> Submeshes;
            std::vector<UUID> materials;

            uint32_t totalVertices = 0;
            uint32_t totalIndices = 0;

            for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
            {
                const cgltf_primitive* prim = &mesh->primitives[primIdx];

                Submesh subInfo;
                subInfo.BaseVertex = totalVertices;
                subInfo.BaseIndex = totalIndices;

                if (prim->material)
                {
                    uint32_t matIdx = (uint32_t)(prim->material - gltf->materials); 
                    if (matIdx < (uint32_t)scene.Materials.size())
                    {
                        subInfo.MaterialID = scene.Materials[matIdx].id;
                        materials.push_back(subInfo.MaterialID);
                    }
                }

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

                if (meshInfo.hasJointData)
                {
                    if (joints.empty()) joints.resize(primVertexCount * 4, 0);
                    if (weights.empty())
                    {
                        weights.resize(primVertexCount * 4, 0.0f);
                        for (size_t i = 0; i < primVertexCount; i++) weights[i * 4 + 0] = 1.0f;
                    }
                    if (joints.empty() || weights.empty())  
                        AE_CORE_WARN("[GLTFConverter] Mesh {0} should have joint data but nothing are found!", (uint64_t)meshInfo.id);
                }

                size_t currOffset = vertexBytes.size();
                if (meshInfo.hasJointData)
                {
                    vertexBytes.resize(currOffset + primVertexCount * sizeof(SkinnedVertex));
                    SkinnedVertex* dest = reinterpret_cast<SkinnedVertex*>(vertexBytes.data() + currOffset);
                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                        dest[i].Joints = glm::uvec4(joints[i*4],   joints[i*4+1],   joints[i*4+2],   joints[i*4+3]);
                        dest[i].Weights = glm::vec4(weights[i*4],   weights[i*4+1],   weights[i*4+2],  weights[i*4+3]);
                    }
                }
                else
                {
                    vertexBytes.resize(currOffset + primVertexCount * sizeof(Vertex));
                    Vertex* dest = reinterpret_cast<Vertex*>(vertexBytes.data() + currOffset);
                    for (size_t i = 0; i < primVertexCount; i++)
                    {
                        dest[i].Position = glm::vec3(positions[i*3], positions[i*3+1], positions[i*3+2]);
                        dest[i].Normal = glm::vec3(normals[i*3],   normals[i*3+1],   normals[i*3+2]);
                        dest[i].Tangent = glm::vec4(tangents[i*4],  tangents[i*4+1],  tangents[i*4+2], tangents[i*4+3]);
                        dest[i].TexCoord = glm::vec2(texCoords[i*2], texCoords[i*2+1]);
                    }
                }

                indices.insert(indices.end(), primIndices.begin(), primIndices.end());
                totalVertices += subInfo.VertexCount;
                totalIndices += subInfo.IndexCount;
                Submeshes.push_back(subInfo);
            }

            AE_CORE_INFO("GLTFConverter: parsed mesh '{0}': {1} vertices, {2} indices, {3} Submeshes",
                meshInfo.debugName, totalVertices, totalIndices, Submeshes.size());

            scene.MeshVertexBytes.push_back(std::move(vertexBytes));
            scene.MeshIndexData.push_back(std::move(indices));
            scene.MeshSubmeshData.push_back(std::move(Submeshes));
            scene.SheetData.push_back(std::move(materials));

            ASheetCreateInfo sheetInfo;
            sheetInfo.materialList = std::span(scene.SheetData.back());
            sheetInfo.debugName = meshInfo.debugName + "_Sheet";
            sheetInfo.id = UUID();

            VertexStream stream;
            stream.Data = scene.MeshVertexBytes.back().data();
            stream.VertexCount = totalVertices;
            stream.Layout = meshInfo.hasJointData ? MeshLayout::PBRSkinned() : MeshLayout::PBR();
            scene.MeshStreamData.push_back(std::vector<VertexStream>{ stream }); 

            meshInfo.streams = std::span<const VertexStream>(scene.MeshStreamData.back());
            meshInfo.indicies = std::span<const uint32_t>(scene.MeshIndexData.back());
            meshInfo.Submeshes = std::span<const Submesh>(scene.MeshSubmeshData.back());

            CalculateStaticBounds(meshInfo);
            if (meshInfo.hasJointData)
            {
                int rigIdx = -1;
                if (scene.Hierarchy)
                {
                    for (const auto& node : scene.Hierarchy->nodes)
                    {
                        if (node.meshIdx == (int)meshIdx && node.animatorIdx >= 0)
                        {
                            rigIdx = node.animatorIdx;
                            break;
                        }
                    }
                }

    
                if (rigIdx >= 0 && rigIdx < (int)scene.Skeletons.size())
                {
                    const auto& skel = scene.Skeletons[rigIdx];
                    std::vector<glm::mat4> poseMats(skel.data.Joints.size());
                    CalcRestPoseMatrices(skel.data, poseMats.data(), poseMats.size());
                    CalculateSkinnedBounds(meshInfo, poseMats);
                }
            }

            scene.Meshes.push_back(std::move(meshInfo));
            scene.Sheets.push_back(std::move(sheetInfo));
        }
    }
}