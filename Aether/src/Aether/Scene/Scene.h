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

    

    namespace Utils
    {
        struct Frustum
        {
            glm::vec4 Planes[6]; 
        };

        Frustum GetFrustum(const glm::mat4& vp)
        {
            Frustum f;
            glm::mat4 m = glm::transpose(vp);
            f.Planes[0] = m[3] + m[0]; // Left
            f.Planes[1] = m[3] - m[0];  // Right
            f.Planes[2] = m[3] + m[1]; // Bottom
            f.Planes[3] = m[3] - m[1]; // Top
            f.Planes[4] = m[3] + m[2]; // Near
            f.Planes[5] = m[3] - m[2]; // Far

            for (int i = 0; i < 6; i++)
            {
                float length = glm::length(glm::vec3(f.Planes[i]));
                f.Planes[i] /= length;
            }
            return f;
        }

        void TransformBound(const glm::vec3& min, const glm::vec3& max, const glm::mat4& transform, glm::vec3& outMin, glm::vec3& outMax)
        {
            glm::vec3 corners[8] =
            {
                {min.x, min.y, min.z},
                {max.x, min.y, min.z},
                {min.x, max.y, min.z},
                {max.x, max.y, min.z},
                {min.x, min.y, max.z},
                {max.x, min.y, max.z},
                {min.x, max.y, max.z},
                {max.x, max.y, max.z},
            };

            outMin = glm::vec3(FLT_MAX);
            outMax = glm::vec3(-FLT_MAX);

            for (int i = 0; i < 8; i++)
            {
                glm::vec3 world = glm::vec3(transform * glm::vec4(corners[i], 1.0));
                outMin = glm::min(outMin, world);
                outMax = glm::max(outMax, world);
            }
        }

        bool CheckBoundVisible(const Frustum& frustum, const glm::vec3& min, const glm::vec3& max)
        {
            for (int i = 0; i < 6; i++)
            {
                glm::vec3 normal = glm::vec3(frustum.Planes[i]);
                float d = frustum.Planes[i].w;
                glm::vec3 p = min;
                if (normal.x >= 0) p.x = max.x;
                if (normal.y >= 0) p.y = max.y;
                if (normal.z >= 0) p.z = max.z;
                if (glm::dot(normal, p) + d < 0)
                    return false;
            }
            return true;
        }
    }
    class AETHER_API Scene 
    {
    public:
        Scene();
        ~Scene();

        Entity CreateEntity();
        Entity CreateEntity(std::string_view name, Entity parent = Null_Entity);
        void DestroyEntity(Entity entity);
        void DestroyHierarchy(Entity entity);
        void MakeParent(Entity child, Entity parent);
        void BreakParent(Entity entity);

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
        entt::registry m_Registry;
        std::unordered_map<UUID, Entity> m_EntityLibrary;
        std::vector<LightParam> m_SceneLights;
        void UpdateTransform(Entity entity, const glm::mat4& pTransfrom, bool pDirty);
    };
}
