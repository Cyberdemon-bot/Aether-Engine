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
        MotionType motionType;
        ColliderShape Shape;
    };

    class Jolt_PhysicsAPI : public PhysicsAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update(Timestep ts) override;

        virtual void CreateBody(UUID bodyID, const BodyConfig& config) override;
        virtual void DestroyBody(UUID bodyID) override;

        virtual MotionType GetMotionType(UUID bodyID) override;
        virtual ColliderShape GetColliderShape(UUID bodyID) override;

        virtual void SetActive(UUID bodyID, bool active) override;
        
        virtual void SetPhysTransform(UUID bodyID, const PhysTransform& transform) override;
        virtual PhysTransform GetPhysTransform(UUID bodyID) const override;

        virtual void AddForce(UUID bodyID, const glm::vec3& force) override;
        virtual void SetVelocity(UUID bodyID, const glm::vec3& velocity) override;
    private:
        std::unordered_map<UUID, BodyData> m_Bodies;
        JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
        JPH::TempAllocatorImpl* m_TempAllocator = nullptr;
        JPH::JobSystemThreadPool* m_JobSystem = nullptr;
        JPH::BroadPhaseLayerInterface* m_BPLayerInterface = nullptr;
        JPH::ObjectVsBroadPhaseLayerFilter* m_ObjVsBPFilter = nullptr;
        JPH::ObjectLayerPairFilter* m_ObjVsObjFilter = nullptr;
        JPH::ContactListener* m_ContactListener = nullptr;
    };
}