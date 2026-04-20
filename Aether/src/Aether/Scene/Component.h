#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/SceneCamera.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Animation/RigModule.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Aether {
    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;
        IDComponent(UUID id) : ID(id) {};
    };

    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}
    };

    struct TransformComponent
    {
        using Self = TransformComponent;
        glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
        glm::quat Rotation = glm::quat({0.0f, 0.0f, 0.0f});
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        glm::mat4 WorldTransform = glm::mat4(1.0f);  
        uint64_t LastUpdate = 0;
        bool Dirty = true;
        bool SubtreeDirty = false;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& translation) : Translation(translation) {}
        TransformComponent(const glm::vec3& translation, const glm::quat& quat, const glm::vec3& scale) 
            : Translation(translation), Rotation(quat), Scale(scale)  {}

        glm::mat4 GetLocalTransform() const
        {
            glm::mat4 rotation = glm::toMat4(Rotation);
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

            return translation * rotation * scale;
        }
    };

    struct LightComponent
    {
        LightParam Config;
        mutable bool Culled = false;

        LightComponent() = default;
        LightComponent(const LightComponent&) = default;
        LightComponent(const LightParam& param) : Config(param) {}
    };

    struct MeshComponent
    {
        AssetHandle Mesh;
        MaterialTable Materials;
        bool ShowBounds = false;
        mutable bool Culled = false;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
    };

    struct AnimatorComponent
    {
        AssetHandle Skeleton;
        std::vector<AssetHandle> Clips;
        
        Handle<CacheTag> Cache;
        Handle<TaskTag>  CurrentTask; 

        int ActiveClipIdx = 0;
        float CurrentTime = 0.0f;
        float Speed = 1.0f;
        bool IsPlaying = true;
        bool Loop = true;
        mutable bool Culled = false;

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;
    };

    struct AudioSourceComponent
    {
        UUID SourceID;

        AudioSourceComponent() = default;
        AudioSourceComponent(const AudioSourceComponent&) = default;
        AudioSourceComponent(const UUID& id) : SourceID(id) {};
    };

    struct ScriptComponent
    {
        Handle<ScriptTag> ScriptHandle;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;
        ScriptComponent(const Handle<ScriptTag>& handle) : ScriptHandle(handle) {};
    };

    struct HierarchyComponent 
    {
        Entity parent = Null_Entity;
        Entity firstChild = Null_Entity;
        Entity nextSibling = Null_Entity;
        Entity prevSibling = Null_Entity;

        HierarchyComponent() = default;
        HierarchyComponent(const HierarchyComponent&) = default;
    };

    struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true; 
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

    struct ColliderComponent
    {
        Handle<BodyTag> ColliderHandle;
        bool Visible = false;
        glm::vec3 ColliderOffset;
        ColliderShape Shape;
        glm::vec3 Size; // box: halfx, halfy, halfz --- capsule: radius, height, __ --- sphere: radius, __, __
        ColliderComponent() = default;
        ColliderComponent(const ColliderComponent&) = default;
        ColliderComponent(const Handle<BodyTag>& handle, bool visible = false)
            : ColliderHandle(handle), Visible(visible)
        {
            auto it = PhysicsSystem::GetBodyInfo(ColliderHandle);
            if (it == nullptr) return;
            auto& info = *it;
            ColliderOffset = info.offset;
            Shape = info.shape;
            Size = info.size;
        }
    };

    struct BoneAttachmentComponent
    {
        Entity AnimatorEntity = Null_Entity;
        std::string BoneName;
        glm::vec3 LocalOffset = {0.0f, 0.0f, 0.0f};
        glm::quat LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); 
 
        mutable int BoneIndex = -1;
        mutable Handle<SkeletonTag> CachedSkeletonHnd = Handle<SkeletonTag>::MakeInvalid();
        mutable glm::mat4 CachedBoneWorld = glm::mat4(1.0f);
 
        BoneAttachmentComponent() = default;
        BoneAttachmentComponent(const BoneAttachmentComponent&) = default;
        BoneAttachmentComponent(Entity animatorEntity, std::string_view boneName, 
                                const glm::vec3& offset   = {0.0f, 0.0f, 0.0f},
                                const glm::quat& localRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
            : AnimatorEntity(animatorEntity)
            , BoneName(boneName)
            , LocalOffset(offset)
            , LocalRotation(localRot)
        {}
 
        glm::mat4 GetLocalAttachTransform() const
        {
            return glm::translate(glm::mat4(1.0f), LocalOffset) * glm::toMat4(LocalRotation);
        }
        void Invalidate() const { BoneIndex = -1; }
    };
}