#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Core/Delegate.h"
#include "Aether/Scene/Entity.h"
#include "Aether/Scene/SceneCamera.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/ServiceManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Aether {

    class Asset;
    struct ScriptInstance;
    
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
        Handle<Asset> Mesh;
        Handle<Asset> Sheet;
        bool ShowBounds = false;
        mutable bool Culled = false;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
    };

    struct AnimatorComponent
    {
        Handle<Asset> Skeleton;
        std::vector<Handle<Asset>> Clips;
        
        Handle<SkeletonCache> Cache;
        Handle<Pose> CurrentPose;

        bool RunSampling = true;
        bool RunPostEval = true;

        int ActiveClipIdx = 0;
        float CurrentTime = 0.0f;
        float Speed = 1.0f;
        bool IsPlaying = true;
        bool Loop = true;
        bool CacheDirty = false;
        mutable bool Culled = false;

        Delegate<void(Entity, RigModule* rig, float dt)> onPostEvaluate;

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;

        void SetClip(int idx)
        {
            ActiveClipIdx = idx;
            CacheDirty = true;
        }
    };

    struct AudioSourceComponent
    {
        Handle<AudioSource> SourceHandle = Handle<AudioSource>::MakeInvalid();
 
        std::string Path;
        AudioType Type = AudioType::Audio2D;
        float Volume = 1.0f;
        float Pan = 0.0f;
        float PlaybackSpeed = 1.0f;
        bool Looping = false;
        bool PlayOnStart = false;
        bool IsPlaying = false;
        Audio3DConfig Config3D;
 
        AudioSourceComponent() = default;
        AudioSourceComponent(const AudioSourceComponent&) = default;
        AudioSourceComponent(const std::string& path, AudioType type = AudioType::Audio2D)
            : Path(path), Type(type) {}
    };

    struct ScriptComponent
    {
        Handle<ScriptInstance> ScriptHandle;
        bool PendingStart = false;
        bool IsActive = true;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;
        ScriptComponent(const Handle<ScriptInstance>& handle) : ScriptHandle(handle) {};
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
        Handle<RigidBody> ColliderHandle = Handle<RigidBody>::MakeInvalid();
        glm::vec3 ColliderOffset = glm::vec3(1.0f);
        ColliderShape Shape = ColliderShape::Box;
        MotionType Type = MotionType::Kinematic;
        glm::vec3 Size = glm::vec3(0.5f); // box: halfx, halfy, halfz --- capsule: radius, height, __ --- sphere: radius, __, __
        bool Visible = false;
        bool IsActive = true;
        float Mass = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        bool IsSensor = false;
        ColliderComponent() = default;
        ColliderComponent(const ColliderComponent&) = default;
        ColliderComponent(Handle<PhysicsInstance> instance, Handle<RigidBody> handle, bool visible = false)
            : ColliderHandle(handle), Visible(visible)
        {
            auto it = ServiceManager::GetService<PhysicsSystem>()->GetBodyInfo(instance, ColliderHandle);
            if (it == nullptr) return;
            auto& info = *it;
            ColliderOffset = info.offset;
            Shape = info.shape;
            Size = info.size;
            Type = info.motionType;
            Mass = info.mass;
            Friction = info.friction;
            Restitution = info.restitution;
            IsSensor = info.isSensor;
        }
    };

    struct BoneAttachmentComponent
    {
        Entity AnimatorEntity = Null_Entity;
        std::string JointName;
        bool affectChild = true;
 
        mutable int JointIndex = -1;
        mutable Handle<Skeleton> CachedSkeletonHnd = Handle<Skeleton>::MakeInvalid();
        mutable glm::mat4 CachedBoneWorld = glm::mat4(1.0f);
 
        BoneAttachmentComponent() = default;
        BoneAttachmentComponent(const BoneAttachmentComponent&) = default;
        BoneAttachmentComponent(Entity animatorEntity, std::string_view jointName)
            : AnimatorEntity(animatorEntity)
            , JointName(jointName)
        {}
 
        void Invalidate() const { JointIndex = -1; }
    };

    template<typename Component>
    struct ComponentInfo
    {
        bool IsExits = false;
        Component data;
    };

    struct Prefab
    {
        ComponentInfo<TagComponent> tag;
        ComponentInfo<TransformComponent> transform;
        ComponentInfo<HierarchyComponent> hierarchy;
        ComponentInfo<MeshComponent> mesh;
        ComponentInfo<LightComponent> light;
        ComponentInfo<CameraComponent> camera;
        ComponentInfo<AnimatorComponent> animator;
        ComponentInfo<ColliderComponent> collider;
        ComponentInfo<ScriptComponent> script;
        ComponentInfo<BoneAttachmentComponent> boneAttach;
    };
}