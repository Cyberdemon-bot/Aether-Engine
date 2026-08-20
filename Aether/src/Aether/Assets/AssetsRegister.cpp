#include "aepch.h" 
#include "AssetsRegister.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    void AssetsRegister::Init() 
    {
        m_Map.reserve(32);
    };

    void AssetsRegister::Shutdown()
    {
        m_Map.clear();
    }

    std::string AssetsRegister::Get(UUID key)
    {
        if (m_Map.find(key) == m_Map.end()) 
        {
            AE_CORE_ERROR("Key '{0}' has not registered yet!", uint64_t(key));
            return ""; 
        }
        return m_Map[key];
    }

    void AssetsRegister::MeshAssembler(UUID id, AssetManager* manager, const AMeshCreateInfo& info)
    {
        auto handle = manager->CreateAsset<AMesh>(id);
        auto* mesh = manager->GetAsset<AMesh>(handle);
        mesh->m_SubMeshes = std::move(info.submeshes);
        AE_CORE_ASSERT(info.streams, "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(info.indicies, "Index data cannot be null!");

        mesh->m_VertexArray = ResourceManager::CreateResource<VertexArray>();
        auto* vao = ResourceManager::GetResource<VertexArray>(mesh->m_VertexArray);

        mesh->m_IndexBuffer = ResourceManager::CreateResource<IndexBuffer>((uint32_t*)info.indicies, info.indexLen);
        vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(mesh->m_IndexBuffer));

        uint32_t vertex_cnt = info.streams[0].VertexCount;

        for (int i = 0; i < info.streamLen; i++)
        {
            const auto& vbuffer = info.streams[i];
            AE_CORE_ASSERT(vbuffer.VertexCount == vertex_cnt, "vbuffer's size mismatch in stream!");

            uint32_t stride = vbuffer.Layout.GetStride();
            uint32_t byteSize = vbuffer.VertexCount * stride;

            Handle<Resource> vboHandle = ResourceManager::CreateResource<VertexBuffer>((float*)vbuffer.Data, byteSize);
            auto* vbo = ResourceManager::GetResource<VertexBuffer>(vboHandle);
            vbo->SetLayout(vbuffer.Layout);
            vao->AddVertexBuffer(vbo);
            mesh->m_VertexBuffers.push_back(vboHandle);
        }

        if (mesh->m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex  = 0;
            defaultSubMesh.BaseIndex   = 0;
            defaultSubMesh.VertexCount = vertex_cnt;
            defaultSubMesh.IndexCount  = info.indexLen;
            mesh->m_SubMeshes.push_back(defaultSubMesh);
        }

        if (info.CalculateBoundsFunc)
        {
            auto [tempMin, tempMax] = info.CalculateBoundsFunc(info);
            mesh->m_BoundsMin = tempMin;
            mesh->m_BoundsMax = tempMax;
            mesh->m_BoundsCenter = (mesh->m_BoundsMin + mesh->m_BoundsMax) * 0.5f;
            mesh->m_BoundsExtents = (mesh->m_BoundsMax - mesh->m_BoundsMin) * 0.5f;
        }

        if (info.CalculateAnimatedBoundsFunc)
        {
            auto [tempAnimMin, tempAnimMax] = info.CalculateAnimatedBoundsFunc(info);
            mesh->m_AnimatedBoundsMin = tempAnimMin;
            mesh->m_AnimatedBoundsMax = tempAnimMax;
            mesh->m_HasAnimatedBounds = true;
        }
        else mesh->m_HasAnimatedBounds = false;
    }

    void AssetsRegister::ImageAssembler(UUID id, AssetManager* manager, const AImageCreateInfo& info)
    {
        auto texture = ResourceManager::CreateResource<Texture2D>(info.layout);
        auto* resource = ResourceManager::GetResource<Texture2D>(texture);
        if (!resource) AE_CORE_ERROR("[Asset Register] Failed to Create texture!");
        resource->SetData((void*)info.raw.data(), info.raw.size());
        manager->CreateAsset<AImage>(id, texture);
    }

    void AssetsRegister::MaterialAssembler(UUID id, AssetManager* manager, const AMaterialCreateInfo& info)
    {
        auto handle = manager->CreateAsset<AMaterial>(id);
        auto* material = manager->GetAsset<AMaterial>(handle);

        // Set material properties
        material->AddVec4("u_AlbedoColor", info.albedo);
        material->AddFloat("u_Metallic", info.metallic);
        material->AddFloat("u_Roughness", info.roughness);

        if (!info.imageList || info.imageSize == 0) return;

        if (info.albedoMapIdx >= 0 && info.albedoMapIdx < info.imageSize)
            material->AddImage("u_AlbedoMap", manager->GetHandle(info.imageList[info.albedoMapIdx])); 
        
        if (info.normalMapIdx >= 0 && info.normalMapIdx < info.imageSize)
        {
            material->AddImage("u_NormalMap", manager->GetHandle(info.imageList[info.normalMapIdx]));
            material->AddInt("u_HasNormalMap", 1);
        }
        else  material->AddInt("u_HasNormalMap", 0);

        if (info.metallicRoughnessMapIdx >= 0 && info.metallicRoughnessMapIdx < info.imageSize)
            material->AddImage("u_MetallicRoughnessMap", manager->GetHandle(info.imageList[info.metallicRoughnessMapIdx]));
    }

    void AssetsRegister::SkeletonAssembler(UUID id, AssetManager* manager, const ASkeletonCreateInfo& info)
    {
        auto* rsys = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        auto rskel = rsys->CreateSkeleton(info);

        auto handle = manager->CreateAsset<ASkeleton>(id);
        auto* skeleton = manager->GetAsset<ASkeleton>(handle);
        skeleton->m_Handle = rskel;
        skeleton->m_JointCount = info.Joints.size();
    }

    void AssetsRegister::ClipAssembler(UUID id, AssetManager* manager, const AClipCreateInfo& info)
    {
        auto* rsys = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        auto Clip = rsys->CreateClip(info.layout, manager->GetAsset<ASkeleton>(info.skeleton)->m_Handle);

        auto handle = manager->CreateAsset<AClip>(id);
        auto* clip = manager->GetAsset<AClip>(handle);
        clip->m_Handle = Clip;
        clip->m_Duration = info.layout.Duration;
    }

    void AssetsRegister::SheetAssembler(UUID id, AssetManager* manager, const ASheetCreateInfo& info)
    {
        auto handle = manager->CreateAsset<ASheet>(id);

        if (!info.matList || info.matSize == 0) return;
        auto* sheet = manager->GetAsset<ASheet>(handle);
        std::vector<Handle<Asset>> temp; temp.reserve(info.matSize);
        for (uint32_t i = 0; i < info.matSize; i++) 
            temp.push_back(manager->GetHandle(info.matList[i]));
        sheet->MoveDefaultList(std::move(temp));
    }
}