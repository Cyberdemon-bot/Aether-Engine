#pragma once

#include <entt/entt.hpp>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include "Aether/Core/UUID.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Importer/ImporterAPI.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Scene/Component.h"

namespace Aether {

    using Entity = entt::entity;
    static const glm::vec4 GREEN = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    static const glm::vec4 RED = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    template<typename Component>
    struct ComponentInfo;
    struct Prefab;
    struct PhysicsInstance;

    struct DestroyInfo
    {
        Entity entity;
        bool clearHierarchy;
        bool repairHie;
    };
    class AETHER_API Scene 
    {
    public:
        Scene() = default;
        ~Scene() = default;

        void Init();
        void Shutdown();

        Entity CreateEntity();
        Entity CreateEntity(std::string_view name, Entity parent = Null_Entity);
        Entity CreateEntity(std::string_view name, UUID id, Entity parent = Null_Entity);
        void DestroyEntity(Entity entity, bool repair_hie = true);
        void DestroyHierarchy(Entity entity);
        void MakeParent(Entity child, Entity parent);
        void BreakParent(Entity entity);
        void MarkDirty(Entity entity);
        glm::vec3 GetWorldPosition(Entity entity);

        void ImportPrefab(Entity entity, const Prefab& prefab, bool override = false);
        Prefab ExportPrefab(Entity entity) const;

        bool IsValid(Entity entity) const;
        void Update(Timestep ts, EditorCamera* camera = nullptr);

        Entity FindEntity(UUID id) const;
        std::vector<Entity> FindEntity(const std::string& tag) const;
        UUID GetUUID(Entity entity) const;

        void LoadHierarchy(const RegisteredScene& registered, Entity parent = Null_Entity);
        Handle<PhysicsInstance> GetPhysicsInstance() { return m_PhysicsInstance; }

        void SortHierarchyCache();
        uint32_t GetHierarchyDriftCount() const { return m_SortDirtyCount; }

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
        auto View() const
        {
            return m_Registry.view<const Components...>(); 
        }

        template<typename... Components>
        auto Group()
        {
            return m_Registry.group<Components...>();
        }

        template<typename Component, typename Fn>
        void Sort(Fn cmp)
        {
            m_Registry.sort<Component>(cmp);
        } 

    private:
        uint64_t m_CurrentFrame = 0;
        uint32_t m_Threshold = 64;
        uint32_t m_SortDirtyCount = 0;
        Handle<PhysicsInstance> m_PhysicsInstance;
        entt::registry m_Registry;
        std::vector<LightParam> m_SceneLights;
        std::vector<std::vector<Entity>> m_HierarchyLevels;
        std::vector<DestroyInfo> m_DestroyQueue;
        std::unordered_map<UUID, Entity> m_EntityLibrary;
        std::unordered_map<Entity, uint32_t> m_Rank;
        void DirtyScan();
        void BreadthFirstSearch(bool usingFilter = true);
        void UpdateTransform(Entity entity);
        void CreateNodeEntity(const RegisteredScene& reg, int nodeIdx, Entity parentEntity);
        void UpdateSubtreeTransforms(Entity entity, const glm::mat4& pTransform);
        void ResolveBoneAttachments();

        void ExcDestroyEntity(Entity entity, bool repair_hie);
        void ExcDestroyHierarchy(Entity entity);

        template<typename T>
        void LoadComponent(Entity entity, const ComponentInfo<T>& info, bool override)
        {
            if (!info.IsExits) return;
            if (m_Registry.any_of<T>(entity))
            {
                if (override) m_Registry.get<T>(entity) = info.data;
            }
            else
            {
                m_Registry.emplace<T>(entity) = info.data;
            }
        }
    };
}
