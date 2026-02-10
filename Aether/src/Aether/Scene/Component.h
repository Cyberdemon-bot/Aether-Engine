#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/SceneCamera.h"

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
        glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        glm::mat4 WorldTransform = glm::mat4(1.0f);  
        bool Dirty = true;

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& translation) : Translation(translation) {}

        void CalcTransform()
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), Translation);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);

            WorldTransform = translation * rotation * scale;
        }
    };

    struct MeshComponent
    {
        UUID MeshID;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
        MeshComponent(const UUID& id) : MeshID(id) {};
    };

    struct AnimatorComponent
    {
        UUID AnimatorID;

        AnimatorComponent() = default;
        AnimatorComponent(const AnimatorComponent&) = default;
        AnimatorComponent(const UUID& id) : AnimatorID(id) {};
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
}