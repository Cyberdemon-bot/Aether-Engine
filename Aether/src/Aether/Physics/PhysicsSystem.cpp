#include "aepch.h"
#include "Aether/Core/Log.h"
#include "Aether/Physics/PhysicsSystem.h"

namespace Aether {

    PhysicsAPI* PhysicsSystem::GetAPI(Handle<PhysicsInstance> world)
    {
        Scope<PhysicsAPI>* api = m_Worlds.GetResource(world);
        if (api == nullptr) return nullptr;
        return api->get();
    }

    void PhysicsSystem::Init()
    {
        m_Worlds.Init();
        AE_CORE_INFO("PhysicsSystem initialized");
    }

    void PhysicsSystem::Shutdown()
    {
        m_Worlds.Loop([](Scope<PhysicsAPI>& api)
        {
            if (api) api->Shutdown();
        });
        m_Worlds.Shutdown();
    }

    Handle<PhysicsInstance> PhysicsSystem::CreateInstance()
    {
        Handle<PhysicsInstance> handle = m_Worlds.CreateResource();
        Scope<PhysicsAPI>* api = m_Worlds.GetResource(handle);
        *api = PhysicsAPI::Create();
        (*api)->Init();
        return handle;
    }

    void PhysicsSystem::DestroyInstance(Handle<PhysicsInstance> handle)
    {
        PhysicsAPI* api = GetAPI(handle);
        if (api == nullptr) return;

        api->Shutdown();
        Scope<PhysicsAPI>* slot = m_Worlds.GetResource(handle);
        slot->reset();
        m_Worlds.DestroyResource(handle);
    }

    void PhysicsSystem::UpdateInstance(Handle<PhysicsInstance> handle, Timestep ts)
    {
        PhysicsAPI* api = GetAPI(handle);
        if (api == nullptr) return;
        api->OnUpdate(ts);
    }

    Handle<RigidBody> PhysicsSystem::CreateBody(Handle<PhysicsInstance> world, const BodyConfig& config)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return Handle<RigidBody>::MakeInvalid();
        return api->CreateBody(config);
    }

    void PhysicsSystem::DestroyBody(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->DestroyBody(handle);
    }

    Handle<CollisionCallback> PhysicsSystem::RegisterCallback(Handle<PhysicsInstance> world, const CollisionCallbackRef& callback)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return Handle<CollisionCallback>::MakeInvalid();
        return api->RegisterCallback(callback);
    }

    void PhysicsSystem::RemoveCallback(Handle<PhysicsInstance> world, Handle<CollisionCallback> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->RemoveCallback(handle);
    }

    RaycastResult PhysicsSystem::CastRay(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return {};
        return api->CastRay(origin, direction, distance);
    }

    std::vector<RaycastResult> PhysicsSystem::CastRayAll(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return {};
        return api->CastRayAll(origin, direction, distance);
    }

    void PhysicsSystem::SetActive(Handle<PhysicsInstance> world, Handle<RigidBody> handle, bool active)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetActive(handle, active);
    }

    void PhysicsSystem::SetUserData(Handle<PhysicsInstance> world, Handle<RigidBody> handle, uint64_t ud)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetUserData(handle, ud);
    }

    uint64_t PhysicsSystem::GetUserData(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return 0;
        return api->GetUserData(handle);
    }

    void PhysicsSystem::SetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& transform)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetPhysTransform(handle, transform);
    }

    PhysTransform PhysicsSystem::GetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return {};
        return api->GetPhysTransform(handle);
    }

    void PhysicsSystem::AddForce(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& force)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->AddForce(handle, force);
    }

    void PhysicsSystem::SetVelocity(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& velocity)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetVelocity(handle, velocity);
    }

    void PhysicsSystem::SetGravity(Handle<PhysicsInstance> world, const glm::vec3& gravity)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetGravity(gravity);
    }

    bool PhysicsSystem::CanMove(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& target)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return false;
        return api->CanMove(handle, target);
    }

    BodyConfig PhysicsSystem::GetBodyInfo(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return {};
        return api->GetBodyInfo(handle);
    }
}