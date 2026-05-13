#include "aepch.h"
#include "Aether/Physics/PhysicsSystem.h"

namespace Aether {

    PhysicsSystem& PhysicsSystem::GetInstance()
    {
        static PhysicsSystem instance;
        return instance;
    }

    PhysicsAPI* PhysicsSystem::GetAPI(Handle<PhysicsInstance> world)
    {
        auto& instance = GetInstance();
        if (!world.IsValid()) return nullptr;
        if (world.index >= instance.m_Worlds.size()) return nullptr;
        PhysicsWorldSlot& slot = instance.m_Worlds[world.index];
        if (slot.generation != world.generation) return nullptr;
        return slot.api.get();
    }

    void PhysicsSystem::Init()
    {
        auto& instance = GetInstance();
        instance.m_Worlds.reserve(8);
        AE_CORE_INFO("PhysicsSystem initialized");
    }

    void PhysicsSystem::Shutdown()
    {
        auto& instance = GetInstance();
        for (auto& slot : instance.m_Worlds)
        {
            if (slot.active && slot.api)
                slot.api->Shutdown();
        }
        instance.m_Worlds.clear();
        instance.m_FreeList.clear();
    }

    Handle<PhysicsInstance> PhysicsSystem::CreateInstance()
    {
        auto& instance = GetInstance();
        uint32_t index;

        if (!instance.m_FreeList.empty())
        {
            index = instance.m_FreeList.back();
            instance.m_FreeList.pop_back();
        }
        else
        {
            index = (uint32_t)instance.m_Worlds.size();
            instance.m_Worlds.emplace_back();
        }

        PhysicsWorldSlot& slot = instance.m_Worlds[index];
        slot.api = std::move(PhysicsAPI::Create());
        slot.api->Init();
        slot.active = true;

        Handle<PhysicsInstance> handle;
        handle.index = index;
        handle.generation = slot.generation;
        return handle;
    }

    void PhysicsSystem::DestroyInstance(Handle<PhysicsInstance> handle)
    {
        auto& instance = GetInstance();
        PhysicsAPI* api = instance.GetAPI(handle);
        if (api == nullptr) return;

        api->Shutdown();
        PhysicsWorldSlot& slot = instance.m_Worlds[handle.index];
        slot.api.reset();
        slot.active = false;
        slot.generation++;
        instance.m_FreeList.push_back(handle.index);
    }

    void PhysicsSystem::UpdateInstance(Handle<PhysicsInstance> handle, Timestep ts)
    {
        PhysicsAPI* api = GetInstance().GetAPI(handle);
        if (api == nullptr) return;
        api->Update(ts);
    }

    Handle<RigidBody> PhysicsSystem::CreateBody(Handle<PhysicsInstance> world, const BodyConfig& config)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return Handle<RigidBody>::MakeInvalid();
        return api->CreateBody(config);
    }

    void PhysicsSystem::DestroyBody(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->DestroyBody(handle);
    }

    Handle<CollisionCallback> PhysicsSystem::RegisterCallback(Handle<PhysicsInstance> world, const CollisionCallbackRef& callback)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return Handle<CollisionCallback>::MakeInvalid();
        return api->RegisterCallback(callback);
    }

    void PhysicsSystem::RemoveCallback(Handle<PhysicsInstance> world, Handle<CollisionCallback> handle)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->RemoveCallback(handle);
    }

    RaycastHit PhysicsSystem::CastRay(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return {};
        return api->CastRay(origin, direction, distance);
    }

    std::vector<RaycastHit> PhysicsSystem::CastRayAll(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return {};
        return api->CastRayAll(origin, direction, distance);
    }

    void PhysicsSystem::SetActive(Handle<PhysicsInstance> world, Handle<RigidBody> handle, bool active)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->SetActive(handle, active);
    }

    void PhysicsSystem::SetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle, UUID id)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->SetUUID(handle, id);
    }

    UUID PhysicsSystem::GetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return UUID(0);
        return api->GetUUID(handle);
    }

    void PhysicsSystem::SetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& transform)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->SetPhysTransform(handle, transform);
    }

    PhysTransform PhysicsSystem::GetPhysTransform(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return {};
        return api->GetPhysTransform(handle);
    }

    void PhysicsSystem::AddForce(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& force)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->AddForce(handle, force);
    }

    void PhysicsSystem::SetVelocity(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const glm::vec3& velocity)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->SetVelocity(handle, velocity);
    }

    void PhysicsSystem::SetGravity(Handle<PhysicsInstance> world, const glm::vec3& gravity)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return;
        api->SetGravity(gravity);
    }

    bool PhysicsSystem::CanMove(Handle<PhysicsInstance> world, Handle<RigidBody> handle, const PhysTransform& target)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return false;
        return api->CanMove(handle, target);
    }

    const BodyConfig* PhysicsSystem::GetBodyInfo(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetInstance().GetAPI(world);
        if (api == nullptr) return nullptr;
        return api->GetBodyInfo(handle);
    }
}