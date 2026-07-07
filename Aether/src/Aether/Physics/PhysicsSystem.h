#pragma once
#include "Aether/Physics/PhysicsAPI.h"
#include "Aether/Container/ResourcePool.h"
#include <vector>

namespace Aether {

    struct PhysicsInstance;

    class AETHER_API PhysicsSystem
    {
    public:
        PhysicsSystem() = default;

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
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;
        PhysicsSystem(PhysicsSystem&&) = default;
        PhysicsSystem& operator=(PhysicsSystem&&) = default;

        PhysicsAPI* GetAPI(Handle<PhysicsInstance> world);

        ResourcePool<Handle<PhysicsInstance>, Scope<PhysicsAPI>> m_Worlds;
    };
}