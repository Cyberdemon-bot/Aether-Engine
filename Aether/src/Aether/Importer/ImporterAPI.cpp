#include "aepch.h"
#include "Aether/Importer/ImporterAPI.h"
#include "Platform/Cgltf/GLTF_ImporterAPI.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/AssetsRegister.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Core/Assert.h"
#include "Aether/Core/ServiceManager.h"
namespace Aether {

	ImporterAPI::API ImporterAPI::s_API = ImporterAPI::API::Cgltf;
    Ref<MeshParser> ImporterAPI::m_MeshParser;
    Ref<MaterialParser> ImporterAPI::m_MaterialParser;
    Ref<AnimationParser> ImporterAPI::m_AnimationParser;
    Ref<SceneGraphParser> ImporterAPI::m_SceneParser;

	Scope<ImporterAPI> ImporterAPI::Create()
	{
        m_MeshParser = MeshParser::Create();
        m_MaterialParser = MaterialParser::Create();
        m_AnimationParser = AnimationParser::Create();
        m_SceneParser = SceneGraphParser::Create();
        
		switch (s_API)
		{
			case ImporterAPI::API::None: AE_CORE_ASSERT(false, "ImporterAPI::None is currently not supported!"); return nullptr;
			case ImporterAPI::API::Cgltf: return CreateScope<GLTF_ImporterAPI>();
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

    RegisteredScene ImporterAPI::Upload(const Ref<ParsedScene>& sceneData)
    {
        RegisteredScene res;
        std::vector<Handle<Resource>> texHandle;
        std::vector<UUID> rigIDs;     
        std::vector<UUID> clipIDs; 
        auto* asset_manager = ServiceManager::GetService<AssetManager>();
        // Upload textures
        for (const auto& texInfo : sceneData->Textures) 
        {
            auto handle = ResourceManager::CreateResource<Texture2D>(texInfo.Spec);
            auto resource = ResourceManager::GetResource<Texture2D>(handle);
            if (!resource) AE_CORE_ERROR("Fail to Create texture while uploading");
            resource->SetData((void*)texInfo.RawData.data(), texInfo.RawData.size());
            texHandle.push_back(handle);
        }
        // Upload materials
        for (const auto& matInfo : sceneData->Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName, matInfo.AssetID);
            asset_manager->CreateAsset<Material>(matID);
            auto material = asset_manager->GetAsset<Material>(matID);
            if (!material) AE_CORE_ERROR("Fail to Create material while uploading");
            
            // Set textures
            if (matInfo.AlbedoMapIdx >= 0 && matInfo.AlbedoMapIdx < texHandle.size())
                material->AddTexture("u_AlbedoMap", texHandle[matInfo.AlbedoMapIdx]);
            
            if (matInfo.NormalMapIdx >= 0 && matInfo.NormalMapIdx < texHandle.size())
            {
                material->AddTexture("u_NormalMap", texHandle[matInfo.NormalMapIdx]);
                material->AddInt("u_HasNormalMap", 1);
            }
            else  material->AddInt("u_HasNormalMap", 0);

            if (matInfo.MetallicRoughnessMapIdx >= 0 && matInfo.MetallicRoughnessMapIdx < texHandle.size())
                material->AddTexture("u_MetallicRoughnessMap", texHandle[matInfo.MetallicRoughnessMapIdx]);
            
            // Set material properties
            material->AddVec4("u_AlbedoColor", matInfo.AlbedoColor);
            material->AddFloat("u_Metallic", matInfo.Metallic);
            material->AddFloat("u_Roughness", matInfo.Roughness);
            res.matIDs.push_back(matID);
        }
        auto animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (size_t rigIdx = 0; rigIdx < sceneData->Rigs.size(); rigIdx++)
        {
            auto& rigInfo = sceneData->Rigs[rigIdx];
            UUID rigID = AssetsRegister::Register(rigInfo.DebugName, rigInfo.AssetID);
            rigIDs.push_back(rigID);
            res.animators.push_back({});
            auto rig = asset_manager->CreateAsset<Skeleton>(rigID, std::move(rigInfo.spec));
            res.animators[rigIdx].skeleton = rig;
        }

        for (size_t clipIdx = 0; clipIdx < sceneData->Clips.size(); clipIdx++)
        {
            auto& clipInfo = sceneData->Clips[clipIdx];
            UUID clipID = AssetsRegister::Register(clipInfo.DebugName, clipInfo.AssetID); clipIDs.push_back(clipID);
            
            uint32_t targetRigIdx = clipInfo.rigIdx; 
            if (targetRigIdx >= 0 && targetRigIdx < res.animators.size())
            {
                auto skeletonHandle = asset_manager->GetAsset<Skeleton>(res.animators[targetRigIdx].skeleton)->GetHandle();
                auto clip = asset_manager->CreateAsset<Clip>(clipID, std::move(clipInfo.spec), skeletonHandle);
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
            res.meshMap.emplace_back();
            // Convert SubMeshCreateInfo to SubMesh
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
                auto mat = res.matIDs[subInfo.MaterialIdx];
                res.meshMap.back().push_back(mat); 
                sm.MaterialIdx = res.meshMap.back().size() - 1;
                submeshes.push_back(sm);
            }
            
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
                auto handle = asset_manager->GetAsset<Skeleton>(rigIDs[rigIdx])->GetHandle();
                spec.RigPoseMats.resize(animSystem->GetJointCount(handle));
                animSystem->GetRestPoseMatrices(handle, spec.RigPoseMats.data(), spec.RigPoseMats.size());
            }

            asset_manager->CreateAsset<Mesh>(meshID, spec);
            res.meshIDs.push_back(meshID);
        }

        res.hierarchy = std::move(sceneData->Hierarchy);
        return res;
    }
}