#pragma once
#include "Aether/Physics/PhysicsAPI.h"

namespace Aether {
    class AETHER_API PhysicsSystem
    {
    public:
        static void Init()
        {
            s_PhysicsAPI->Init();
        }

        static void Shutdown()
        {
            s_PhysicsAPI->Shutdown();
        }

        static void Update(Timestep ts)
        {
            s_PhysicsAPI->Update(ts);
        }

        static Handle<BodyTag> CreateBody(const BodyConfig& config)
        {
            return s_PhysicsAPI->CreateBody(config);
        }

        static void DestroyBody(Handle<BodyTag> handle)
        {
            s_PhysicsAPI->DestroyBody(handle);
        }

        static Handle<CallbackTag> RegisterCallback(const CollisionCallbackRef& callback)
        {
            return s_PhysicsAPI->RegisterCallback(callback);
        }

        static void RemoveCallback(Handle<CallbackTag> handle) 
        {
            s_PhysicsAPI->RemoveCallback(handle);
        }

        static RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance)
        {
            return s_PhysicsAPI->CastRay(origin, direction, distance);
        }

        static std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) 
        {
            return s_PhysicsAPI->CastRayAll(origin, direction, distance);
        }

        static void SetActive(Handle<BodyTag> handle, bool active)
        {
            s_PhysicsAPI->SetActive(handle, active);
        }

        static void SetUUID(Handle<BodyTag> handle, UUID id)
        {
            s_PhysicsAPI->SetUUID(handle, id);
        }

        static UUID GetUUID(Handle<BodyTag> handle)
        {
            return s_PhysicsAPI->GetUUID(handle);
        }
        
        static void SetPhysTransform(Handle<BodyTag> handle, const PhysTransform& transform)
        {
            s_PhysicsAPI->SetPhysTransform(handle, transform);
        }

        static PhysTransform GetPhysTransform(Handle<BodyTag> handle)
        {
            return s_PhysicsAPI->GetPhysTransform(handle);
        }

        static void AddForce(Handle<BodyTag> handle, const glm::vec3& force)
        {
            s_PhysicsAPI->AddForce(handle, force);
        }

        static void SetVelocity(Handle<BodyTag> handle, const glm::vec3& velocity)
        {
            s_PhysicsAPI->SetVelocity(handle, velocity);
        }

        static void SetGravity(const glm::vec3& gravity)
        {
            s_PhysicsAPI->SetGravity(gravity);
        }

        static bool CanMove(Handle<BodyTag> handle, const PhysTransform& target)
        {
            return s_PhysicsAPI->CanMove(handle, target);
        }

        static const BodyConfig* GetBodyInfo(Handle<BodyTag> handle)
        {
            return s_PhysicsAPI->GetBodyInfo(handle);
        }
    
    private:
        static Scope<PhysicsAPI> s_PhysicsAPI;
    };
}