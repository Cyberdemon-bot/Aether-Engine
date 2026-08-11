#include "Aether/Physics/PhysicsAPI.h"
#include "Aether/Container/ResourcePool.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>


namespace JPH {
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    class BroadPhaseLayerInterface;
    class ObjectVsBroadPhaseLayerFilter;
    class ObjectLayerPairFilter;
    class Listener;
}

namespace Aether {
    struct JoltBodyData
    {
        JPH::BodyID joltID;
        BodyConfig bodyInfo;

        PhysTransform transform;
        bool IsDirty = false;
    };

    class Jolt_PhysicsAPI : public PhysicsAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update(Timestep ts) override;

        virtual Handle<RigidBody> CreateBody(const BodyConfig& config) override;
        virtual void DestroyBody(Handle<RigidBody> handle) override;

        virtual Handle<CollisionCallback> RegisterCallback(const CollisionCallbackRef& callback) override;
        virtual void RemoveCallback(Handle<CollisionCallback> handle) override;

        virtual BodyConfig GetBodyInfo(Handle<RigidBody> handle) const override;

        virtual RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual bool CanMove(Handle<RigidBody> handle, const PhysTransform& target) override;

        virtual void SetActive(Handle<RigidBody> handle, bool active) override;
        virtual void SetUserData(Handle<RigidBody> handle, uint64_t ud) override;
        virtual uint64_t GetUserData(Handle<RigidBody> handle) override;
        
        virtual void SetPhysTransform(Handle<RigidBody> handle, const PhysTransform& transform) override;
        virtual PhysTransform GetPhysTransform(Handle<RigidBody> handle) const override;

        virtual void AddForce(Handle<RigidBody> handle, const glm::vec3& force) override;
        virtual void SetVelocity(Handle<RigidBody> handle, const glm::vec3& velocity) override;
        virtual void SetGravity(const glm::vec3& gravity) override;
    private:
        std::vector<uint64_t> m_UDList;
        ResourcePool<Handle<RigidBody>, JoltBodyData> m_BodyPool;
        ResourcePool<Handle<CollisionCallback>, CollisionCallbackRef> m_CallbackPool;
        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::BroadPhaseLayerInterface* m_BPLayerInterface = nullptr;
        JPH::ObjectVsBroadPhaseLayerFilter* m_ObjVsBPFilter = nullptr;
        JPH::ObjectLayerPairFilter* m_ObjVsObjFilter = nullptr;
        JPH::Listener* m_ContactListener = nullptr;

        void ExcSetPhysTranform(JoltBodyData& data, const PhysTransform& transform);
    };
}