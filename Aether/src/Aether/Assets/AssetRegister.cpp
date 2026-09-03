#include "aepch.h" 
#include "Aether/Core/Base.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Assets/AssetRegister.h"

namespace Aether {

    void AssetRegister::Init() 
    {
        m_DebugInfo.reserve(32);
    };

    void AssetRegister::Shutdown()
    {
        m_DebugInfo.clear();
    }

    std::string AssetRegister::GetInfo(UUID key)
    {
        auto it = m_DebugInfo.find(key);
        if (it == m_DebugInfo.end()) 
        {
            AE_CORE_ERROR("ID '{0}' has no debug info!", uint64_t(key));
            return ""; 
        }
        return it->second;
    }

    void AssetRegister::MeshAssembler(AssetManager* manager, const AMeshCreateInfo& info)
    {
        auto handle = manager->CreateAsset<AMesh>(info.id);
        auto* mesh = manager->GetAsset<AMesh>(handle);
        mesh->m_Submeshes = std::vector(info.Submeshes.begin(), info.Submeshes.end());
        AE_CORE_ASSERT(!info.streams.empty(), "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(!info.indicies.empty(), "Index data cannot be null!");

        mesh->m_VertexArray = ResourceManager::CreateResource<VertexArray>();
        auto* vao = ResourceManager::GetResource<VertexArray>(mesh->m_VertexArray);

        mesh->m_IndexBuffer = ResourceManager::CreateResource<IndexBuffer>(info.indicies.data(), info.indicies.size());
        vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(mesh->m_IndexBuffer));

        uint32_t vertex_cnt = info.streams[0].VertexCount;

        for (const auto& vbuffer : info.streams)
        {
            AE_CORE_ASSERT(vbuffer.VertexCount == vertex_cnt, "vbuffer's size mismatch in stream!");

            uint32_t stride = vbuffer.Layout.GetStride();
            uint32_t byteSize = vbuffer.VertexCount * stride;

            Handle<Resource> vboHandle = ResourceManager::CreateResource<VertexBuffer>(reinterpret_cast<const float*>(vbuffer.Data), byteSize);
            auto* vbo = ResourceManager::GetResource<VertexBuffer>(vboHandle);
            vbo->SetLayout(vbuffer.Layout);
            vao->AddVertexBuffer(vbo);
            mesh->m_VertexBuffers.push_back(vboHandle);
        }

        if (mesh->m_Submeshes.empty())
        {
            Submesh defaultSubmesh;
            defaultSubmesh.BaseVertex = 0;
            defaultSubmesh.BaseIndex = 0;
            defaultSubmesh.VertexCount = vertex_cnt;
            defaultSubmesh.IndexCount  = info.indicies.size();
            mesh->m_Submeshes.push_back(defaultSubmesh);
        }

        mesh->m_BoundsMin = info.boundsMin;
        mesh->m_BoundsMax = info.boundsMax;
        mesh->m_BoundsCenter = (mesh->m_BoundsMin + mesh->m_BoundsMax) * 0.5f;
        mesh->m_BoundsExtents = (mesh->m_BoundsMax - mesh->m_BoundsMin) * 0.5f;
        mesh->m_HasJointData = info.hasJointData;
        if (mesh->m_HasJointData)
        {
            mesh->m_AnimatedBoundsMin = info.animatedBoundsMin;
            mesh->m_AnimatedBoundsMax = info.animatedBoundsMax;
        }
    }

    void AssetRegister::ImageAssembler(AssetManager* manager, const AImageCreateInfo& info)
    {
        auto texture = ResourceManager::CreateResource<Texture2D>(info.layout);
        auto* resource = ResourceManager::GetResource<Texture2D>(texture);
        if (!resource) AE_CORE_ERROR("[Asset Register] Failed to Create texture!");
        resource->SetData((void*)info.raw.data(), info.raw.size());
        manager->CreateAsset<AImage>(info.id, texture);
    }

    void AssetRegister::MaterialAssembler(AssetManager* manager, const AMaterialCreateInfo& info)
    {
        auto handle = manager->CreateAsset<AMaterial>(info.id);
        auto* material = manager->GetAsset<AMaterial>(handle);

        // Set material properties
        material->AddVec4("u_AlbedoColor", info.albedo);
        material->AddFloat("u_Metallic", info.metallic);
        material->AddFloat("u_Roughness", info.roughness);

        material->AddImage("u_AlbedoMap", manager->GetHandle(info.albedoMap)); 
        material->AddImage("u_MetallicRoughnessMap", manager->GetHandle(info.metallicRoughnessMap));

        auto normal = manager->GetHandle(info.normalMap);
        if (normal.IsValid())
        {
            material->AddImage("u_NormalMap", normal);
            material->AddInt("u_HasNormalMap", 1);
        }
        else material->AddInt("u_HasNormalMap", 0);
    }

    void AssetRegister::SkeletonAssembler(AssetManager* manager, const ASkeletonCreateInfo& info)
    {
        auto* rsys = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        auto rskel = rsys->CreateSkeleton(info.data);

        auto handle = manager->CreateAsset<ASkeleton>(info.id);
        auto* skeleton = manager->GetAsset<ASkeleton>(handle);
        skeleton->m_Handle = rskel;
        skeleton->m_JointCount = info.data.Joints.size();
    }

    void AssetRegister::ClipAssembler(AssetManager* manager, const AClipCreateInfo& info)
    {
        auto* rsys = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        auto Clip = rsys->CreateClip(info.data, manager->GetAsset<ASkeleton>(info.skeleton)->m_Handle);

        auto handle = manager->CreateAsset<AClip>(info.id);
        auto* clip = manager->GetAsset<AClip>(handle);
        clip->m_Handle = Clip;
        clip->m_Duration = info.data.Duration;
        clip->m_Skeleton = manager->GetHandle(info.skeleton);
    }

    void AssetRegister::SheetAssembler(AssetManager* manager, const ASheetCreateInfo& info)
    {
        auto handle = manager->CreateAsset<ASheet>(info.id);

        if (info.materialList.empty()) return;
        auto* sheet = manager->GetAsset<ASheet>(handle);
        std::vector<Handle<Asset>> temp; temp.reserve(info.materialList.size());
        for (uint32_t i = 0; i < info.materialList.size(); i++) 
            temp.push_back(manager->GetHandle(info.materialList[i]));
        sheet->MoveDefaultList(std::move(temp));
    }

    void AssetRegister::AudioAssambler(AssetManager* manager, const AAudioCreateInfo& info)
    {
        auto* audiosys = ServiceManager::GetService<AudioSystem>();
        auto source = audiosys->CreateSource(info.raw.data(), info.raw.size());
        manager->CreateAsset<AAudio>(info.id, source);
    }

    BatchRegisterResult AssetRegister::RegisterBatch(Ref<CreateInfoList> createInfoList)
    {
        BatchRegisterResult result;
        result.m_AllIDs.reserve(createInfoList->GetAssetCount());

        ProcessAssetGroup<AMesh, AMeshCreateInfo>(AssetType::Mesh, createInfoList.get(), result);
        ProcessAssetGroup<AImage, AImageCreateInfo>(AssetType::Image, createInfoList.get(), result);
        ProcessAssetGroup<AMaterial, AMaterialCreateInfo>(AssetType::Material, createInfoList.get(), result);
        ProcessAssetGroup<ASheet, ASheetCreateInfo>(AssetType::Sheet, createInfoList.get(), result);
        ProcessAssetGroup<ASkeleton, ASkeletonCreateInfo>(AssetType::Skeleton, createInfoList.get(), result);
        ProcessAssetGroup<AClip, AClipCreateInfo>(AssetType::Clip, createInfoList.get(), result);
        ProcessAssetGroup<AAudio, AAudioCreateInfo>(AssetType::Audio, createInfoList.get(), result);

        return result; 
    }
}