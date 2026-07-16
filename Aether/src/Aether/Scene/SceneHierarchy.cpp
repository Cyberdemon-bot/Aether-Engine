#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"

namespace Aether {
    void Scene::MarkDirty(Entity entity)
    {
        GetComponent<TransformComponent>(entity).Dirty = true;
        Entity current = GetComponent<HierarchyComponent>(entity).parent;
        while (current != Null_Entity)
        {
            auto& t = GetComponent<TransformComponent>(current);
            if (t.SubtreeDirty) break;
            t.SubtreeDirty = true;
            current = GetComponent<HierarchyComponent>(current).parent;
        }
    }
    
    Entity Scene::CreateEntity()
    {
        return CreateEntity("default entity");
    }

    Entity Scene::CreateEntity(std::string_view name, Entity parent)
    {
        return CreateEntity(name, UUID(), parent);
    }

    Entity Scene::CreateEntity(std::string_view name, UUID id, Entity parent)
    {
        Entity e = m_Registry.create();
        m_Registry.emplace<IDComponent>(e, id);
        m_Registry.emplace<TagComponent>(e, std::string(name));
        m_Registry.emplace<TransformComponent>(e);
        auto& hierarchy = m_Registry.emplace<HierarchyComponent>(e);
        m_EntityLibrary[id] = e;

        if (parent != Null_Entity && IsValid(parent))
            MakeParent(e, parent);

        m_SortDirtyCount++; 
        return e;
    }

    void Scene::DestroyHierarchy(Entity entity)
    {
        m_DestroyQueue.push_back({entity, true, false});
    }

    void Scene::DestroyEntity(Entity entity, bool repair_hie)
    {
        m_DestroyQueue.push_back({entity, false, repair_hie});
    }

    void Scene::ExcDestroyEntity(Entity entity, bool repair_hie)
    {
        if (!m_Registry.valid(entity)) return;
        if (repair_hie) BreakParent(entity);

        if (HasComponent<AnimatorComponent>(entity))
        {
            auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            if (rigModule)
            {
                auto& comp = GetComponent<AnimatorComponent>(entity);
                if (comp.Cache.IsValid()) rigModule->DestroyCache(comp.Cache);
                if (comp.CurrentPose.IsValid()) rigModule->DestroyPose(comp.CurrentPose);
            }
        }

        if (HasComponent<ColliderComponent>(entity) && GetComponent<ColliderComponent>(entity).ColliderHandle.IsValid()) 
            ServiceManager::GetService<PhysicsSystem>()->DestroyBody(m_PhysicsInstance, GetComponent<ColliderComponent>(entity).ColliderHandle);

        if (HasComponent<AudioSourceComponent>(entity))
        {
            auto& audio = GetComponent<AudioSourceComponent>(entity);
            if (audio.SourceHandle.IsValid())
                ServiceManager::GetService<AudioSystem>()->DestroySource(audio.SourceHandle);
        }

        if (HasComponent<ScriptComponent>(entity))
        {
            auto& script = GetComponent<ScriptComponent>(entity);
            ServiceManager::GetService<ScriptEngine>()->DestroyInstance(script.ScriptHandle);
        }
        
        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    void Scene::ExcDestroyHierarchy(Entity entity)
    {
        if (!m_Registry.valid(entity)) return;

        auto hierarchy = GetComponent<HierarchyComponent>(entity);
        Entity currentChild = hierarchy.firstChild;
        while(currentChild != Null_Entity)
        {
            Entity next = GetComponent<HierarchyComponent>(currentChild).nextSibling;
            ExcDestroyHierarchy(currentChild);
            currentChild = next;
        }

        GetComponent<HierarchyComponent>(entity).firstChild = Null_Entity;
        BreakParent(entity);

        if (HasComponent<AnimatorComponent>(entity))
        {
            auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            if (rigModule)
            {
                auto& comp = GetComponent<AnimatorComponent>(entity);
                if (comp.Cache.IsValid()) rigModule->DestroyCache(comp.Cache);
                if (comp.CurrentPose.IsValid()) rigModule->DestroyPose(comp.CurrentPose);
            }
        }
        if (HasComponent<ColliderComponent>(entity) && GetComponent<ColliderComponent>(entity).ColliderHandle.IsValid()) 
            ServiceManager::GetService<PhysicsSystem>()->DestroyBody(m_PhysicsInstance, GetComponent<ColliderComponent>(entity).ColliderHandle);

        if (HasComponent<AudioSourceComponent>(entity))
        {
            auto& audio = GetComponent<AudioSourceComponent>(entity);
            if (audio.SourceHandle.IsValid())
                ServiceManager::GetService<AudioSystem>()->DestroySource(audio.SourceHandle);
        }

        if (HasComponent<ScriptComponent>(entity))
        {
            auto& script = GetComponent<ScriptComponent>(entity);
            ServiceManager::GetService<ScriptEngine>()->DestroyInstance(script.ScriptHandle);
        }

        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    void Scene::MakeParent(Entity child, Entity parent)
    {
        if (!IsValid(child)) return;
        if (parent != Null_Entity && !IsValid(parent)) return;

        Entity ancestor = parent;
        while (ancestor!= Null_Entity) 
        {
            if (ancestor == child) 
            {
                AE_CORE_ERROR("[Hierarchy] Cycle detected! Cannot parent Entity '{0}' to '{1}' as it would create an infinite loop.", 
                    GetComponent<TagComponent>(child).Tag, 
                    GetComponent<TagComponent>(parent).Tag);
                return; 
            }
            ancestor = GetComponent<HierarchyComponent>(ancestor).parent;
        }

        BreakParent(child);

        auto& hierarchy = GetComponent<HierarchyComponent>(child);

        if (parent != Null_Entity && IsValid(parent))
        {
            hierarchy.parent = parent;
            auto& parentHie = GetComponent<HierarchyComponent>(parent);
            if (parentHie.firstChild == Null_Entity) parentHie.firstChild = child;
            else
            {
                Entity lastSib = parentHie.firstChild;
                while (GetComponent<HierarchyComponent>(lastSib).nextSibling != Null_Entity)
                    lastSib = GetComponent<HierarchyComponent>(lastSib).nextSibling;
                GetComponent<HierarchyComponent>(lastSib).nextSibling = child;
                hierarchy.prevSibling = lastSib;
            }
        }
        else
        {
            auto view = View<HierarchyComponent>();
            Entity sib = Null_Entity;
            for (auto entity : view)
            {
                if (entity == child) continue;
                if (GetComponent<HierarchyComponent>(entity).parent == Null_Entity)
                {
                    sib = entity;
                    break;
                }
            }
            if (sib != Null_Entity)
            {
                Entity oldNext = GetComponent<HierarchyComponent>(sib).nextSibling;
                hierarchy.prevSibling = sib;
                hierarchy.nextSibling = oldNext;
                GetComponent<HierarchyComponent>(sib).nextSibling = child;
                if (oldNext != Null_Entity) GetComponent<HierarchyComponent>(oldNext).prevSibling = child;
            }
        }

        GetComponent<TransformComponent>(child).Dirty = true;
    }

    void Scene::BreakParent(Entity entity)
    {
        if (!IsValid(entity)) return;
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);

        if (hierarchy.prevSibling != Null_Entity)
            GetComponent<HierarchyComponent>(hierarchy.prevSibling).nextSibling = hierarchy.nextSibling;
        if (hierarchy.nextSibling != Null_Entity)
            GetComponent<HierarchyComponent>(hierarchy.nextSibling).prevSibling = hierarchy.prevSibling;

        if (hierarchy.parent != Null_Entity)
        {
            auto& parentHie = GetComponent<HierarchyComponent>(hierarchy.parent);
            if (parentHie.firstChild == entity)
                parentHie.firstChild = hierarchy.nextSibling;

            Entity child = hierarchy.firstChild;
            while (child != Null_Entity)
            {
                Entity nextChild = GetComponent<HierarchyComponent>(child).nextSibling;
                GetComponent<HierarchyComponent>(child).parent      = hierarchy.parent;
                GetComponent<HierarchyComponent>(child).nextSibling = Null_Entity;
                GetComponent<HierarchyComponent>(child).prevSibling = Null_Entity;
                GetComponent<TransformComponent>(child).Dirty       = true;

                if (parentHie.firstChild == Null_Entity) parentHie.firstChild = child;
                else
                {
                    Entity tail = parentHie.firstChild;
                    while (GetComponent<HierarchyComponent>(tail).nextSibling != Null_Entity)
                        tail = GetComponent<HierarchyComponent>(tail).nextSibling;
                    GetComponent<HierarchyComponent>(tail).nextSibling  = child;
                    GetComponent<HierarchyComponent>(child).prevSibling = tail;
                }
                child = nextChild;
            }
        }

        hierarchy.parent = Null_Entity;
        hierarchy.prevSibling = Null_Entity;
        hierarchy.nextSibling = Null_Entity;
        hierarchy.firstChild = Null_Entity;
    }

    void Scene::DirtyScan()
    {
        auto view = View<TransformComponent>();
        for (auto entity : view)
            if (GetComponent<TransformComponent>(entity).Dirty)
                MarkDirty(entity);
    }

    void Scene::BreadthFirstSearch(bool usingFilter)
    {
        m_HierarchyLevels.clear();
        std::queue<std::pair<Entity, uint32_t>> Queue;
        auto view = View<HierarchyComponent>();
    
        for (auto entity : view)
        {
            if (GetComponent<HierarchyComponent>(entity).parent != Null_Entity) continue;
    
            if (usingFilter)
            {
                auto& t = GetComponent<TransformComponent>(entity);
                if (t.Dirty || t.SubtreeDirty ||
                (HasComponent<ColliderComponent>(entity) &&
                    GetComponent<ColliderComponent>(entity).Type != MotionType::Static))
                    Queue.push({entity, 0});
            }
            else Queue.push({entity, 0});
        }
    
        while (!Queue.empty())
        {
            auto [entity, depth] = Queue.front(); Queue.pop();
            if (m_HierarchyLevels.size() <= depth) m_HierarchyLevels.push_back({});
            m_HierarchyLevels[depth].push_back(entity);
    
            auto& parentT = GetComponent<TransformComponent>(entity);
            bool  parentDirty = parentT.Dirty || parentT.SubtreeDirty;
    
            Entity child = GetComponent<HierarchyComponent>(entity).firstChild;
            while (child != Null_Entity)
            {
                if (!usingFilter) Queue.push({child, depth + 1});
                else
                {
                    auto& childT = GetComponent<TransformComponent>(child);
                    if (parentDirty || childT.Dirty || childT.SubtreeDirty || HasComponent<ColliderComponent>(entity))
                    {
                        if (parentDirty) childT.Dirty = true;
                        Queue.push({child, depth + 1});
                    }
                }
                child = GetComponent<HierarchyComponent>(child).nextSibling;
            }
        }
    }

    void Scene::SortHierarchyCache()
    {
        BreadthFirstSearch(false);
        std::vector<Entity> topoOrder;
        topoOrder.reserve(m_EntityLibrary.size());
        for (auto& level : m_HierarchyLevels)
            for (auto e : level)
                topoOrder.push_back(e);

        m_Registry.storage<TransformComponent>().sort_as(topoOrder.begin(), topoOrder.end());
        m_Registry.sort<HierarchyComponent, TransformComponent>();
        m_Registry.sort<ColliderComponent, TransformComponent>();
        m_SortDirtyCount = 0;
    }

    void Scene::CreateNodeEntity(const RegisteredScene& reg, int nodeIdx, Entity parentEntity)
    {
        auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
        const Node& node = reg.hierarchy->nodes[nodeIdx];
        Entity e = CreateEntity(node.name, parentEntity);
        auto& t = GetComponent<TransformComponent>(e);
        t.Translation = node.translation;
        t.Rotation = glm::normalize(node.rotation);
        t.Scale = node.scale;
        t.Dirty = true;

        if (node.meshIdx >= 0 && node.meshIdx < (int)reg.meshIDs.size())
        {
            auto& component = AddComponent<MeshComponent>(e);
            component.Mesh = asset_manager->GetHandle(reg.meshIDs[node.meshIdx]);
            auto sh_handle = asset_manager->CreateAsset<Sheet>(UUID());
            auto* sh = asset_manager->GetAsset<Sheet>(sh_handle);
            sh->Resize(reg.meshMap[node.meshIdx].size());
            for(size_t i = 0; i < reg.meshMap[node.meshIdx].size(); i++)
            {
                auto& id = reg.meshMap[node.meshIdx][i];
                sh->SetDefault(i, asset_manager->GetHandle(id));
            }
            component.Sheet = sh_handle;
        }

        if (node.animatorIdx >= 0 && node.animatorIdx < (int)reg.animators.size())
        {
            const auto& animator = reg.animators[node.animatorIdx];
            auto& comp = AddComponent<AnimatorComponent>(e);
            comp.Skeleton = animator.skeleton;
            comp.Clips = animator.clips;

            if (!comp.Clips.empty())
            {
                auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
                auto* skeletonAsset = asset_manager->GetAsset<Skeleton>(comp.Skeleton);
                auto* clipAsset = asset_manager->GetAsset<Clip>(comp.Clips[0]);
                if (skeletonAsset && clipAsset)
                {
                    comp.Cache = rigModule->CreateCache(clipAsset->m_Handle);
                    comp.CurrentPose = rigModule->CreatePose(skeletonAsset->m_Handle);
                }
            }
        }
        for (int childIdx : node.children)
            CreateNodeEntity(reg, childIdx, e);
    }

    void Scene::LoadHierarchy(const RegisteredScene& registered, Entity parent)
    {
        if(!registered.hierarchy) return;
        for (int rootIdx : registered.hierarchy->roots)
            CreateNodeEntity(registered, rootIdx, parent);
    }
}