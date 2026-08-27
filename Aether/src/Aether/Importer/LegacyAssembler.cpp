#include "aepch.h"
#include "Aether/Importer/LegacyAssembler.h"
#include "Platform/Cgltf/GLTF_Assembler.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Media.h"
#include "Aether/Assets/AssetRegister.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Core/Assert.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Renderer/VertexArray.h"
namespace Aether {

	LegacyAssembler::API LegacyAssembler::s_API = LegacyAssembler::API::Cgltf;

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

        glm::vec3 boundsMin = glm::vec3( FLT_MAX);
        glm::vec3 boundsMax = glm::vec3(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            const glm::vec3& pos = *reinterpret_cast<const glm::vec3*>(byteData + i * stride);
            boundsMin = glm::min(boundsMin, pos);
            boundsMax = glm::max(boundsMax, pos);
        }

        spec.boundsMin = boundsMin; spec.boundsMax = boundsMax;
    }

    static void CalculateSkinnedBoundsAOS(AMeshCreateInfo& spec, std::vector<glm::mat4>& poseMats)
    {
        if (spec.streams.empty() || spec.streams[0].VertexCount == 0 || poseMats.empty()) 
        {
            spec.hasAnimatedBounds = false;
            return;
        } 

        const SkinnedVertex* verts = static_cast<const SkinnedVertex*>(spec.streams[0].Data);
        uint32_t vertexCount = spec.streams[0].VertexCount;

        glm::vec3 boundsMin( FLT_MAX);
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
        spec.hasAnimatedBounds = true;
    }

	Scope<LegacyAssembler> LegacyAssembler::Create()
	{
        Scope<LegacyAssembler> assembler;
		switch (s_API)
		{
			case LegacyAssembler::API::Cgltf:
                assembler = CreateScope<GLTF_Assembler>();
                break;
            case LegacyAssembler::API::None:
                AE_CORE_ASSERT(false, "LegacyAssembler::None is currently not supported!");
                return nullptr;
		}

        assembler->m_MeshParser = MeshParser::Create();
        assembler->m_MaterialParser = MaterialParser::Create();
        assembler->m_AnimationParser = AnimationParser::Create();
        assembler->m_SceneParser = SceneGraphParser::Create();
		return assembler;
	}

    RegisteredScene LegacyAssembler::Upload(const Ref<ParsedScene>& sceneData)
    {
        RegisteredScene res;

        std::vector<UUID> imgs;
        auto* asset_manager = ServiceManager::GetService<AssetManager>();

        // Upload textures
        for (const auto& img : sceneData->Images) 
        {
            UUID id = ServiceManager::GetService<AssetRegister>()->Register<AImage>(AImageCreateInfo
                                                                                    {
                                                                                        UUID(),
                                                                                        img.DebugName,
                                                                                        img.Spec, 
                                                                                        std::span(img.RawData)
                                                                                    });
            imgs.push_back(id);
        }

        for (const auto& matInfo : sceneData->Materials)
        {
            UUID albedo = UUID(), normal = UUID(), metal = UUID();
            if (matInfo.AlbedoMapIdx >= 0) albedo = imgs[matInfo.AlbedoMapIdx];
            if (matInfo.NormalMapIdx >= 0) normal = imgs[matInfo.NormalMapIdx];
            if (matInfo.MetallicRoughnessMapIdx >= 0) metal = imgs[matInfo.MetallicRoughnessMapIdx];
            UUID matID = ServiceManager::GetService<AssetRegister>()->Register<AMaterial>(
                                                                        AMaterialCreateInfo
                                                                        {
                                                                            matInfo.AssetID,
                                                                            matInfo.DebugName, 
                                                                            matInfo.AlbedoColor, 
                                                                            matInfo.Metallic,
                                                                            matInfo.Roughness,
                                                                            albedo, normal, metal
                                                                        });
            res.matIDs.push_back(matID);
        }
        auto animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (auto& info : sceneData->Skeletons) 
        {
            UUID skelID = ServiceManager::GetService<AssetRegister>()->Register<ASkeleton>(
                                                                        ASkeletonCreateInfo
                                                                        {
                                                                            info.AssetID,
                                                                            info.DebugName, 
                                                                            std::move(info.spec)
                                                                        });
            res.animators.push_back({});
            res.animators.back().skeleton = skelID;
        }

        for (size_t i = 0; i < sceneData->Clips.size(); i++)
        {
            auto& info = sceneData->Clips[i];
            uint32_t targetRigIdx = info.rigIdx; 
            if (targetRigIdx >= 0 && targetRigIdx < res.animators.size())
            {
                auto clipID = ServiceManager::GetService<AssetRegister>()->Register<AClip>(
                                                                            AClipCreateInfo
                                                                            {
                                                                                info.AssetID,
                                                                                info.DebugName,
                                                                                std::move(info.spec),
                                                                                res.animators[targetRigIdx].skeleton
                                                                            });
                res.animators[targetRigIdx].clips.push_back(clipID);
            }
        }

        // Upload meshes
        for (size_t meshIdx = 0; meshIdx < sceneData->Meshes.size(); meshIdx++)
        {
            const auto& meshInfo = sceneData->Meshes[meshIdx];

            int rigIdx = -1;
            if (sceneData->Hierarchy)
            {
                for (const auto& node : sceneData->Hierarchy->nodes)
                {
                    if (node.meshIdx == (int)meshIdx && node.animatorIdx >= 0)
                    {
                        rigIdx = node.animatorIdx;
                        break;
                    }
                }
            }

            const size_t submeshCount = meshInfo.SubMeshes.size();

            std::vector<UUID> sheetMatIDs;
            sheetMatIDs.reserve(submeshCount);

            std::vector<SubMesh> submeshes;
            submeshes.reserve(submeshCount);

            for (const auto& subInfo : meshInfo.SubMeshes)
            {
                SubMesh sm;
                sm.VertexCount = subInfo.VertexCount;
                sm.IndexCount = subInfo.IndexCount;
                sm.BaseVertex = subInfo.BaseVertex;
                sm.BaseIndex = subInfo.BaseIndex;
                sm.BoundsMin = subInfo.BoundsMin;
                sm.BoundsMax = subInfo.BoundsMax;

                if (subInfo.MaterialIdx >= 0 && subInfo.MaterialIdx < static_cast<int>(res.matIDs.size()))
                {
                    sheetMatIDs.push_back(res.matIDs[subInfo.MaterialIdx]);
                    sm.MaterialIdx = static_cast<int>(sheetMatIDs.size()) - 1;
                }
                else sm.MaterialIdx = -1;

                submeshes.push_back(sm);
            }

            UUID sheetID = ServiceManager::GetService<AssetRegister>()->Register<ASheet>(
                                                                        ASheetCreateInfo 
                                                                        {
                                                                            UUID(),
                                                                            meshInfo.DebugName + "_Sheet",
                                                                            sheetMatIDs.data(),
                                                                            static_cast<uint32_t>(sheetMatIDs.size())
                                                                        });

            res.sheetIDs.push_back(sheetID);
            
            VertexStream interleavedStream;
            interleavedStream.Data = meshInfo.InterleavedVertices.data();
            interleavedStream.VertexCount = meshInfo.totalVertices;
            interleavedStream.Layout = meshInfo.IsSkinned ? MeshLayout::PBRSkinned() : MeshLayout::PBR();

            AMeshCreateInfo spec;
            spec.streams = std::span<VertexStream>(&interleavedStream, 1);
            spec.indicies = std::span(meshInfo.Indices);
            spec.submeshes = std::span(submeshes);
            spec.id = meshInfo.AssetID;
            spec.debugName = std::move(meshInfo.DebugName);
            CalculateStaticBoundsAOS(spec);
            if (rigIdx >= 0 && rigIdx < (int)res.animators.size() && meshInfo.IsSkinned)
            {
                auto asset = asset_manager->GetAsset<ASkeleton>(res.animators[rigIdx].skeleton);
                if (asset)
                {
                    auto handle = asset->m_Handle;
                    std::vector<glm::mat4> poseMats(asset->m_JointCount);
                    animSystem->GetRestPoseMatrices(handle, poseMats.data(), poseMats.size());
                    CalculateSkinnedBoundsAOS(spec, poseMats);
                }
            }

            ServiceManager::GetService<AssetRegister>()->Register<AMesh>(spec);
            res.meshIDs.push_back(meshInfo.AssetID);
        }

        res.hierarchy = std::move(sceneData->Hierarchy);
        return res;
    }
}