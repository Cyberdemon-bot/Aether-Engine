#include "aepch.h"
#include "Aether/Importer/ImporterAPI.h"
#include "Platform/Cgltf/GLTF_ImporterAPI.h"
#include "Aether/Core/AssetsRegister.h"
#include "Aether/Resources/Material.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Texture.h"
#include "Aether/Resources/Shader.h"
#include "Aether/Animation/AnimationManager.h"

namespace Aether {

	ImporterAPI::API ImporterAPI::s_API = ImporterAPI::API::Cgltf;
    Ref<MeshParser> ImporterAPI::m_MeshParser;
    Ref<MaterialParser> ImporterAPI::m_MaterialParser;
    Ref<AnimationParser> ImporterAPI::m_AnimationParser;

	Scope<ImporterAPI> ImporterAPI::Create()
	{
        m_MeshParser = MeshParser::Create();
        m_MaterialParser = MaterialParser::Create();
        m_AnimationParser = AnimationParser::Create();
        
		switch (s_API)
		{
			case ImporterAPI::API::None:    AE_CORE_ASSERT(false, "ImporterAPI::None is currently not supported!"); return nullptr;
			case ImporterAPI::API::Cgltf:  return CreateScope<GLTF_ImporterAPI>();
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

    RegisteredScene ImporterAPI::Upload(const ParsedScene& sceneData, UUID shaderID)
    {
        RegisteredScene res;
        std::vector<UUID> texIDs;
        
        // Upload textures
        for (const auto& texInfo : sceneData.Textures)
        {
            UUID texID = AssetsRegister::Register(texInfo.DebugName);
            auto tex = Texture2D::Create(texInfo.Spec);
            tex->SetData((void*)texInfo.RawData.data(), texInfo.RawData.size());
            Texture2DLibrary::Add(tex, texID);
            texIDs.push_back(texID);
        }
        
        // Upload materials
        for (const auto& matInfo : sceneData.Materials)
        {
            UUID matID = AssetsRegister::Register(matInfo.DebugName);
            auto material = Material::Create(shaderID);
            
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

        auto animSystem = AnimationManager::GetSystem<SkeletalAnimationSystem>(AnimationType::Skeletal);

        for (const auto& skelInfo : sceneData.Skeletons)
        {
            UUID skelID = AssetsRegister::Register(skelInfo.DebugName);
            animSystem->RegisterSkeleton(skelInfo, skelID);
            res.skelIDs.push_back(skelID);
        }
        
        for (const auto& clipInfo : sceneData.Clips)
        {
            UUID clipID = AssetsRegister::Register(clipInfo.DebugName);
            animSystem->RegisterClip(clipInfo, clipID);
            res.clipIDs.push_back(clipID);
        }

        return res;
    }
}