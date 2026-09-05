#pragma once

#include <entt/entt.hpp>
#include <string_view>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include "Aether/Core/UUID.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Scene/Component.h"
#include "Aether/Renderer/EditorCamera.h"

namespace Aether {

    using Entity = entt::entity;
    using entt::get;
    using entt::exclude;
    static const glm::vec4 GREEN = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    static const glm::vec4 RED = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    template<typename Component>
    struct ComponentInfo;
    struct Prefab;
    struct PhysicsInstance;
    struct RegisteredScene;

    struct DestroyInfo
    {
        Entity entity;
        bool clearHierarchy;
        bool repairHie;
    };

    struct ResolvedAnimator
    {
        bool valid = false;
        Handle<Skeleton> skeleton;
        std::span<const glm::mat4> pose;
        glm::mat4 world{1.0f};
    };

    class AETHER_API Scene 
    {
    public:
        Scene() = default;
        ~Scene() = default;

        void Init();
        void Shutdown();

        void OnUpdate(Timestep ts, EditorCamera* camera = nullptr);
        void OnTick(Timestep ts);

        Entity CreateEntity();
        Entity CreateEntity(std::string_view name, Entity parent = Null_Entity);
        Entity CreateEntity(std::string_view name, UUID id, Entity parent = Null_Entity);
        void DestroyEntity(Entity entity, bool repair_hie = true);
        void DestroyHierarchy(Entity entity);
        void MakeParent(Entity child, Entity parent);
        void BreakParent(Entity entity);
        void MarkDirty(Entity entity);
        glm::vec3 GetWorldPosition(Entity entity);

        Entity LoadHierarchy(const RegisteredScene* registered, Entity parent = Null_Entity);
        bool IsValid(Entity entity) const;

        Entity FindEntity(UUID id) const;
        std::vector<Entity> FindEntity(std::string_view tag) const;

        static uint64_t ToNumber64(Entity entity)
        {
            return static_cast<uint64_t>(entt::to_integral(entity));
        }

        static uint32_t ToNumber32(Entity entity)
        {
            return static_cast<uint32_t>(entt::to_integral(entity));
        }

        static Entity FromNumber(uint64_t number)
        {
            return entt::entity(static_cast<std::underlying_type_t<Entity>>(number));
        }

        static Entity FromNumber(uint32_t number)
        {
            return entt::entity(static_cast<std::underlying_type_t<Entity>>(number));
        }

        Handle<PhysicsInstance> GetPhysicsInstance() 
        { 
            return m_PhysicsInstance; 
        }

        void SortHierarchyCache();
        uint32_t GetHierarchyDriftCount() const 
        { 
            return m_SortDirtyCount;
        }

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

        template<typename T>
        T* TryGetComponent(Entity entity)
        {
            return m_Registry.try_get<T>(entity);
        }

        template<typename T>
        const T* TryGetComponent(Entity entity) const
        {
            return m_Registry.try_get<const T>(entity);
        }

        template<typename... Owned, typename... Args>
        auto Group(Args&&... args)
        {
            return m_Registry.group<Owned...>(std::forward<Args>(args)...);
        }

        template<typename T>
        auto& Storage()
        {
            return m_Registry.storage<T>();
        }

        template<typename T>
        const auto& Storage() const
        {
            return *m_Registry.storage<std::remove_const_t<T>>();
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
        Entity m_FirstRoot = Null_Entity;
        Entity m_LastRoot = Null_Entity;
        Handle<PhysicsInstance> m_PhysicsInstance;
        
        entt::registry m_Registry;
        std::vector<LightParam> m_SceneLights;
        std::vector<DestroyInfo> m_DestroyQueue;
        std::vector<Entity> m_HierarchyBuffer;
        std::queue<std::pair<Entity, uint32_t>> m_BFSQueue;
        std::unordered_map<UUID, Entity> m_EntityLibrary;
        void DirtyScan();
        void BreadthFirstSearch(bool usingFilter = true, bool removeHeader = false);
        void UpdateTransform(Entity entity);
        void UpdateSubtreeTransforms(Entity entity, const glm::mat4& pTransform);
        void ResolveBoneAttachments();

        void ExcDestroyEntity(Entity entity, bool repair_hie);
        void ExcDestroyHierarchy(Entity entity);
        void CleanupEntityResources(Entity entity);

        Entity CreateNodeEntity(
            const RegisteredScene* reg, 
            int nodeIdx, 
            Entity parentEntity,
            const std::unordered_map<uint64_t, std::vector<Handle<Asset>>>& skelToClipsMap
        );

        entt::registry& Registry() {return m_Registry;}
        const entt::registry& Registry() const {return m_Registry;}

        template<typename Fn>
        void ChainLoop(Entity entity, Fn fn)
        {
            while (entity != Null_Entity)
            {
                fn(entity);
                entity = GetComponent<HierarchyComponent>(entity).nextSibling;
            }
        }
    };
}