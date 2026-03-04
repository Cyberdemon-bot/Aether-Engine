#include "Platform/JoltPhys/Jolt_PhysicsAPI.h"
#include "aepch.h"

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>  
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

namespace JPH {
    namespace Layers {
        static constexpr JPH::ObjectLayer STATIC  = 0;
        static constexpr JPH::ObjectLayer DYNAMIC = 1;
        static constexpr JPH::uint NUM_LAYERS     = 2;
    }

    namespace BPLayers {
        static constexpr JPH::BroadPhaseLayer STATIC  { 0 };
        static constexpr JPH::BroadPhaseLayer DYNAMIC { 1 };
        static constexpr JPH::uint NUM_LAYERS          = 2;
    }

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        JPH::uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
        {
            return layer == Layers::STATIC ? BPLayers::STATIC : BPLayers::DYNAMIC;
        }
    #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
        { return layer == BPLayers::STATIC ? "STATIC" : "DYNAMIC"; }
    #endif
    };

    class ObjVsBPFilter final : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer obj, JPH::BroadPhaseLayer bp) const override
        {
            if (obj == Layers::STATIC)  return bp == BPLayers::DYNAMIC;
            if (obj == Layers::DYNAMIC) return true;
            return false;
        }
    };

    class ObjVsObjFilter final : public ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
        {
            if (a == Layers::STATIC)  return b == Layers::DYNAMIC;
            if (a == Layers::DYNAMIC) return true;
            return false;
        }
    };
}

namespace Aether {
    
    void Jolt_PhysicsAPI::Init()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();   

        m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10MB
        m_JobSystem     = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2);

        const JPH::uint maxBodies       = 1024;
        const JPH::uint numBodyMutexes = 0;
        const JPH::uint maxBodyPairs    = 1024;
        const JPH::uint maxContactConstraints = 1024;

        m_BPLayerInterface = new JPH::BPLayerInterfaceImpl();
        m_ObjVsBPFilter = new JPH::ObjVsBPFilter();
        m_ObjVsObjFilter = new JPH::ObjVsObjFilter();

        m_PhysicsSystem = new JPH::PhysicsSystem();
        m_PhysicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, *m_BPLayerInterface, *m_ObjVsBPFilter, *m_ObjVsObjFilter);
        m_PhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    }

    void Jolt_PhysicsAPI::Shutdown()
    {
        if (m_PhysicsSystem) 
        {
            delete m_PhysicsSystem;
            m_PhysicsSystem = nullptr;
        }

        if (m_ObjVsObjFilter) 
        {
            delete m_ObjVsObjFilter;
            m_ObjVsObjFilter = nullptr;
        }

        if (m_ObjVsBPFilter)
        {
            delete m_ObjVsBPFilter;
            m_ObjVsBPFilter = nullptr;
        }

        if (m_BPLayerInterface)
        {
            delete m_BPLayerInterface;
            m_BPLayerInterface = nullptr;
        }

        if (m_JobSystem)
        {
            delete m_JobSystem;
            m_JobSystem = nullptr;
        }

        if (m_TempAllocator)
        {
            delete m_TempAllocator;
            m_TempAllocator = nullptr;
        }

        JPH::UnregisterTypes();

        if (JPH::Factory::sInstance)
        {
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }

        m_Bodies.clear();
    }

    void Jolt_PhysicsAPI::Update(Timestep ts)
    {
        if (!m_PhysicsSystem) return;

        float deltaTime = ts.GetSeconds();
        if (deltaTime > 1.0f / 30.0f) deltaTime = 1.0f / 30.0f;
        const int CollisionSteps = 1;
        m_PhysicsSystem->Update(deltaTime, CollisionSteps, m_TempAllocator, m_JobSystem);
    }

    void Jolt_PhysicsAPI::CreateBody(UUID bodyID, const BodyConfig& config)
    {
        if (!m_PhysicsSystem) return;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        JPH::ShapeRefC shape;
        JPH::ShapeSettings::ShapeResult result;
        switch (config.shape)
        {
            case ColliderShape::Box:
            {
                JPH::BoxShapeSettings box(JPH::Vec3(config.size.x, config.size.y, config.size.z));
                result = box.Create();
                break;
            }
            case ColliderShape::Sphere:
            {
                JPH::SphereShapeSettings sphere(config.size.x);
                result = sphere.Create();
                break;
            }
            case ColliderShape::Capsule:
            {
                float radius = config.size.x;
                float height = config.size.y;

                float half_height = (height/ 2.0f) - radius;
                half_height = std::max(half_height, 0.0f);

                JPH::CapsuleShapeSettings capsule(half_height, radius);
                result = capsule.Create();
                break;
            }
            case ColliderShape::None:
            {
                AE_CORE_ERROR("Cannot create body with no shape");
                return;
            }
        }
        if (result.HasError())
        {
            AE_CORE_ERROR("Jolt Physics Error: {0}", result.GetError().c_str());
            return;
        }

        shape = result.Get();

        if (!shape) 
        {
            AE_CORE_ERROR("Fail to identify shape for body {0}", (uint64_t)bodyID);
            return;
        }

        auto& trans = config.transform.translation;
        auto& rot   = config.transform.rotation;
        auto& offset = config.offset; 

        glm::quat entityRot(rot.w, rot.x, rot.y, rot.z);
        glm::vec3 worldOffset = entityRot * offset;
        glm::vec3 finalPos = trans + worldOffset;

        JPH::Vec3 translation(finalPos.x, finalPos.y, finalPos.z);
        JPH::Quat rotation(rot.x, rot.y, rot.z, rot.w);

        JPH::EMotionType motionType;
        JPH::ObjectLayer objectLayer;
        switch (config.motionType)
        {
            case MotionType::Static:
            {
                motionType = JPH::EMotionType::Static;
                objectLayer = JPH::Layers::STATIC;
                break;
            }
            case MotionType::Dynamic:
            {
                motionType = JPH::EMotionType::Dynamic;
                objectLayer = JPH::Layers::DYNAMIC;
                break;
            }
            case MotionType::Kinematic:
            {
                motionType = JPH::EMotionType::Kinematic;
                objectLayer = JPH::Layers::DYNAMIC; 
                break;
            }
            case MotionType::None:
            {
                AE_CORE_ERROR("Cannot create body with no motion type");
                return;
            }
        }
        JPH::BodyCreationSettings bodyInfo(shape, translation, rotation, motionType, objectLayer);

        bodyInfo.mRestitution = config.restitution;
        bodyInfo.mFriction = config.friction;
        bodyInfo.mIsSensor = config.isSensor;
        bodyInfo.mUserData = static_cast<uint64_t>(bodyID);

        if (motionType == JPH::EMotionType::Dynamic) 
        {
            bodyInfo.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        }
        JPH::Body* body = bodyInterface.CreateBody(bodyInfo);
        if (!body)
        {
            AE_CORE_ERROR("Fail to create body {0}", (uint64_t)bodyID);
            return;
        }
        m_Bodies[bodyID] = {body->GetID(), config.motionType, config.shape};
        bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
    }

    MotionType Jolt_PhysicsAPI::GetMotionType(UUID bodyID)
    {
        if (!m_PhysicsSystem) return MotionType::None;
        if (m_Bodies.find(bodyID) == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return MotionType::None;
        }

        return m_Bodies[bodyID].motionType;
    }
    
    ColliderShape Jolt_PhysicsAPI::GetColliderShape(UUID bodyID)
    {
        if (!m_PhysicsSystem) return ColliderShape::None;
        if (m_Bodies.find(bodyID) == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return ColliderShape::None;
        }

        return m_Bodies[bodyID].Shape;
    }

    RaycastHit Jolt_PhysicsAPI::CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        RaycastHit result;
        const JPH::NarrowPhaseQuery& query = m_PhysicsSystem->GetNarrowPhaseQuery();
        glm::vec3 rayDir = glm::normalize(direction) * distance;

        JPH::RVec3 joltOrigin(origin.x, origin.y, origin.z);
        JPH::Vec3 joltDir(rayDir.x, rayDir.y, rayDir.z);

        JPH::RRayCast ray{ joltOrigin, joltDir };
        JPH::RayCastResult hit; 

        if (query.CastRay(ray, hit))
        {
            result.Hit = true;
            result.Distance = distance * hit.mFraction;
            result.Position = origin + (glm::normalize(direction) * result.Distance);

            JPH::BodyID bodyID = hit.mBodyID;

            JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();
                result.HitEntityID = static_cast<UUID>(body.GetUserData()); 

                JPH::RVec3 joltHitPos(result.Position.x, result.Position.y, result.Position.z);
                JPH::Vec3 joltNormal = body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, joltHitPos);
                result.Normal = glm::vec3(joltNormal.GetX(), joltNormal.GetY(), joltNormal.GetZ());
            }
        }

        return result;
    }

    std::vector<RaycastHit> Jolt_PhysicsAPI::CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        std::vector<RaycastHit> results;
        const JPH::NarrowPhaseQuery& query = m_PhysicsSystem->GetNarrowPhaseQuery();
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        glm::vec3 rayDir = glm::normalize(direction) * distance;

        JPH::RVec3 joltOrigin(origin.x, origin.y, origin.z);
        JPH::Vec3 joltDir(rayDir.x, rayDir.y, rayDir.z);
        JPH::RRayCast ray{ joltOrigin, joltDir };

        JPH::RayCastSettings settings;
        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        query.CastRay(ray, settings, collector);
        for (const JPH::RayCastResult& hit : collector.mHits)
        {
            RaycastHit res;
            res.Hit = true;
            res.Distance = distance * hit.mFraction; 
            res.Position = origin + (glm::normalize(direction) * res.Distance);
            JPH::BodyID bodyID = hit.mBodyID;

            JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();
                res.HitEntityID = static_cast<UUID>(body.GetUserData()); 
                JPH::RVec3 joltHitPos(res.Position.x, res.Position.y, res.Position.z);
                JPH::Vec3 joltNormal = body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, joltHitPos);
                res.Normal = glm::vec3(joltNormal.GetX(), joltNormal.GetY(), joltNormal.GetZ());
            }
            results.push_back(res);
        }

        std::sort(results.begin(), results.end(), [](const RaycastHit& a, const RaycastHit& b) {return a.Distance < b.Distance;});

        return results;
    }
    
    void Jolt_PhysicsAPI::SetActive(UUID bodyID, bool active)
    {
        if (!m_PhysicsSystem) return;
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return;
        }

        if (it->second.motionType == MotionType::Static) return;

        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        if (active) bodyInterface.ActivateBody(id);
        else bodyInterface.DeactivateBody(id);
    }

    void Jolt_PhysicsAPI::DestroyBody(UUID bodyID)
    {
        if (!m_PhysicsSystem) return;   
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return;
        }

        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
        m_Bodies.erase(it);
    }

    void Jolt_PhysicsAPI::SetPhysTransform(UUID bodyID, const PhysTransform& transform)
    {
        if (!m_PhysicsSystem) return;
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return;
        }
        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 pos(transform.translation.x, transform.translation.y, transform.translation.z);
        JPH::Quat rot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

        bodyInterface.SetPositionAndRotation(id, pos, rot, JPH::EActivation::Activate);
    }

    PhysTransform Jolt_PhysicsAPI::GetPhysTransform(UUID bodyID) const
    {
        PhysTransform transform;
        if (!m_PhysicsSystem) return transform;
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return transform;
        }
        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 pos = bodyInterface.GetPosition(id);
        JPH::Quat rot = bodyInterface.GetRotation(id);

        transform.translation = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
        transform.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());

        return transform;
    }

    void Jolt_PhysicsAPI::AddForce(UUID bodyID, const glm::vec3& force)
    {
        if (!m_PhysicsSystem) return;
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return;
        }
        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltForce(force.x, force.y, force.z);
        bodyInterface.AddForce(id, joltForce, JPH::EActivation::Activate);
    }

    void Jolt_PhysicsAPI::SetVelocity(UUID bodyID, const glm::vec3& velocity)
    {
        if (!m_PhysicsSystem) return;
        auto it = m_Bodies.find(bodyID);
        if (it == m_Bodies.end())
        {
            AE_CORE_ERROR("Body ID {0} is not exits", (uint64_t)bodyID);
            return;
        }
        JPH::BodyID id = it->second.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltVel(velocity.x, velocity.y, velocity.z);
        bodyInterface.SetLinearVelocity(id, joltVel);
    }

    void Jolt_PhysicsAPI::SetGravity(const glm::vec3& gravity)
    {
        m_PhysicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
    }
}