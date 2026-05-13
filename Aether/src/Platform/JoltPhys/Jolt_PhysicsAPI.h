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
    };

    class Jolt_PhysicsAPI : public PhysicsAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update(Timestep ts) override;

        virtual Handle<BodyTag> CreateBody(const BodyConfig& config) override;
        virtual void DestroyBody(Handle<BodyTag> handle) override;

        virtual Handle<CallbackTag> RegisterCallback(const CollisionCallbackRef& callback) override;
        virtual void RemoveCallback(Handle<CallbackTag> handle) override;

        virtual const BodyConfig* GetBodyInfo(Handle<BodyTag> handle) const override;

        virtual RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual bool CanMove(Handle<BodyTag> handle, const PhysTransform& target) override;

        virtual void SetActive(Handle<BodyTag> handle, bool active) override;
        virtual void SetUUID(Handle<BodyTag> handle, UUID id) override;
        virtual UUID GetUUID(Handle<BodyTag> handle) override;
        
        virtual void SetPhysTransform(Handle<BodyTag> handle, const PhysTransform& transform) override;
        virtual PhysTransform GetPhysTransform(Handle<BodyTag> handle) const override;

        virtual void AddForce(Handle<BodyTag> handle, const glm::vec3& force) override;
        virtual void SetVelocity(Handle<BodyTag> handle, const glm::vec3& velocity) override;
        virtual void SetGravity(const glm::vec3& gravity) override;
    private:
        std::vector<UUID> m_IDList;
        ResourcePool<Handle<BodyTag>, JoltBodyData> m_BodyPool;
        ResourcePool<Handle<CallbackTag>, CollisionCallbackRef> m_CallbackPool;
        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::BroadPhaseLayerInterface* m_BPLayerInterface = nullptr;
        JPH::ObjectVsBroadPhaseLayerFilter* m_ObjVsBPFilter = nullptr;
        JPH::ObjectLayerPairFilter* m_ObjVsObjFilter = nullptr;
        JPH::Listener* m_ContactListener = nullptr;
    };
}