#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include "Aether/Core/Timestep.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {

    enum class ColliderShape
    {
        None = 0, // point
        Box, Sphere, Capsule
    };

    AE_REFLECT_ENUM(ColliderShape,
        AE_ENUM_VAL(ColliderShape::Box),
        AE_ENUM_VAL(ColliderShape::Sphere),
        AE_ENUM_VAL(ColliderShape::Capsule)
    )

    enum class MotionType
    {
        None = 0,
        Static, 
        Kinematic,  
        Dynamic    
    };

    struct PhysTransform
    {
        glm::vec3 translation;
        glm::quat rotation;
    };

    struct BodyHandle
    {
        int index = -1, generation = -1;

        bool IsValid() const { return index >= 0 && generation >= 0; }
        void MakeInvalid() { index = -1, generation = -1; }
    };

    struct RaycastHit
    {
        bool Hit = false;
        glm::vec3 Position{0.0f}; 
        glm::vec3 Normal{0.0f};  
        float Distance = 0.0f;    
        BodyHandle HitEntityHandle;
    };

    struct BodyConfig
    {
        MotionType motionType = MotionType::Static;
        ColliderShape shape = ColliderShape::Box;
        glm::vec3 size = { 0.5f, 0.5f, 0.5f };
        glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
        PhysTransform transform = { glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) };

        float mass = 1.0f;
        float friction = 0.5f;
        float restitution = 0.0f;
        bool isSensor = false;
    };

    class PhysicsAPI
    {
    public:
        enum class API {
            None = 0, JoltPhys = 1
        };
    public:
        virtual ~PhysicsAPI() = default;

        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Update(Timestep ts) = 0;

        virtual BodyHandle CreateBody(const BodyConfig& config) = 0;
        virtual void DestroyBody(BodyHandle handle) = 0;

        virtual const BodyConfig* GetBodyInfo(BodyHandle handle) const = 0;

        virtual RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance) = 0;
        virtual std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) = 0;
        virtual bool CanMove(BodyHandle handle, const PhysTransform& target) = 0;

        virtual void SetActive(BodyHandle handle, bool active) = 0;
        virtual void SetUUID(BodyHandle handle, UUID id) = 0;
        virtual UUID GetUUID(BodyHandle handle) = 0;
        
        virtual void SetPhysTransform(BodyHandle handle, const PhysTransform& transform) = 0;
        virtual PhysTransform GetPhysTransform(BodyHandle handle) const = 0;

        virtual void AddForce(BodyHandle handle, const glm::vec3& force) = 0;
        virtual void SetVelocity(BodyHandle handle, const glm::vec3& velocity) = 0;
        virtual void SetGravity(const glm::vec3& gravity) = 0;

        static Scope<PhysicsAPI> Create();
    private:
        static API s_API;
    };

}