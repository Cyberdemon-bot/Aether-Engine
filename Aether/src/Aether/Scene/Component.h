#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/SceneCamera.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/Math.h"
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

        void SetTranslation(Math::Vec3 translation)
        {
            Translation = (glm::vec3)translation;
            Dirty = true;
        }

        Math::Vec3 GetTranslation() { return Math::Vec3(Translation); }

        void SetRotation(Math::Quat rotation)
        {
            Rotation = (glm::quat)rotation;
            Dirty = true;
        }

        Math::Quat GetRotation() { return Math::Quat(Rotation); }

        void SetScale(Math::Vec3 scale)
        {
            Scale = (glm::vec3)scale;
            Dirty = true;
        }

        Math::Vec3 GetScale() { return Math::Vec3(Scale); }

        AE_REFLECT_NAME("TransformComponent")
        AE_PROP_LIST(
            AE_REFLECT_PROP(Translation, GetTranslation , SetTranslation),
            AE_REFLECT_PROP(Rotation, GetRotation, SetRotation),
            AE_REFLECT_PROP(Scale, GetScale, SetScale)
        )
        AE_OP_LIST()
    };

    struct LightComponent
    {
        LightParam Config;
        bool Culled = false;

        LightComponent() = default;
        LightComponent(const LightComponent&) = default;
        LightComponent(const LightParam& param) : Config(param) {}
    };

    struct MeshComponent
    {
        AssetHandle Mesh;
        MaterialTable Materials;
        bool ShowBounds = false;
        bool Culled = false;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
    };

    struct AnimatorComponent
    {
        UUID AnimatorID;

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;
        AnimatorComponent(const UUID& id) : AnimatorID(id) {};
    };

    struct BoneAttachmentComponent
    {
        UUID AnimatorID;
        bool Active = false;
        int BoneIdx = -1;
        
        BoneAttachmentComponent() = default;
        BoneAttachmentComponent(const BoneAttachmentComponent&) = default;
        BoneAttachmentComponent(const UUID& id) : AnimatorID(id) {};
        BoneAttachmentComponent(const UUID& id, int idx, bool active = false) : AnimatorID(id), BoneIdx(idx), Active(active) {};
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
        InstanceHandle Handle;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;
        ScriptComponent(const InstanceHandle& handle) : Handle(handle) {};
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
        BodyHandle Handle;
        bool Visible = false;
        glm::vec3 ColliderOffset;
        ColliderShape Shape;
        glm::vec3 Size; // box: halfx, halfy, halfz --- capsule: radius, height, __ --- sphere: radius, __, __
        ColliderComponent() = default;
        ColliderComponent(const ColliderComponent&) = default;
        ColliderComponent(const BodyHandle& handle, bool visible = false)
            : Handle(handle), Visible(visible)
        {
            auto it = PhysicsSystem::GetBodyInfo(Handle);
            if (it == nullptr) return;
            auto& info = *it;
            ColliderOffset = info.offset;
            Shape = info.shape;
            Size = info.size;
        }
    };
}