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

        static void CreateBody(UUID bodyID, const BodyConfig& config)
        {
            s_PhysicsAPI->CreateBody(bodyID, config);
        }

        static void DestroyBody(UUID bodyID)
        {
            s_PhysicsAPI->DestroyBody(bodyID);
        }

        static RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance)
        {
            return s_PhysicsAPI->CastRay(origin, direction, distance);
        }

        static std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) 
        {
            return s_PhysicsAPI->CastRayAll(origin, direction, distance);
        }

        static void SetActive(UUID bodyID, bool active)
        {
            s_PhysicsAPI->SetActive(bodyID, active);
        }
        
        static void SetPhysTransform(UUID bodyID, const PhysTransform& transform)
        {
            s_PhysicsAPI->SetPhysTransform(bodyID, transform);
        }

        static PhysTransform GetPhysTransform(UUID bodyID)
        {
            return s_PhysicsAPI->GetPhysTransform(bodyID);
        }

        static void AddForce(UUID bodyID, const glm::vec3& force)
        {
            s_PhysicsAPI->AddForce(bodyID, force);
        }

        static void SetVelocity(UUID bodyID, const glm::vec3& velocity)
        {
            s_PhysicsAPI->SetVelocity(bodyID, velocity);
        }

        static void SetGravity(const glm::vec3& gravity)
        {
            s_PhysicsAPI->SetGravity(gravity);
        }

        static bool CanMove(UUID bodyID, const PhysTransform& target)
        {
            return s_PhysicsAPI->CanMove(bodyID, target);
        }

        static const BodyConfig* GetBodyInfo(UUID bodyID)
        {
            return s_PhysicsAPI->GetBodyInfo(bodyID);
        }
    
    private:
        static Scope<PhysicsAPI> s_PhysicsAPI;
    };
}