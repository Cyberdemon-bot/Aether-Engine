#pragma once

#include <entt/entt.hpp>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include "Aether/Core/UUID.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Importer/ImporterAPI.h"

namespace Aether {

    using Entity = entt::entity;
    static constexpr Entity Null_Entity = entt::null;
    static const glm::vec4 GREEN = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    static const glm::vec4 RED = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    class AETHER_API Scene 
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity();
        Entity CreateEntity(std::string_view name, Entity parent = Null_Entity);
        UUID GetSceneID() { return m_SceneID; }
        void DestroyEntity(Entity entity);
        void DestroyHierarchy(Entity entity);
        void MakeParent(Entity child, Entity parent);
        void BreakParent(Entity entity);
        void MarkDirty(Entity entity);

        bool IsValid(Entity entity) const;
        void Update(Timestep ts, EditorCamera* camera = nullptr);

        Entity FindEntity(UUID id) const;
        UUID GetUUID(Entity entity) const;

        void LoadHierarchy(const RegisteredScene& registered, Entity parent = Null_Entity);

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

        template<typename T>
        void CloneComponent(Entity entity, Entity sample)
        {
            if (!HasComponent<T>(sample)) return;
            if (!HasComponent<T>(entity)) AddComponent<T>(entity);
            GetComponent<T>(entity) = GetComponent<T>(sample);
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
        UUID m_SceneID;
        uint64_t m_CurrentFrame = 0;
        uint32_t m_Threshold = 64;
        entt::registry m_Registry;
        std::unordered_map<UUID, Entity> m_EntityLibrary;
        std::vector<LightParam> m_SceneLights;
        std::vector<std::vector<Entity>> m_HierarchyLevels;
        void BreadthFirstSearch();
        void UpdateTransform(Entity entity);
        void CreateNodeEntity(const RegisteredScene& reg, int nodeIdx, Entity parentEntity);
    };
}
