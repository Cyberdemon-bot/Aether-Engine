#include "aepch.h"
#include "Aether/Importer/LegacyAssembler.h"
#include "Platform/Cgltf/GLTF_Assembler.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Media.h"
#include "Aether/Assets/AssetsRegister.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Core/Assert.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Renderer/VertexArray.h"
namespace Aether {

	LegacyAssembler::API LegacyAssembler::s_API = LegacyAssembler::API::Cgltf;

    static std::tuple<glm::vec3, glm::vec3> CalculateStaticBoundsAOS(const AMeshCreateInfo& spec)
    {
        if (!spec.streams || spec.streams[0].VertexCount == 0) 
            return { glm::vec3(0.0f), glm::vec3(0.0f) };

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

        return { boundsMin, boundsMax };
    }

    static std::tuple<glm::vec3, glm::vec3> CalculateSkinnedBoundsAOS(const AMeshCreateInfo& spec)
    {
        if (!spec.streams || spec.streams[0].VertexCount == 0 || spec.poseMats.empty()) 
            return { glm::vec3(0.0f), glm::vec3(0.0f) };

        const SkinnedVertex* verts = static_cast<const SkinnedVertex*>(spec.streams[0].Data);
        uint32_t vertexCount = spec.streams[0].VertexCount;

        glm::vec3 boundsMin( FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            const auto& v = verts[i];
            glm::vec4 skinnedPos =
                spec.poseMats[v.Joints.x] * glm::vec4(v.Position, 1.0f) * v.Weights.x +
                spec.poseMats[v.Joints.y] * glm::vec4(v.Position, 1.0f) * v.Weights.y +
                spec.poseMats[v.Joints.z] * glm::vec4(v.Position, 1.0f) * v.Weights.z +
                spec.poseMats[v.Joints.w] * glm::vec4(v.Position, 1.0f) * v.Weights.w;

            boundsMin = glm::min(boundsMin, glm::vec3(skinnedPos));
            boundsMax = glm::max(boundsMax, glm::vec3(skinnedPos));
        }

        return { boundsMin, boundsMax };
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
        std::vector<UUID> skels;
        auto* asset_manager = ServiceManager::GetService<AssetManager>();

        // Upload textures
        for (const auto& img : sceneData->Images) 
        {
            UUID id = ServiceManager::GetService<AssetsRegister>()->Register<AImage>(AImageCreateInfo{img.Spec, img.RawData}, img.DebugName, UUID());
            imgs.push_back(id);
        }

        for (const auto& matInfo : sceneData->Materials)
        {
            UUID matID = ServiceManager::GetService<AssetsRegister>()->Register<AMaterial>(
                                                                        AMaterialCreateInfo
                                                                        {
                                                                            matInfo.AlbedoColor, 
                                                                            matInfo.Metallic,
                                                                            matInfo.Roughness,
                                                                            matInfo.AlbedoMapIdx,
                                                                            matInfo.NormalMapIdx,
                                                                            matInfo.MetallicRoughnessMapIdx,
                                                                            imgs.data(), static_cast<uint32_t>(imgs.size())
                                                                        },
                                                                        matInfo.DebugName, matInfo.AssetID);
            res.matIDs.push_back(matID);
        }
        auto animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (auto& info : sceneData->Skeletons)
        {
            UUID skelID = ServiceManager::GetService<AssetsRegister>()->Register<ASkeleton>(info.spec, info.DebugName, info.AssetID);
            res.animators.push_back({});
            res.animators.back().skeleton = asset_manager->GetHandle(skelID);
            skels.push_back(skelID);
        }

        for (size_t i = 0; i < sceneData->Clips.size(); i++)
        {
            auto& info = sceneData->Clips[i];
            uint32_t targetRigIdx = info.rigIdx; 
            if (targetRigIdx >= 0 && targetRigIdx < res.animators.size())
            {
                auto clipID = ServiceManager::GetService<AssetsRegister>()->Register<AClip>(
                                                                            AClipCreateInfo
                                                                            {
                                                                                std::move(info.spec),
                                                                                skels[targetRigIdx]
                                                                            },
                                                                            info.DebugName, info.AssetID);
                res.animators[targetRigIdx].clips.push_back(asset_manager->GetHandle(clipID));
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

            UUID sheetID = ServiceManager::GetService<AssetsRegister>()->Register<ASheet>(
                                                                        ASheetCreateInfo 
                                                                        {
                                                                            sheetMatIDs.data(),
                                                                            static_cast<uint32_t>(sheetMatIDs.size())
                                                                        },
                                                                        meshInfo.DebugName + "_Sheet", UUID());

            res.sheetIDs.push_back(sheetID);
            
            VertexStream interleavedStream;
            interleavedStream.Data = meshInfo.InterleavedVertices.data();
            interleavedStream.VertexCount = meshInfo.totalVertices;
            interleavedStream.Layout = meshInfo.IsSkinned ? MeshLayout::PBRSkinned() : MeshLayout::PBR();

            AMeshCreateInfo spec;
            spec.streams = &interleavedStream;
            spec.streamLen = 1;
            spec.indicies = meshInfo.Indices.data();
            spec.indexLen = meshInfo.totalIndices;
            spec.submeshes = submeshes;
            spec.CalculateBoundsFunc = &CalculateStaticBoundsAOS;
            if (rigIdx >= 0 && rigIdx < (int)skels.size() && meshInfo.IsSkinned)
            {
                auto asset = asset_manager->GetAsset<ASkeleton>(skels[rigIdx]);
                if (asset)
                {
                    auto handle = asset->m_Handle;
                    spec.poseMats.resize(asset->m_JointCount);
                    animSystem->GetRestPoseMatrices(handle, spec.poseMats.data(), spec.poseMats.size());
                    spec.CalculateAnimatedBoundsFunc = &CalculateSkinnedBoundsAOS;
                }
            }

            ServiceManager::GetService<AssetsRegister>()->Register<AMesh>(spec, meshInfo.DebugName, meshInfo.AssetID);
            res.meshIDs.push_back(meshInfo.AssetID);
        }

        res.hierarchy = std::move(sceneData->Hierarchy);
        return res;
    }
}