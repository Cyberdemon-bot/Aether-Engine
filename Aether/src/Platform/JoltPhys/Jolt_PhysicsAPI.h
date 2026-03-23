#include "Aether/Physics/PhysicsAPI.h"
#include <unordered_map>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace JPH {
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    class BroadPhaseLayerInterface;
    class ObjectVsBroadPhaseLayerFilter;
    class ObjectLayerPairFilter;
    class ContactListener;
}

namespace Aether {
    struct BodyData
    {
        JPH::BodyID joltID;
        BodyConfig bodyInfo;
    };

    struct BodySlot
    {
        BodyData data;
        int generation = 0;
    };

    class Jolt_PhysicsAPI : public PhysicsAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update(Timestep ts) override;

        virtual BodyHandle CreateBody(const BodyConfig& config) override;
        virtual void DestroyBody(BodyHandle handle) override;

        virtual const BodyConfig* GetBodyInfo(BodyHandle handle) const override;

        virtual RaycastHit CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual std::vector<RaycastHit> CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance) override;
        virtual bool CanMove(BodyHandle handle, const PhysTransform& target) override;

        virtual void SetActive(BodyHandle handle, bool active) override;
        virtual void SetUUID(BodyHandle handle, UUID id) override;
        virtual UUID GetUUID(BodyHandle handle) override;
        
        virtual void SetPhysTransform(BodyHandle handle, const PhysTransform& transform) override;
        virtual PhysTransform GetPhysTransform(BodyHandle handle) const override;

        virtual void AddForce(BodyHandle handle, const glm::vec3& force) override;
        virtual void SetVelocity(BodyHandle handle, const glm::vec3& velocity) override;
        virtual void SetGravity(const glm::vec3& gravity) override;
    private:
        std::vector<BodySlot> m_Bodies;
        std::vector<UUID> m_IDList;
        std::vector<uint32_t> FreeList;
        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::BroadPhaseLayerInterface* m_BPLayerInterface = nullptr;
        JPH::ObjectVsBroadPhaseLayerFilter* m_ObjVsBPFilter = nullptr;
        JPH::ObjectLayerPairFilter* m_ObjVsObjFilter = nullptr;
        JPH::ContactListener* m_ContactListener = nullptr;
    };
}