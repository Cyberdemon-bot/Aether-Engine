#pragma once

#include <entt/entt.hpp>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include "Aether/Core/UUID.h"

using Entity = entt::entity;
static constexpr Entity Null_Entity = entt::null;

namespace Aether {
    class Scene 
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity();
        Entity CreateEntity(std::string_view name);
        void DestroyEntity(Entity entity);

        bool IsValid(Entity entity) const;

        Entity FindEntity(UUID id) const;
        UUID GetUUID(Entity entity) const;

        entt::registry& Registry() {return m_Registry;}
        const entt::registry& Registry() const {return m_Registry;}

        template<typename T, typename... Args>
        T& AddComponent(Entity entity, Args&&... args)
        {
            return m_Registry.emplace<T>(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        void RemoveComponent(Entity entity)
        {
            m_Registry.remove<T>(entity);
        }

        template<typename T>
        bool HasComponent(Entity entity) const
        {
            return m_Registry.any_of<T>(entity);
        }

        template<typename T>
        T& GetComponent(Entity entity)
        {
            return m_Registry.get<T>(entity);
        }

        template<typename T>
        const T& GetComponent(Entity entity) const
        {
            return m_Registry.get<T>(entity);
        }

        template<typename... Components>
        auto View()
        {
            return m_Registry.view<Components...>();
        }

        template<typename... Components>
        auto Group()
        {
            return m_Registry.group<Components...>();
        }

    private:
        entt::registry m_Registry;
        std::unordered_map<UUID, Entity> m_EntityLibrary;
    };
}
