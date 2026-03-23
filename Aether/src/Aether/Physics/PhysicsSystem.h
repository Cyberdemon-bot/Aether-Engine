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

        static BodyHandle CreateBody(const BodyConfig& config)
        {
            return s_PhysicsAPI->CreateBody(config);
        }

        static void DestroyBody(BodyHandle handle)
        {
            s_PhysicsAPI->DestroyBody(handle);
        }

        static RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance)
        {
            return s_PhysicsAPI->CastRay(origin, direction, distance);
        }

        static std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) 
        {
            return s_PhysicsAPI->CastRayAll(origin, direction, distance);
        }

        static void SetActive(BodyHandle handle, bool active)
        {
            s_PhysicsAPI->SetActive(handle, active);
        }

        static void SetUUID(BodyHandle handle, UUID id)
        {
            s_PhysicsAPI->SetUUID(handle, id);
        }

        static UUID GetUUID(BodyHandle handle)
        {
            return s_PhysicsAPI->GetUUID(handle);
        }
        
        static void SetPhysTransform(BodyHandle handle, const PhysTransform& transform)
        {
            s_PhysicsAPI->SetPhysTransform(handle, transform);
        }

        static PhysTransform GetPhysTransform(BodyHandle handle)
        {
            return s_PhysicsAPI->GetPhysTransform(handle);
        }

        static void AddForce(BodyHandle handle, const glm::vec3& force)
        {
            s_PhysicsAPI->AddForce(handle, force);
        }

        static void SetVelocity(BodyHandle handle, const glm::vec3& velocity)
        {
            s_PhysicsAPI->SetVelocity(handle, velocity);
        }

        static void SetGravity(const glm::vec3& gravity)
        {
            s_PhysicsAPI->SetGravity(gravity);
        }

        static bool CanMove(BodyHandle handle, const PhysTransform& target)
        {
            return s_PhysicsAPI->CanMove(handle, target);
        }

        static const BodyConfig* GetBodyInfo(BodyHandle handle)
        {
            return s_PhysicsAPI->GetBodyInfo(handle);
        }
    
    private:
        static Scope<PhysicsAPI> s_PhysicsAPI;
    };
}