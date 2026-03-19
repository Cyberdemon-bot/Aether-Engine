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
			case ImporterAPI::API::None:    AE_CORE_ASSERT(false, "ImporterAPI::None is currently not supported!"); return nullptr;
			case ImporterAPI::API::Cgltf:  return CreateScope<GLTF_ImporterAPI>();
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

    RegisteredScene ImporterAPI::Upload(const Ref<ParsedScene>& sceneData)
    {
        RegisteredScene res;
        std::vector<ResourceHandle> texHandle;
        std::vector<UUID> rigIDs;     
        std::vector<UUID> clipIDs; 
        // Upload textures
        for (const auto& texInfo : sceneData->Textures)
        {
            UUID texID = AssetsRegister::Register(texInfo.DebugName);
            auto handle = ResourceManager::CreateResource<Texture2D>(texInfo.Spec);
            auto resource = ResourceManager::GetResource<Texture2D>(handle);
            if (!resource) AE_CORE_ERROR("Fail to Create texture while uploading");
            resource->SetData((void*)texInfo.RawData.data(), texInfo.RawData.size());
            texHandle.push_back(handle);
        }
        // Upload materials
        for (const auto& matInfo : sceneData->Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName);
            AssetManager::CreateAsset<Material>(matID);
            auto material = AssetManager::GetAsset<Material>(AssetManager::GetHandle(matID));
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
        auto animSystem = AnimationSystem::GetModule<RigModule>();
        for (const auto& rigInfo : sceneData->Rigs)
        {
            UUID rigID = AssetsRegister::Register(rigInfo.DebugName);
            animSystem->RegisterSkeleton(rigInfo, rigID);
            rigIDs.push_back(rigID);
        }

        for (const auto& clipInfo : sceneData->Clips)
        {
            UUID clipID = AssetsRegister::Register(clipInfo.DebugName);
            clipIDs.push_back(clipID);
        }

        for (size_t rigIdx = 0; rigIdx < rigIDs.size(); rigIdx++)
        {
            UUID rigID = rigIDs[rigIdx];
            UUID animatorID = AssetsRegister::Register("RigAnimator_" + AssetsRegister::Get(rigID));
            animSystem->CreateAnimator(animatorID, rigID);
            res.animatorIDS.push_back(animatorID); 
            auto it = sceneData->RigMap.find((uint32_t)rigIdx);
            if (it != sceneData->RigMap.end())
            {
                const std::vector<uint32_t>& clipIndices = it->second;
                for (uint32_t clipIdx : clipIndices)
                {
                    const auto& clipInfo = sceneData->Clips[clipIdx];
                    UUID clipID = clipIDs[clipIdx];
                    
                    animSystem->RegisterClip(clipInfo, clipID, rigID);
                    animSystem->AddClip(animatorID, clipID);
                }
            }
        }

        // Upload meshes
        for (size_t meshIdx = 0; meshIdx < sceneData->Meshes.size(); meshIdx++)
        {
            const auto& meshInfo = sceneData->Meshes[meshIdx];

            int rigIdx = -1;
            for (const auto& node : sceneData->Hierarchy->nodes)
            {
                if (node.meshIdx == (int)meshIdx && node.animatorIdx >= 0)
                {
                    rigIdx = node.animatorIdx;
                    break;
                }
            }

            UUID meshID = AssetsRegister::Register(meshInfo.DebugName);
            res.meshMap.emplace_back();
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
                auto mat = res.matIDs[subInfo.MaterialIdx];
                res.meshMap.back().push_back(mat); 
                sm.MaterialIdx = res.meshMap.back().size() - 1;
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
            if (rigIdx >= 0)
                spec.RigPoseMats = animSystem->GetRestPoseMatrices(rigIDs[rigIdx]);

            AssetManager::CreateAsset<Mesh>(meshID, spec);
            res.meshIDs.push_back(meshID);
        }

        res.hierarchy = sceneData->Hierarchy;
        return res;
    }
}