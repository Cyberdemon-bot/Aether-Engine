#include "aepch.h"
#include "Aether/Importer/ImporterAPI.h"
#include "Platform/Cgltf/GLTF_ImporterAPI.h"
#include "Aether/Core/AssetsRegister.h"
#include "Aether/Resources/Material.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Texture.h"
#include "Aether/Resources/Shader.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigSystem.h"

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

    RegisteredScene ImporterAPI::Upload(const ParsedScene& sceneData)
    {
        RegisteredScene res;
        std::vector<UUID> texIDs;
        std::vector<UUID> rigIDs;     
        std::vector<UUID> clipIDs; 
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
            auto material = Material::Create();
            
            // Set textures
            if (matInfo.AlbedoMapIdx >= 0 && matInfo.AlbedoMapIdx < texIDs.size())
                material->AddTexture("u_AlbedoMap", Texture2DLibrary::Get(texIDs[matInfo.AlbedoMapIdx]));
            
            if (matInfo.NormalMapIdx >= 0 && matInfo.NormalMapIdx < texIDs.size())
            {
                material->AddTexture("u_NormalMap", Texture2DLibrary::Get(texIDs[matInfo.NormalMapIdx]));
                material->AddInt("u_HasNormalMap", 1);
            }
            else  material->AddInt("u_HasNormalMap", 0);

            if (matInfo.MetallicRoughnessMapIdx >= 0 && matInfo.MetallicRoughnessMapIdx < texIDs.size())
                material->AddTexture("u_MetallicRoughnessMap", Texture2DLibrary::Get(texIDs[matInfo.MetallicRoughnessMapIdx]));
            
            // Set material properties
            material->AddVec4("u_AlbedoColor", matInfo.AlbedoColor);
            material->AddFloat("u_Metallic", matInfo.Metallic);
            material->AddFloat("u_Roughness", matInfo.Roughness);
            
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
        auto animSystem = AnimationSystem::GetModule<RigSystem>();
        for (const auto& rigInfo : sceneData.Rigs)
        {
            UUID rigID = AssetsRegister::Register(rigInfo.DebugName);
            animSystem->RegisterSkeleton(rigInfo, rigID);
            rigIDs.push_back(rigID);
        }

        for (const auto& clipInfo : sceneData.Clips)
        {
            UUID clipID = AssetsRegister::Register(clipInfo.DebugName);
            clipIDs.push_back(clipID);
        }

        for (auto& [rigIdx, clipIndices] : sceneData.RigMap)
        {
            UUID rigID = rigIDs[rigIdx];
            UUID animatorID = AssetsRegister::Register("RigAnimator_" + AssetsRegister::Get(rigID));
            animSystem->CreateAnimator(animatorID, rigID);
            res.animatorIDS.push_back(animatorID);
            
            for (uint32_t clipIdx : clipIndices)
            {
                const auto& clipInfo = sceneData.Clips[clipIdx];
                UUID clipID = clipIDs[clipIdx];
                animSystem->RegisterClip(clipInfo, clipID, rigID);
                animSystem->AddClip(animatorID, clipID);
            }
        }

        return res;
    }
}