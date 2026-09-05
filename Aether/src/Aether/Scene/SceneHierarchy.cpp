#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Importer/Importer.h"
#include "Aether/Scene/TransformMath.h"

namespace Aether {

    inline void ApplyWorldToLocalTransform(TransformComponent& transform, Entity parent, 
                const glm::mat4& pTransform, const glm::vec3& worldPos, const glm::quat& worldRot)
    {
        if (parent == Null_Entity)
        {
            transform.Translation = worldPos;
            transform.Rotation = worldRot;
        }
        else
        {
            glm::vec3 pP, pS; glm::quat pR;
            Utils::GetTRS(pTransform, pP, pR, pS);
            transform.Translation = glm::vec3(glm::inverse(pTransform) * glm::vec4(worldPos, 1.0f));
            transform.Rotation = glm::inverse(pR) * worldRot;
        }

        transform.Dirty = true;
    }

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

        if (parent != Null_Entity && IsValid(parent)) MakeParent(e, parent);
        else
        {
            Entity prevLast = m_LastRoot;
            if (m_LastRoot == Null_Entity) m_FirstRoot = e;
            else GetComponent<HierarchyComponent>(m_LastRoot).nextSibling = e;
            hierarchy.prevSibling = prevLast;
            m_LastRoot = e;
        }
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

    void Scene::CleanupEntityResources(Entity entity)
    {
        if (auto* anim = TryGetComponent<AnimatorComponent>(entity))
        {
            if (auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>())
            {
                if (anim->Cache.IsValid()) rigModule->DestroyCache(anim->Cache);
                if (anim->CurrentPose.IsValid()) rigModule->DestroyPose(anim->CurrentPose);
            }
        }

        if (auto* collider = TryGetComponent<ColliderComponent>(entity))
        {
            if (collider->ColliderHandle.IsValid())
                ServiceManager::GetService<PhysicsSystem>()->DestroyBody(m_PhysicsInstance, collider->ColliderHandle);
        }

        if (auto* script = TryGetComponent<ScriptComponent>(entity))
            ServiceManager::GetService<ScriptEngine>()->DestroyInstance(script->ScriptHandle);

        if (auto* mesh = TryGetComponent<MeshComponent>(entity))
            ServiceManager::GetService<AssetManager>()->Unload(mesh->UniqueSheet);
    }

    void Scene::ExcDestroyEntity(Entity entity, bool repair_hie)
    {
        if (!m_Registry.valid(entity)) return;
        if (repair_hie) BreakParent(entity);

        CleanupEntityResources(entity);

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
        GetComponent<HierarchyComponent>(entity).lastChild = Null_Entity;
        BreakParent(entity);

        CleanupEntityResources(entity);

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
            Entity prevLast = parentHie.lastChild;
            if (parentHie.firstChild == Null_Entity) parentHie.firstChild = child;
            else GetComponent<HierarchyComponent>(prevLast).nextSibling = child;
            hierarchy.prevSibling = prevLast;
            parentHie.lastChild = child;
        }
        else
        {
            Entity prevLast = m_LastRoot;
            if (m_LastRoot == Null_Entity) m_FirstRoot = child;
            else GetComponent<HierarchyComponent>(m_LastRoot).nextSibling = child;
            hierarchy.prevSibling = prevLast;
            m_LastRoot = child;
        }

        GetComponent<TransformComponent>(child).Dirty = true;
    }

    void Scene::BreakParent(Entity entity)
    {
        if (!IsValid(entity)) return;
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);
        Entity prev = hierarchy.prevSibling;
        Entity next = hierarchy.nextSibling;

        if (hierarchy.parent != Null_Entity)
        {
            auto& parentHie = GetComponent<HierarchyComponent>(hierarchy.parent);
            if (parentHie.firstChild == entity) parentHie.firstChild = next;
            else if (prev != Null_Entity && IsValid(prev)) GetComponent<HierarchyComponent>(prev).nextSibling = next;

            if (parentHie.lastChild == entity) parentHie.lastChild = prev;
            else if (next != Null_Entity && IsValid(next)) GetComponent<HierarchyComponent>(next).prevSibling = prev;
        }
        else
        {
            if (m_FirstRoot == entity) m_FirstRoot = next;
            else if (prev != Null_Entity && IsValid(prev)) GetComponent<HierarchyComponent>(prev).nextSibling = next;

            if (m_LastRoot == entity) m_LastRoot = prev;
            else if (next != Null_Entity && IsValid(next)) GetComponent<HierarchyComponent>(next).prevSibling = prev;
        }

        Entity newParent = hierarchy.parent;
        Entity firstChild = hierarchy.firstChild;
        Entity lastChild = hierarchy.lastChild;

        if (firstChild != Null_Entity)
        {
            for (Entity c = firstChild; c != Null_Entity; c = GetComponent<HierarchyComponent>(c).nextSibling)
            {
                GetComponent<HierarchyComponent>(c).parent = newParent;
                GetComponent<TransformComponent>(c).Dirty = true;
            }

            if (newParent != Null_Entity)
            {
                auto& parentHie = GetComponent<HierarchyComponent>(newParent);
                Entity prevLast = parentHie.lastChild;
                if (parentHie.firstChild == Null_Entity) parentHie.firstChild = firstChild;
                else if (prevLast != Null_Entity && IsValid(prevLast)) GetComponent<HierarchyComponent>(prevLast).nextSibling = firstChild;
                
                if (firstChild != Null_Entity && IsValid(firstChild)) GetComponent<HierarchyComponent>(firstChild).prevSibling = prevLast;
                parentHie.lastChild = lastChild;
            }
            else
            {
                Entity prevLast = m_LastRoot;
                if (m_LastRoot == Null_Entity) m_FirstRoot = firstChild;
                else if (prevLast != Null_Entity && IsValid(prevLast)) GetComponent<HierarchyComponent>(prevLast).nextSibling = firstChild;
                
                if (firstChild != Null_Entity && IsValid(firstChild)) GetComponent<HierarchyComponent>(firstChild).prevSibling = prevLast;
                m_LastRoot = lastChild;
            }
        }

        hierarchy.parent = Null_Entity;
        hierarchy.nextSibling = Null_Entity;
        hierarchy.prevSibling = Null_Entity;
        hierarchy.firstChild = Null_Entity;
        hierarchy.lastChild = Null_Entity;
    }

    void Scene::DirtyScan()
    {
        for (auto&& [entity, trans] : Storage<TransformComponent>().each())
        {
            if (trans.Dirty) MarkDirty(entity);
            else if (auto* coll = TryGetComponent<ColliderComponent>(entity))
            {
                if (coll->IsActive && coll->Type == MotionType::Dynamic)
                {
                    MarkDirty(entity);
                    trans.Dirty = false;
                }
            }
        }
    }

    void Scene::BreadthFirstSearch(bool usingFilter, bool removeHeader)
    {
        m_HierarchyBuffer.clear();

        auto& Queue = m_BFSQueue;
        auto view = View<HierarchyComponent>();
        ChainLoop(m_FirstRoot, [&](Entity entity)
        {
            if (!usingFilter) Queue.push({entity, 0});
            else
            {
                auto& t = GetComponent<TransformComponent>(entity);
                auto* coll = TryGetComponent<ColliderComponent>(entity);
                if (t.Dirty || t.SubtreeDirty || (coll && coll->Type != MotionType::Static))
                    Queue.push({entity, 0});
            }
        });

        if (Queue.empty()) return;
        uint32_t curr_depth = 0;
        uint32_t anchorIdx = 0;
        if (!removeHeader) m_HierarchyBuffer.push_back(FromNumber(static_cast<uint32_t>(0)));
    
        while (!Queue.empty())
        {
            auto [entity, depth] = Queue.front(); Queue.pop();
            if (depth > curr_depth && !removeHeader)
            {
                curr_depth = depth;
                m_HierarchyBuffer[anchorIdx] = FromNumber(static_cast<uint32_t>(m_HierarchyBuffer.size() - anchorIdx - 1));
                anchorIdx = static_cast<uint32_t>(m_HierarchyBuffer.size());
                m_HierarchyBuffer.push_back(FromNumber(static_cast<uint32_t>(0)));
            }
            m_HierarchyBuffer.push_back(entity);

    
            auto& parentT = GetComponent<TransformComponent>(entity);
            auto* parentColl = TryGetComponent<ColliderComponent>(entity);
            bool isParentDynamic = parentColl && parentColl->Type != MotionType::Static;
            bool parentDirty = parentT.Dirty || isParentDynamic;
    
            ChainLoop(GetComponent<HierarchyComponent>(entity).firstChild, [&](Entity child)
            {
                if (!usingFilter) Queue.push({child, depth + 1});
                else
                {
                    auto& childT = GetComponent<TransformComponent>(child);
                    auto* childColl = TryGetComponent<ColliderComponent>(child);
                    bool isChildDynamic = childColl && childColl->Type != MotionType::Static;
                    if (parentDirty || childT.Dirty || childT.SubtreeDirty || isChildDynamic)
                    {
                        if (parentDirty) childT.Dirty = true;
                        Queue.push({child, depth + 1});
                    }
                }
            });
        }

        if (!m_HierarchyBuffer.empty() && !removeHeader) 
            m_HierarchyBuffer[anchorIdx] = FromNumber(static_cast<uint32_t>(m_HierarchyBuffer.size() - anchorIdx - 1));
    }

    void Scene::SortHierarchyCache()
    {
        BreadthFirstSearch(false, true);
        m_Registry.storage<TransformComponent>().sort_as(m_HierarchyBuffer.begin(), m_HierarchyBuffer.end());
        m_Registry.sort<HierarchyComponent, TransformComponent>();
        m_Registry.sort<ColliderComponent, TransformComponent>();
        m_SortDirtyCount = 0;
    }

    Entity Scene::LoadHierarchy(const RegisteredScene* registered, Entity parent)
    {
        Entity first_child = Null_Entity;
        if (!registered->hierarchy) return first_child;

        auto* asset_manager = ServiceManager::GetService<AssetManager>();

        std::unordered_map<uint64_t, std::vector<Handle<Asset>>> skelToClipsMap;
        for (UUID clipID : registered->assets.Get(AssetType::Clip))
        {
            auto clipHandle = asset_manager->GetHandle(clipID);
            auto* clipAsset = asset_manager->GetAsset<AClip>(clipHandle);
            if (clipAsset)
            {
                uint64_t skelKey = clipAsset->m_Skeleton.Blend();
                skelToClipsMap[skelKey].push_back(clipHandle); 
            }
        }

        for (int rootIdx : registered->hierarchy->roots)
        {
            auto temp = CreateNodeEntity(registered, rootIdx, parent, skelToClipsMap);
            if (first_child == Null_Entity) first_child = temp;
        }
        return first_child;
    }

    Entity Scene::CreateNodeEntity(
        const RegisteredScene* reg, 
        int nodeIdx, 
        Entity parentEntity,
        const std::unordered_map<uint64_t, std::vector<Handle<Asset>>>& skelToClipsMap)
    {
        auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
        const Node& node = reg->hierarchy->nodes[nodeIdx];
        Entity e = CreateEntity(node.name, parentEntity);
        auto& t = GetComponent<TransformComponent>(e);
        t.Translation = node.translation;
        t.Rotation = glm::normalize(node.rotation);
        t.Scale = node.scale;
        t.Dirty = true;

        if (node.meshIdx >= 0 && node.meshIdx < (int)reg->assets.Get(AssetType::Mesh).size())
        {
            auto& component = AddComponent<MeshComponent>(e);
            component.Mesh = asset_manager->GetHandle(reg->assets.Get(AssetType::Mesh)[node.meshIdx]);
            component.SharedSheet = asset_manager->GetHandle(reg->assets.Get(AssetType::Sheet)[node.meshIdx]);
        }

        if (node.animatorIdx >= 0 && node.animatorIdx < (int)reg->assets.Get(AssetType::Skeleton).size())
        {
            UUID skeletonID = reg->assets.Get(AssetType::Skeleton)[node.animatorIdx];
            
            auto& comp = AddComponent<AnimatorComponent>(e);
            comp.Skeleton = asset_manager->GetHandle(skeletonID);

            auto it = skelToClipsMap.find(comp.Skeleton.Blend());
            if (it != skelToClipsMap.end())
            {
                comp.Clips = it->second;
            }

            if (!comp.Clips.empty())
            {
                auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
                auto* skeletonAsset = asset_manager->GetAsset<ASkeleton>(comp.Skeleton);
                auto* clipAsset = asset_manager->GetAsset<AClip>(comp.Clips[0]);
                if (skeletonAsset && clipAsset)
                {
                    comp.Cache = rigModule->CreateCache(clipAsset->m_Handle);
                    comp.CurrentPose = rigModule->CreatePose(skeletonAsset->m_Handle);
                }
            }
        }

        for (int childIdx : node.children)
            CreateNodeEntity(reg, childIdx, e, skelToClipsMap);

        return e;
    }

    void Scene::ResolveBoneAttachments()
    {
        auto view = View<BoneAttachmentComponent, TransformComponent>();
        auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
        auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        std::unordered_map<Entity, ResolvedAnimator> animatorCache;

        for (auto entity : view)
        {
            auto& attach = GetComponent<BoneAttachmentComponent>(entity);
            auto& transform = GetComponent<TransformComponent>(entity);
            Entity animEnt = attach.AnimatorEntity;

            if (!IsValid(animEnt) || !HasComponent<AnimatorComponent>(animEnt)) continue;

            auto [it, inserted] = animatorCache.try_emplace(animEnt);
            ResolvedAnimator& resolved = it->second;
            if (inserted)
            {
                auto& animComp = GetComponent<AnimatorComponent>(animEnt);
                auto* skeletonAsset = asset_manager->GetAsset<ASkeleton>(animComp.Skeleton);
                if (skeletonAsset && animComp.CurrentPose.IsValid())
                {
                    resolved.valid = true;
                    resolved.skeleton = skeletonAsset->m_Handle;
                    resolved.pose = rigModule->GetPose(animComp.CurrentPose);
                    resolved.world = GetComponent<TransformComponent>(animEnt).WorldTransform;
                }
            }
            if (!resolved.valid) continue;

            if (attach.JointIndex < 0) 
                attach.JointIndex = rigModule->GetJointIndex(resolved.skeleton, attach.JointName);

            if (!resolved.pose.empty() && attach.JointIndex >= 0 && (size_t)attach.JointIndex < resolved.pose.size())
            {
                glm::mat4 ibm; 
                rigModule->GetIBM(resolved.skeleton, attach.JointIndex, ibm);
                
                glm::mat4 modelSpaceMat = resolved.pose[attach.JointIndex] * glm::inverse(ibm);
                glm::mat4 boneWorld = resolved.world * modelSpaceMat;

                glm::vec3 right = glm::normalize(glm::vec3(boneWorld[0])); glm::vec3 up = glm::normalize(glm::vec3(boneWorld[1]));
                glm::vec3 forward = glm::normalize(glm::vec3(boneWorld[2])); glm::vec3 trans = glm::vec3(boneWorld[3]);
                glm::mat4 pureBoneWorld(1.0f);
                pureBoneWorld[0] = glm::vec4(right, 0.0f); pureBoneWorld[1] = glm::vec4(up, 0.0f); 
                pureBoneWorld[2] = glm::vec4(forward, 0.0f); pureBoneWorld[3] = glm::vec4(trans, 1.0f);

                glm::mat4 objScaleMat = glm::scale(glm::mat4(1.0f), transform.Scale);
                glm::mat4 localOffsetMat = glm::translate(glm::mat4(1.0f), transform.Translation) * glm::toMat4(transform.Rotation) * objScaleMat;
                transform.WorldTransform = pureBoneWorld * localOffsetMat;

                if (attach.affectChild)
                {
                    Entity firstChild = GetComponent<HierarchyComponent>(entity).firstChild;
                    if (firstChild != Null_Entity) UpdateSubtreeTransforms(firstChild, transform.WorldTransform);
                }
            }
        }
    }

    void Scene::UpdateSubtreeTransforms(Entity entity, const glm::mat4& pTransform)
    {
        while (entity != Null_Entity && IsValid(entity))
        {
            auto& transform = GetComponent<TransformComponent>(entity);
            auto& hierarchy = GetComponent<HierarchyComponent>(entity);
            transform.WorldTransform = pTransform * transform.GetLocalTransform();

            if (hierarchy.firstChild != Null_Entity)
                UpdateSubtreeTransforms(hierarchy.firstChild, transform.WorldTransform);

            entity = hierarchy.nextSibling;
        }
    }
    
    void Scene::UpdateTransform(Entity entity)
    {
        auto& transform = GetComponent<TransformComponent>(entity);
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);

        glm::mat4 pTransform = glm::mat4(1.0f);
        bool pDirty = false;

        if (hierarchy.parent != Null_Entity)
        {
            const auto& parentTransform = GetComponent<TransformComponent>(hierarchy.parent);
            pTransform = parentTransform.WorldTransform;
            pDirty = (parentTransform.LastUpdate == m_CurrentFrame);
        }

        bool isWorldDirty = transform.Dirty || pDirty;

        if (auto* rbComp = TryGetComponent<ColliderComponent>(entity))
        {
            auto* physys = ServiceManager::GetService<PhysicsSystem>();
            Handle<RigidBody>& handle = rbComp->ColliderHandle;
            MotionType motionType = rbComp->Type;

            if (handle.IsValid()) 
            {
                if (motionType == MotionType::Dynamic && !transform.Dirty)
                {
                    PhysTransform physTrans = physys->GetPhysTransform(m_PhysicsInstance, handle);
                    glm::vec3 worldPos = physTrans.translation - (physTrans.rotation * rbComp->ColliderOffset);
                    ApplyWorldToLocalTransform(transform, hierarchy.parent, pTransform, worldPos, physTrans.rotation);
                    isWorldDirty = true; 
                }
                else if (isWorldDirty)
                {
                    glm::mat4 tempWorld = pTransform * transform.GetLocalTransform();
                    glm::vec3 pos, scale; glm::quat rot; 
                    Utils::GetTRS(tempWorld, pos, rot, scale);
                    PhysTransform target = {pos + (rot * rbComp->ColliderOffset), rot};

                    if (motionType == MotionType::Kinematic && !physys->CanMove(m_PhysicsInstance, handle, target))
                    {
                        PhysTransform physTrans = physys->GetPhysTransform(m_PhysicsInstance, handle);
                        glm::vec3 validPos = physTrans.translation - (physTrans.rotation * rbComp->ColliderOffset);
                        ApplyWorldToLocalTransform(transform, hierarchy.parent, pTransform, validPos, physTrans.rotation);
                    }
                    else physys->SetPhysTransform(m_PhysicsInstance, handle, target);
                }
            }
        }

        if (isWorldDirty)
        {
            if (transform.Dirty) transform.WorldTransform = pTransform * transform.GetLocalTransform();

            if (auto* light = TryGetComponent<LightComponent>(entity))
                light->Config.position = glm::vec3(transform.WorldTransform[3]);

            transform.LastUpdate = m_CurrentFrame;
        }

        transform.Dirty = false;
        transform.SubtreeDirty = false;
    }
}