#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"

namespace Aether {
    Entity Scene::CreateEntity()
    {
        return CreateEntity("default entity");
    }

    Entity Scene::CreateEntity(std::string_view name)
    {
        Entity e = m_Registry.create();
        UUID id = UUID();
        m_Registry.emplace<IDComponent>(e, id);
        m_Registry.emplace<TagComponent>(e, std::string(name));
        m_Registry.emplace<TransformComponent>(e);
        m_EntityLibrary[id] = e;
        return e;
    }

    bool Scene::IsValid(Entity entity) const
    {
        return m_Registry.valid(entity);
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!m_Registry.valid(entity)) return;

        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    Entity Scene::FindEntity(UUID id) const
    {
        auto it = m_EntityLibrary.find(id);
        if(it != m_EntityLibrary.end()) return it->second;
        return Null_Entity;
    }

    UUID Scene::GetUUID(Entity entity) const
    {
         if (!m_Registry.valid(entity)) return UUID(0);
         return m_Registry.get<IDComponent>(entity).ID;
    }
}