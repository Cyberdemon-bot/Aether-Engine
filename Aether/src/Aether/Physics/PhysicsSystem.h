#pragma once
#include "Aether/Physics/PhysicsAPI.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/Assert.h"
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
        static void Init();
        static void Shutdown();

        static Handle<PhysicsInstance> CreateInstance();
        static void DestroyInstance(Handle<PhysicsInstance> handle);
        static void UpdateInstance(Handle<PhysicsInstance> handle, Timestep ts);

        static Handle<RigidBody> CreateBody(Handle<PhysicsInstance> world, const BodyConfig& config);
        static void DestroyBody(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        static Handle<CollisionCallback> RegisterCallback(Handle<PhysicsInstance> world, const CollisionCallbackRef& callback);
        static void RemoveCallback(Handle<PhysicsInstance> world, Handle<CollisionCallback> handle);

        static RaycastHit CastRay(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance);
        static std::vector<RaycastHit> CastRayAll(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance);

        static void SetActive(Handle<PhysicsInstance> world, Handle<RigidBody> handle, bool active);
        static void SetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle, UUID id);
        static UUID GetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        static void SetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& transform);
        static PhysTransform GetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

        static void AddForce(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& force);
        static void SetVelocity(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& velocity);
        static void SetGravity(Handle<PhysicsInstance> world, const glm::vec3& gravity);

        static bool CanMove(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& target);
        static const BodyConfig* GetBodyInfo(Handle<PhysicsInstance> world, Handle<RigidBody> handle);

    private:
        PhysicsSystem() = default;
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;
        PhysicsSystem(PhysicsSystem&&) = delete;
        PhysicsSystem& operator=(PhysicsSystem&&) = delete;

        static PhysicsSystem& GetInstance();
        PhysicsAPI* GetAPI(Handle<PhysicsInstance> world);

        std::vector<PhysicsWorldSlot> m_Worlds;
        std::vector<uint32_t> m_FreeList;
    };
}