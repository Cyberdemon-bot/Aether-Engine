#pragma once
#include "Aether/Physics/PhysicsAPI.h"
#include <vector>

namespace Aether {

    struct PhysicsInstance;
    struct PhysicsWorldSlot
    {
        Scope<PhysicsAPI> api = nullptr;
        bool active = false;
        uint32_t generation = 0;
    };

    class AETHER_API PhysicsSystem
    {
    public:
        void Init();
        void Shutdown();

        Handle<PhysicsInstance> CreateInstance();
        void DestroyInstance(Handle<PhysicsInstance> handle);
        void UpdateInstance(Handle<PhysicsInstance> handle, Timestep ts);

        Handle<RigidBody> CreateBody(Handle<PhysicsInstance> world, const BodyConfig& config);
        void DestroyBody(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        Handle<CollisionCallback> RegisterCallback(Handle<PhysicsInstance> world, const CollisionCallbackRef& callback);
        void RemoveCallback(Handle<PhysicsInstance> world, Handle<CollisionCallback> handle);

        RaycastHit CastRay(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance);
        std::vector<RaycastHit> CastRayAll(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance);

        void SetActive(Handle<PhysicsInstance> world, Handle<RigidBody> handle, bool active);
        void SetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle, UUID id);
        UUID GetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        void SetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& transform);
        PhysTransform GetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        void AddForce(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& force);
        void SetVelocity(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& velocity);
        void SetGravity(Handle<PhysicsInstance> world, const glm::vec3& gravity);

        bool CanMove(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& target);
        const BodyConfig* GetBodyInfo(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

    private:
        PhysicsAPI* GetAPI(Handle<PhysicsInstance> world);
        

        std::vector<PhysicsWorldSlot> m_Worlds;
        std::vector<uint32_t> m_FreeList;
    };
}