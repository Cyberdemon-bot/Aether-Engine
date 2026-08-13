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
namespace Aether {

	LegacyAssembler::API LegacyAssembler::s_API = LegacyAssembler::API::Cgltf;

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

        std::vector<Handle<Asset>> imgs;
        std::vector<Handle<Asset>> skels;
        auto* asset_manager = ServiceManager::GetService<AssetManager>();
        // Upload textures
        for (const auto& img : sceneData->Images) 
        {
            auto handle = ResourceManager::CreateResource<Texture2D>(img.Spec);
            auto resource = ResourceManager::GetResource<Texture2D>(handle);
            if (!resource) AE_CORE_ERROR("Fail to Create texture while uploading");
            resource->SetData((void*)img.RawData.data(), img.RawData.size());
            auto imgHandle = asset_manager->CreateAsset<Image>(UUID(), handle);
            imgs.push_back(imgHandle);
        }
        // Upload materials
        for (const auto& matInfo : sceneData->Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName, matInfo.AssetID);
            asset_manager->CreateAsset<Material>(matID);
            auto material = asset_manager->GetAsset<Material>(matID);
            if (!material) AE_CORE_ERROR("Fail to Create material while uploading");
            
            // Set textures
            if (matInfo.AlbedoMapIdx >= 0 && matInfo.AlbedoMapIdx < imgs.size())
                material->AddImage("u_AlbedoMap", imgs[matInfo.AlbedoMapIdx]);
            
            if (matInfo.NormalMapIdx >= 0 && matInfo.NormalMapIdx < imgs.size())
            {
                material->AddImage("u_NormalMap", imgs[matInfo.NormalMapIdx]);
                material->AddInt("u_HasNormalMap", 1);
            }
            else  material->AddInt("u_HasNormalMap", 0);

            if (matInfo.MetallicRoughnessMapIdx >= 0 && matInfo.MetallicRoughnessMapIdx < imgs.size())
                material->AddImage("u_MetallicRoughnessMap", imgs[matInfo.MetallicRoughnessMapIdx]);
            
            // Set material properties
            material->AddVec4("u_AlbedoColor", matInfo.AlbedoColor);
            material->AddFloat("u_Metallic", matInfo.Metallic);
            material->AddFloat("u_Roughness", matInfo.Roughness);
            res.matIDs.push_back(matID);
        }
        auto animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (size_t i = 0; i < sceneData->Skeletons.size(); i++)
        {
            auto& skelInfo = sceneData->Skeletons[i];
            auto skeleton = asset_manager->CreateAsset<Skeleton>(AssetsRegister::Register(skelInfo.DebugName, skelInfo.AssetID), 
                                                                animSystem->CreateSkeleton(skelInfo.spec),
                                                                static_cast<uint32_t>(skelInfo.spec.Joints.size()));
            res.animators.push_back({});
            res.animators[i].skeleton = skeleton;
            skels.push_back(skeleton);
        }

        for (size_t i = 0; i < sceneData->Clips.size(); i++)
        {
            auto& clipInfo = sceneData->Clips[i];
            uint32_t targetRigIdx = clipInfo.rigIdx; 
            if (targetRigIdx >= 0 && targetRigIdx < res.animators.size())
            {
                auto skeleton = asset_manager->GetAsset<Skeleton>(res.animators[targetRigIdx].skeleton)->m_Handle;
                auto clip = asset_manager->CreateAsset<Clip>(AssetsRegister::Register(clipInfo.DebugName, clipInfo.AssetID), 
                                                            animSystem->CreateClip(clipInfo.spec, skeleton), 
                                                            clipInfo.spec.Duration);
                res.animators[targetRigIdx].clips.push_back(clip);
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

            UUID meshID = AssetsRegister::Register(meshInfo.DebugName, meshInfo.AssetID);
            std::vector<Handle<Asset>> matHandles;
            std::vector<SubMesh> submeshes;
            for (const auto& subInfo : meshInfo.SubMeshes)
            {
                SubMesh sm;
                sm.VertexCount = subInfo.VertexCount;
                sm.IndexCount = subInfo.IndexCount;
                sm.BaseVertex = subInfo.BaseVertex;
                sm.BaseIndex = subInfo.BaseIndex;
                sm.BoundsMin = subInfo.BoundsMin;
                sm.BoundsMax = subInfo.BoundsMax;

                // Assign material
                auto matHandle = asset_manager->GetHandle(res.matIDs[subInfo.MaterialIdx]);
                matHandles.push_back(matHandle);
                sm.MaterialIdx = (int)matHandles.size() - 1;
                submeshes.push_back(sm);
            }

            UUID sheetID = AssetsRegister::Register(meshInfo.DebugName + "_Sheet", UUID());
            auto sheetHandle = asset_manager->CreateAsset<Sheet>(sheetID);
            auto* sheet = asset_manager->GetAsset<Sheet>(sheetHandle);
            sheet->Resize((uint32_t)matHandles.size());
            sheet->MoveDefaultList(std::move(matHandles));
            res.sheetIDs.push_back(sheetID);
            
            // Create mesh spec
            std::vector<VertexStream> temp = {
                {meshInfo.Positions.data(), meshInfo.totalVertices, {{"a_Position", ShaderDataType::Float3}}},
                {meshInfo.Normals.data(), meshInfo.totalVertices, {{"a_Normal", ShaderDataType::Float3}}},
                {meshInfo.Tangents.data(), meshInfo.totalVertices, {{"a_Tangent", ShaderDataType::Float4}}},
                {meshInfo.TexCoords.data(), meshInfo.totalVertices, {{"a_TexCoord", ShaderDataType::Float2}}},
                {meshInfo.Joints.data(), meshInfo.totalVertices, {{"a_Joints", ShaderDataType::Uint4}}},
                {meshInfo.Weights.data(), meshInfo.totalVertices, {{"a_Weights", ShaderDataType::Float4}}}
            };
            MeshSpec spec;
            spec.StreamData = temp.data();
            spec.StreamCount = temp.size();
            spec.IndexData = meshInfo.Indices.data();
            spec.IndexCount = meshInfo.totalIndices;
            spec.Submeshes = submeshes;
            if (rigIdx >= 0)
            {
                auto asset = asset_manager->GetAsset<Skeleton>(skels[rigIdx]);
                auto handle = asset->m_Handle;
                spec.RigPoseMats.resize(asset->m_JointCount);
                animSystem->GetRestPoseMatrices(handle, spec.RigPoseMats.data(), spec.RigPoseMats.size());
            }

            asset_manager->CreateAsset<Mesh>(meshID, spec);
            res.meshIDs.push_back(meshID);
        }

        res.hierarchy = std::move(sceneData->Hierarchy);
        return res;
    }
}