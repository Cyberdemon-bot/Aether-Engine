#include "aepch.h"
#include "Aether/Core/Log.h"
#include "Aether/Physics/PhysicsSystem.h"

namespace Aether {

    PhysicsAPI* PhysicsSystem::GetAPI(Handle<PhysicsInstance> world)
    {
        if (!world.IsValid()) return nullptr;
        if (world.index >= m_Worlds.size()) return nullptr;
        PhysicsWorldSlot& slot = m_Worlds[world.index];
        if (slot.generation != world.generation) return nullptr;
        return slot.api.get();
    }

    void PhysicsSystem::Init()
    {
        m_Worlds.reserve(8);
        AE_CORE_INFO("PhysicsSystem initialized");
    }

    void PhysicsSystem::Shutdown()
    {
        for (auto& slot : m_Worlds)
        {
            if (slot.active && slot.api)
                slot.api->Shutdown();
        }
        m_Worlds.clear();
        m_FreeList.clear();
    }

    Handle<PhysicsInstance> PhysicsSystem::CreateInstance()
    {
        uint32_t index;

        if (!m_FreeList.empty())
        {
            index = m_FreeList.back();
            m_FreeList.pop_back();
        }
        else
        {
            index = (uint32_t)m_Worlds.size();
            m_Worlds.emplace_back();
        }

        PhysicsWorldSlot& slot = m_Worlds[index];
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
        PhysicsAPI* api = GetAPI(handle);
        if (api == nullptr) return;

        api->Shutdown();
        PhysicsWorldSlot& slot = m_Worlds[handle.index];
        slot.api.reset();
        slot.active = false;
        slot.generation++;
        m_FreeList.push_back(handle.index);
    }

    void PhysicsSystem::UpdateInstance(Handle<PhysicsInstance> handle, Timestep ts)
    {
        PhysicsAPI* api = GetAPI(handle);
        if (api == nullptr) return;
        api->Update(ts);
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

    RaycastHit PhysicsSystem::CastRay(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return {};
        return api->CastRay(origin, direction, distance);
    }

    std::vector<RaycastHit> PhysicsSystem::CastRayAll(Handle<PhysicsInstance> world, const glm::vec3& origin, const glm::vec3& direction, float distance)
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

    void PhysicsSystem::SetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle, UUID id)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return;
        api->SetUUID(handle, id);
    }

    UUID PhysicsSystem::GetUUID(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return UUID(0);
        return api->GetUUID(handle);
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

    const BodyConfig* PhysicsSystem::GetBodyInfo(Handle<PhysicsInstance> world, Handle<RigidBody> handle)
    {
        PhysicsAPI* api = GetAPI(world);
        if (api == nullptr) return nullptr;
        return api->GetBodyInfo(handle);
    }
}