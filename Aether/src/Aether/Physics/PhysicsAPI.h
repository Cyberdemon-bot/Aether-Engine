#pragma once
#include "Aether/Core/Timestep.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Container/ResourcePool.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

namespace Aether {
    struct BodyTag;
    struct CallbackTag;
    enum class ColliderShape
    {
        None = 0, // point
        Box, Sphere, Capsule
    };

    enum class MotionType
    {
        None = 0,
        Static, 
        Kinematic,  
        Dynamic    
    };

    enum class CollisionType 
    {
        None = 0, 
        Enter, Stay, Exit
    };

    struct PhysTransform
    {
        glm::vec3 translation;
        glm::quat rotation;
    };

    struct RaycastHit
    {
        bool Hit = false;
        glm::vec3 Position{0.0f}; 
        glm::vec3 Normal{0.0f};  
        float Distance = 0.0f;    
        Handle<BodyTag> HitEntityHandle;
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

    struct CollisionEvent 
    {
        CollisionType type;
        Handle<BodyTag> bodyA;
        Handle<BodyTag> bodyB;

        glm::vec3 contactPoint;
        glm::vec3 contactNormal;
    };

    using CollisionCallbackRef = Delegate<void(const CollisionEvent&)>;

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

        virtual Handle<BodyTag> CreateBody(const BodyConfig& config) = 0;
        virtual void DestroyBody(Handle<BodyTag> handle) = 0;

        virtual Handle<CallbackTag> RegisterCallback(const CollisionCallbackRef& callback) = 0;
        virtual void RemoveCallback(Handle<CallbackTag> handle) = 0;

        virtual const BodyConfig* GetBodyInfo(Handle<BodyTag> handle) const = 0;

        virtual RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance) = 0;
        virtual std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) = 0;
        virtual bool CanMove(Handle<BodyTag> handle, const PhysTransform& target) = 0;

        virtual void SetActive(Handle<BodyTag> handle, bool active) = 0;
        virtual void SetUUID(Handle<BodyTag> handle, UUID id) = 0;
        virtual UUID GetUUID(Handle<BodyTag> handle) = 0;
        
        virtual void SetPhysTransform(Handle<BodyTag> handle, const PhysTransform& transform) = 0;
        virtual PhysTransform GetPhysTransform(Handle<BodyTag> handle) const = 0;

        virtual void AddForce(Handle<BodyTag> handle, const glm::vec3& force) = 0;
        virtual void SetVelocity(Handle<BodyTag> handle, const glm::vec3& velocity) = 0;
        virtual void SetGravity(const glm::vec3& gravity) = 0;

        static Scope<PhysicsAPI> Create();
    private:
        static API s_API;
    };

}