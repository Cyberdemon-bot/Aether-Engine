#include "aepch.h" 
#include "AssetRegister.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/ResourceManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

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
        mesh->m_SubMeshes = std::vector(info.submeshes.begin(), info.submeshes.end());
        AE_CORE_ASSERT(info.streams, "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(info.indicies, "Index data cannot be null!");

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

            Handle<Resource> vboHandle = ResourceManager::CreateResource<VertexBuffer>((float*)vbuffer.Data, byteSize);
            auto* vbo = ResourceManager::GetResource<VertexBuffer>(vboHandle);
            vbo->SetLayout(vbuffer.Layout);
            vao->AddVertexBuffer(vbo);
            mesh->m_VertexBuffers.push_back(vboHandle);
        }

        if (mesh->m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex = 0;
            defaultSubMesh.BaseIndex = 0;
            defaultSubMesh.VertexCount = vertex_cnt;
            defaultSubMesh.IndexCount  = info.indicies.size();
            mesh->m_SubMeshes.push_back(defaultSubMesh);
        }

        mesh->m_BoundsMin = info.boundsMin;
        mesh->m_BoundsMax = info.boundsMax;
        mesh->m_BoundsCenter = (mesh->m_BoundsMin + mesh->m_BoundsMax) * 0.5f;
        mesh->m_BoundsExtents = (mesh->m_BoundsMax - mesh->m_BoundsMin) * 0.5f;

        mesh->m_HasAnimatedBounds = info.hasAnimatedBounds;
        if (info.hasAnimatedBounds)
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
    }

    void AssetRegister::SheetAssembler(AssetManager* manager, const ASheetCreateInfo& info)
    {
        auto handle = manager->CreateAsset<ASheet>(info.id);

        if (!info.matList || info.matSize == 0) return;
        auto* sheet = manager->GetAsset<ASheet>(handle);
        std::vector<Handle<Asset>> temp; temp.reserve(info.matSize);
        for (uint32_t i = 0; i < info.matSize; i++) 
            temp.push_back(manager->GetHandle(info.matList[i]));
        sheet->MoveDefaultList(std::move(temp));
    }
}