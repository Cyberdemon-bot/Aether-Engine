#include "aepch.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Scene/Component.h"
#include "Aether/Assets/Material.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {
    glm::mat4 TransformComponent::GetLocalTransform() const
    {
        glm::mat4 rotation = glm::toMat4(Rotation);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

        return translation * rotation * scale;
    }

    void MeshComponent::AttachUniqueSheet()
    {
        UsingUniqueSheet = true;
        auto* asset_manager = ServiceManager::GetService<AssetManager>();
        UniqueSheet = asset_manager->CreateAsset<Sheet>(UUID()); 
        auto* ss = asset_manager->GetAsset<Sheet>(SharedSheet);
        auto* us = asset_manager->GetAsset<Sheet>(UniqueSheet);
        us->Resize(ss->GetSize());
        us->CopyDefaultList(ss->BaseHandles);
    }

    void MeshComponent::DetachUniqueSheet()
    {
        UsingUniqueSheet = false;
        ServiceManager::GetService<AssetManager>()->Unload(UniqueSheet);
    }

    void AnimatorComponent::SetClip(int idx)
    {
        ActiveClipIdx = idx;
        CacheDirty = true;
    }

    ColliderComponent::ColliderComponent(Handle<PhysicsInstance> instance, Handle<RigidBody> handle, bool visible)
            : ColliderHandle(handle), Visible(visible)
    {
        auto it = ServiceManager::GetService<PhysicsSystem>()->GetBodyInfo(instance, ColliderHandle);
        auto& info = it;
        ColliderOffset = info.offset;
        Shape = info.shape;
        Size = info.size;
        Type = info.motionType;
        Mass = info.mass;
        Friction = info.friction;
        Restitution = info.restitution;
        IsSensor = info.isSensor;
    }

    void BoneAttachmentComponent::Invalidate() const
    {
        JointIndex = -1;
    }
}