#include "aepch.h"
#include "Platform/JoltPhys/Jolt_PhysicsAPI.h"
#include "Aether/Container/MSPCQueue.h"
#include <Jolt/Core/Memory.h>
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
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

//#define JPH_ENABLE_ASSERTS

namespace JPH {
    namespace Layers {
        static constexpr ObjectLayer STATIC  = 0;
        static constexpr ObjectLayer DYNAMIC = 1;
        static constexpr uint NUM_LAYERS     = 2;
    }

    namespace BPLayers {
        static constexpr BroadPhaseLayer STATIC  { 0 };
        static constexpr BroadPhaseLayer DYNAMIC { 1 };
        static constexpr uint NUM_LAYERS          = 2;
    }

    class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
    {
    public:
        uint GetNumBroadPhaseLayers() const override { return BPLayers::NUM_LAYERS; }
        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer layer) const override
        {
            return layer == Layers::STATIC ? BPLayers::STATIC : BPLayers::DYNAMIC;
        }
    #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(BroadPhaseLayer layer) const override
        { return layer == BPLayers::STATIC ? "STATIC" : "DYNAMIC"; }
    #endif
    };

    class ObjVsBPFilter final : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        bool ShouldCollide(ObjectLayer obj, BroadPhaseLayer bp) const override
        {
            if (obj == Layers::STATIC)  return bp == BPLayers::DYNAMIC;
            if (obj == Layers::DYNAMIC) return true;
            return false;
        }
    };

    class ObjVsObjFilter final : public ObjectLayerPairFilter
    {
    public:
        bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override
        {
            if (a == Layers::STATIC)  return b == Layers::DYNAMIC;
            if (a == Layers::DYNAMIC) return true;
            return false;
        }
    };

    struct InternalCollisionEvent
    {
        Aether::CollisionType type;
        JPH::BodyID bodyA;
        JPH::BodyID bodyB;
        glm::vec3 contactPoint;
        glm::vec3 contactNormal;
    };
    class Listener final : public ContactListener 
    {
    public:
        Listener(PhysicsSystem* system)
            : m_System(system)
        {
        }

        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override 
        {
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override 
        {
            JPH::RVec3 joltContactPoint = inManifold.GetWorldSpaceContactPointOn1(0);
            JPH::Vec3 joltNormal = inManifold.mWorldSpaceNormal;

            InternalCollisionEvent ev;
            ev.type = Aether::CollisionType::Enter;
            ev.bodyA = inBody1.GetID();     
            ev.bodyB = inBody2.GetID();
            ev.contactPoint = glm::vec3(joltContactPoint.GetX(), joltContactPoint.GetY(), joltContactPoint.GetZ());
            ev.contactNormal = glm::vec3(joltNormal.GetX(), joltNormal.GetY(), joltNormal.GetZ());

            bool pushed = m_EventQueue.Push(std::move(ev));
            if (!pushed) AE_CORE_WARN("Collision event queue full, dropping event");
        }

        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override 
        {
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
        {
            InternalCollisionEvent ev;
            ev.type = Aether::CollisionType::Exit;
            ev.bodyA = inSubShapePair.GetBody1ID(); 
            ev.bodyB = inSubShapePair.GetBody2ID();

            if (!m_EventQueue.Push(std::move(ev))) AE_CORE_WARN("Collision event queue full, dropping exit event");
        }

        void ProcessEvents(const Aether::CollisionCallbackRef& callback) 
        {
            if (!m_System) return;
            const JPH::BodyLockInterface& lockInterface = m_System->GetBodyLockInterface();

            m_EventQueue.Drain([this, &callback, &lockInterface](InternalCollisionEvent&& internalEv) 
            {
                Aether::CollisionEvent ev;
                ev.type = internalEv.type;
                ev.bodyA = get_handle(internalEv.bodyA, lockInterface);
                ev.bodyB = get_handle(internalEv.bodyB, lockInterface);
                ev.contactPoint = internalEv.contactPoint;
                ev.contactNormal = internalEv.contactNormal;
                callback(ev);
            });
        }
    private:
        Aether::Handle<Aether::RigidBody> get_handle(JPH::BodyID id, const JPH::BodyLockInterface& lockInterface)
        {
            JPH::BodyLockRead lock(lockInterface, id);
            if (!lock.Succeeded()) return Aether::Handle<Aether::RigidBody>::MakeInvalid();
            const JPH::Body& body = lock.GetBody();
            return Aether::Handle<Aether::RigidBody>::FromBlend(body.GetUserData());
        }

        Aether::MSPCQueue<InternalCollisionEvent, 1024> m_EventQueue;
        PhysicsSystem* m_System = nullptr;
    };
}

namespace Aether {

    static void JoltTraceImpl(const char* inFMT, ...) 
    {
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);
        AE_CORE_TRACE("JOLT TRACE: {0}", buffer);
    }

    #ifdef JPH_ENABLE_ASSERTS

    static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine) 
    {
        std::cout << "JOLT ASSERT CRASH: " << inFile << ":" << inLine 
                << " (" << inExpression << ") " << (inMessage ? inMessage : "") << std::endl;
        return true; 
    }
    #endif
    
    void Jolt_PhysicsAPI::Init()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Trace = JoltTraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();   

        const JPH::uint maxBodies = 1024;
        const JPH::uint numBodyMutexes = 0;
        const JPH::uint maxBodyPairs = 1024;
        const JPH::uint maxContactConstraints = 1024;

        m_TempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10MB
        m_JobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2);

        m_PhysicsSystem = new JPH::PhysicsSystem();

        m_BPLayerInterface = new JPH::BPLayerInterfaceImpl();
        m_ObjVsBPFilter = new JPH::ObjVsBPFilter();
        m_ObjVsObjFilter = new JPH::ObjVsObjFilter();
        m_ContactListener = new JPH::Listener(m_PhysicsSystem);

        m_PhysicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, *m_BPLayerInterface, *m_ObjVsBPFilter, *m_ObjVsObjFilter);
        m_PhysicsSystem->SetContactListener(m_ContactListener);
        m_PhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

        m_IDList.reserve(32);
        m_BodyPool.Init();
        m_CallbackPool.Init();
    }

    void Jolt_PhysicsAPI::Shutdown()
    {
        if (m_PhysicsSystem) 
        {
            m_PhysicsSystem->SetContactListener(nullptr);
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

        if (m_ContactListener)
        {
            delete m_ContactListener;
            m_ContactListener = nullptr;
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

        m_IDList.clear();
        m_BodyPool.Shutdown();
        m_CallbackPool.Shutdown();
    }

    void Jolt_PhysicsAPI::Update(Timestep ts)
    {
        if (!m_PhysicsSystem) return;

        float deltaTime = ts.GetSeconds();
        if (deltaTime > 1.0f / 30.0f) deltaTime = 1.0f / 30.0f;
        const int CollisionSteps = 1;

        m_BodyPool.Loop([this](JoltBodyData& data)
        {
            if (data.IsDirty)
            {
                this->ExcSetPhysTranform(data, data.transform);
                data.IsDirty = false;
            }
        });

        m_PhysicsSystem->Update(deltaTime, CollisionSteps, m_TempAllocator, m_JobSystem);
        m_CallbackPool.Loop([this](const CollisionCallbackRef& callback) 
        {
            this->m_ContactListener->ProcessEvents(callback);
        });
    }

    Handle<RigidBody> Jolt_PhysicsAPI::CreateBody(const BodyConfig& config)
    {
        if (!m_PhysicsSystem) return Handle<RigidBody>::MakeInvalid();
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
                return Handle<RigidBody>::MakeInvalid();
            }
        }
        if (result.HasError())
        {
            AE_CORE_ERROR("Jolt Physics Error: {0}", result.GetError().c_str());
            return Handle<RigidBody>::MakeInvalid();
        }

        shape = result.Get();

        if (!shape) 
        {
            AE_CORE_ERROR("Fail to identify shape for body");
            return Handle<RigidBody>::MakeInvalid();
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
                return Handle<RigidBody>::MakeInvalid();
            }
        }
        JPH::BodyCreationSettings bodyInfo(shape, translation, rotation, motionType, objectLayer);

        bodyInfo.mRestitution = config.restitution;
        bodyInfo.mFriction = config.friction;
        bodyInfo.mIsSensor = config.isSensor;

        if (motionType == JPH::EMotionType::Dynamic) 
        {
            bodyInfo.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        }
        JPH::Body* body = bodyInterface.CreateBody(bodyInfo);
        if (!body)
        {
            AE_CORE_ERROR("Fail to create body");
            return Handle<RigidBody>::MakeInvalid();
        }
        bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);

        auto handle = m_BodyPool.CreateResource();
        m_BodyPool.GetResource(handle)->joltID = body->GetID();
        m_BodyPool.GetResource(handle)->bodyInfo = config;
        body->SetUserData(handle.Blend());
        m_IDList.resize(m_BodyPool.GetLen());
        m_IDList[handle.index] = UUID(0);
        return handle;
    }

    void Jolt_PhysicsAPI::DestroyBody(Handle<RigidBody> handle)
    {
        if (!m_PhysicsSystem) return;   
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return;

        JPH::BodyID id = data->joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
        m_BodyPool.DestroyResource(handle);
    }

    Handle<CollisionCallback> Jolt_PhysicsAPI::RegisterCallback(const CollisionCallbackRef& callback)
    {
        if (!m_PhysicsSystem) return Handle<CollisionCallback>::MakeInvalid();   
        return m_CallbackPool.SaveResource(callback);
    }

    void Jolt_PhysicsAPI::RemoveCallback(Handle<CollisionCallback> handle) 
    {
        if (!m_PhysicsSystem) return; 
        m_CallbackPool.DestroyResource(handle);
    }

    const BodyConfig* Jolt_PhysicsAPI::GetBodyInfo(Handle<RigidBody> handle) const
    {
        if (!m_PhysicsSystem) return nullptr;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return nullptr;
        return &data->bodyInfo;
    }

    RaycastHit Jolt_PhysicsAPI::CastRay(const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        RaycastHit result;
        if (!m_PhysicsSystem) return result;

        const JPH::NarrowPhaseQuery& query = m_PhysicsSystem->GetNarrowPhaseQuery();
        glm::vec3 normDir = glm::normalize(direction);
        glm::vec3 rayDir = normDir * distance;

        JPH::RVec3 joltOrigin(origin.x, origin.y, origin.z);
        JPH::Vec3 joltDir(rayDir.x, rayDir.y, rayDir.z);

        JPH::RRayCast ray{ joltOrigin, joltDir };
        JPH::RayCastResult hit; 

        if (query.CastRay(ray, hit))
        {
            result.Hit = true;
            result.Position = origin + (normDir * (distance * hit.mFraction));
            result.Distance = glm::distance(origin, result.Position);

            JPH::BodyID bodyID = hit.mBodyID;
            JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();
                uint64_t id = body.GetUserData();
                result.HitEntityHandle = Handle<RigidBody>::FromBlend(id);
                result.HitEntityID = GetUUID(result.HitEntityHandle);

                JPH::RVec3 joltHitPos(result.Position.x, result.Position.y, result.Position.z);
                JPH::Vec3 localHitPos = body.GetInverseCenterOfMassTransform() * joltHitPos;
                JPH::Vec3 localNormal = body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, localHitPos);
                JPH::Vec3 worldNormal = body.GetRotation() * localNormal;
                
                result.Normal = glm::vec3(worldNormal.GetX(), worldNormal.GetY(), worldNormal.GetZ());
            }
        }

        return result;
    }

    std::vector<RaycastHit> Jolt_PhysicsAPI::CastRayAll(const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        std::vector<RaycastHit> results;
        if (!m_PhysicsSystem) return results;

        const JPH::NarrowPhaseQuery& query = m_PhysicsSystem->GetNarrowPhaseQuery();
        glm::vec3 normDir = glm::normalize(direction);
        glm::vec3 rayDir = normDir * distance;

        JPH::RVec3 joltOrigin(origin.x, origin.y, origin.z);
        JPH::Vec3 joltDir(rayDir.x, rayDir.y, rayDir.z);
        JPH::RRayCast ray{ joltOrigin, joltDir };

        JPH::RayCastSettings settings;
        JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
        query.CastRay(ray, settings, collector);
        
        results.reserve(collector.mHits.size());

        for (const JPH::RayCastResult& hit : collector.mHits)
        {
            RaycastHit res;
            res.Hit = true;
            res.Position = origin + (normDir * (distance * hit.mFraction));
            res.Distance = glm::distance(origin, res.Position);

            JPH::BodyID bodyID = hit.mBodyID;
            JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body = lock.GetBody();
                uint64_t id = body.GetUserData();
                res.HitEntityHandle = Handle<RigidBody>::FromBlend(id);
                res.HitEntityID = GetUUID(res.HitEntityHandle);
                
                JPH::RVec3 joltHitPos(res.Position.x, res.Position.y, res.Position.z);
                JPH::Vec3 localHitPos = body.GetInverseCenterOfMassTransform() * joltHitPos;
                JPH::Vec3 localNormal = body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, localHitPos);
                JPH::Vec3 worldNormal = body.GetRotation() * localNormal;

                res.Normal = glm::vec3(worldNormal.GetX(), worldNormal.GetY(), worldNormal.GetZ());
            }
            results.push_back(res);
        }

        std::sort(results.begin(), results.end(), [](const RaycastHit& a, const RaycastHit& b) 
        {
            return a.Distance < b.Distance;
        });

        return results;
    }

    bool Jolt_PhysicsAPI::CanMove(Handle<RigidBody> handle, const PhysTransform& target)
    {
        if (!m_PhysicsSystem) return false;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return false;
        JPH::BodyID joltID = data->joltID;

        JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), joltID);
        if (!lock.Succeeded()) return false;

        const JPH::Body& body = lock.GetBody();
        JPH::Vec3 currentPos  = body.GetPosition();
        JPH::Quat currentRot  = body.GetRotation();
        const JPH::Shape* shape = body.GetShape();

        lock.ReleaseLock(); 

        JPH::Vec3 targetPos(target.translation.x, target.translation.y, target.translation.z);
        JPH::Quat targetRot(target.rotation.x, target.rotation.y, target.rotation.z, target.rotation.w);
        JPH::Vec3 displacement = targetPos - currentPos;

        JPH::RShapeCast shapeCast(
            shape,
            JPH::Vec3::sReplicate(1.0f),                                   
            JPH::RMat44::sRotationTranslation(currentRot, currentPos),     
            displacement                                                     
        );

        JPH::ShapeCastSettings settings;
        settings.mReturnDeepestPoint = false;

        class IgnoreSelf : public JPH::BodyFilter {
        public:
            JPH::BodyID selfID;
            bool ShouldCollide(const JPH::BodyID& id) const override { return id != selfID; }
            bool ShouldCollideLocked(const JPH::Body&) const override { return true; }
        } bodyFilter;
        bodyFilter.selfID = joltID;

        JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;

        m_PhysicsSystem->GetNarrowPhaseQuery().CastShape(
            shapeCast, settings, JPH::RVec3::sZero(), collector,
            JPH::BroadPhaseLayerFilter{},
            JPH::ObjectLayerFilter{},
            bodyFilter
        );

        return !collector.HadHit();
    }
    
    void Jolt_PhysicsAPI::SetActive(Handle<RigidBody> handle, bool active)
    {
        if (!m_PhysicsSystem) return;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return;

        if (data->bodyInfo.motionType == MotionType::Static) return;

        JPH::BodyID id = data->joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();
        if (active) bodyInterface.ActivateBody(id);
        else bodyInterface.DeactivateBody(id);
    }

    void Jolt_PhysicsAPI::SetUUID(Handle<RigidBody> handle, UUID id)
    {
        if (!m_PhysicsSystem) return;
        if (!m_BodyPool.GetResource(handle)) return;
        m_IDList[handle.index] = id;
    }


    UUID Jolt_PhysicsAPI::GetUUID(Handle<RigidBody> handle) 
    {
        if (!m_PhysicsSystem) return 0;
        if (!m_BodyPool.GetResource(handle)) return 0;
        return m_IDList[handle.index];
    }

    void Jolt_PhysicsAPI::SetPhysTransform(Handle<RigidBody> handle, const PhysTransform& transform)
    {
        if (!m_PhysicsSystem) return;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return;

        data->transform = transform;
        data->IsDirty = true;
    }

    void Jolt_PhysicsAPI::ExcSetPhysTranform(JoltBodyData& data, const PhysTransform& transform)
    {
        if (!m_PhysicsSystem) return;

        JPH::BodyID id = data.joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 pos(transform.translation.x, transform.translation.y, transform.translation.z);
        JPH::Quat rot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

        bodyInterface.SetPositionAndRotation(id, pos, rot, JPH::EActivation::Activate);
    }

    PhysTransform Jolt_PhysicsAPI::GetPhysTransform(Handle<RigidBody> handle) const
    {
        PhysTransform transform{};
        if (!m_PhysicsSystem) return transform;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return {};

        JPH::BodyID id = data->joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 pos = bodyInterface.GetPosition(id);
        JPH::Quat rot = bodyInterface.GetRotation(id);

        transform.translation = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
        transform.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());

        return transform;
    }

    void Jolt_PhysicsAPI::AddForce(Handle<RigidBody> handle, const glm::vec3& force)
    {
        if (!m_PhysicsSystem) return;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return;

        JPH::BodyID id = data->joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltForce(force.x, force.y, force.z);
        bodyInterface.AddForce(id, joltForce, JPH::EActivation::Activate);
    }

    void Jolt_PhysicsAPI::SetVelocity(Handle<RigidBody> handle, const glm::vec3& velocity)
    {
        if (!m_PhysicsSystem) return;
        auto data = m_BodyPool.GetResource(handle);
        if (!data) return;

        JPH::BodyID id = data->joltID;
        JPH::BodyInterface& bodyInterface = m_PhysicsSystem->GetBodyInterface();

        JPH::Vec3 joltVel(velocity.x, velocity.y, velocity.z);
        bodyInterface.SetLinearVelocity(id, joltVel);
    }

    void Jolt_PhysicsAPI::SetGravity(const glm::vec3& gravity)
    {
        m_PhysicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
    }
}