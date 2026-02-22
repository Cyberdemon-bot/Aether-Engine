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

        static MotionType GetMotionType(UUID bodyID)
        {
            return s_PhysicsAPI->GetMotionType(bodyID);
        }

        static ColliderShape GetColliderShape(UUID bodyID)
        {
            return s_PhysicsAPI->GetColliderShape(bodyID);
        }
    
    private:
        static Scope<PhysicsAPI> s_PhysicsAPI;
    };
}