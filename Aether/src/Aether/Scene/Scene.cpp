#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Core/JobSystem.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Aether/Assets/AssetManager.h"

namespace Aether {
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

        bool CheckSphereVisible(const Frustum& frustum, const glm::vec3& center, float radius)
        {
            for (int i = 0; i < 6; i++)
            {
                glm::vec3 normal = glm::vec3(frustum.Planes[i]);
                float d = frustum.Planes[i].w;
                float distance = glm::dot(normal, center) + d;
                if (distance < -radius)
                {
                    return false;
                }
            }
            return true;
        }
    }

    Scene::Scene() 
    {
    }

    Scene::~Scene() 
    {
    }
    
    Entity Scene::CreateEntity()
    {
        return CreateEntity("default entity");
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
        {
            hierarchy.parent = parent;
            auto& parentHie = GetComponent<HierarchyComponent>(parent);
            if (parentHie.firstChild == Null_Entity) parentHie.firstChild = e;
            else
            {
                Entity lastSib = parentHie.firstChild;
                while (GetComponent<HierarchyComponent>(lastSib).nextSibling != Null_Entity)
                    lastSib = GetComponent<HierarchyComponent>(lastSib).nextSibling;
                
                GetComponent<HierarchyComponent>(lastSib).nextSibling = e;
                hierarchy.prevSibling = lastSib;
            }
        }
        else
        {
            auto view = View<HierarchyComponent>();
            Entity sib = Null_Entity; 
            for (auto entity : view)
            {
                if (entity == e) continue;
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
                GetComponent<HierarchyComponent>(sib).nextSibling = e;
                if (oldNext != Null_Entity) GetComponent<HierarchyComponent>(oldNext).prevSibling = e;
            }
        }
        return e;
    }

    Entity Scene::CreateEntity(std::string_view name, Entity parent)
    {
        Entity e = m_Registry.create();
        UUID id = UUID();
        m_Registry.emplace<IDComponent>(e, id);
        m_Registry.emplace<TagComponent>(e, std::string(name));
        m_Registry.emplace<TransformComponent>(e);
        auto& hierarchy = m_Registry.emplace<HierarchyComponent>(e);
        m_EntityLibrary[id] = e;

        if (parent != Null_Entity && IsValid(parent))
        {
            hierarchy.parent = parent;
            auto& parentHie = GetComponent<HierarchyComponent>(parent);
            if (parentHie.firstChild == Null_Entity) parentHie.firstChild = e;
            else
            {
                Entity lastSib = parentHie.firstChild;
                while (GetComponent<HierarchyComponent>(lastSib).nextSibling != Null_Entity)
                    lastSib = GetComponent<HierarchyComponent>(lastSib).nextSibling;
                
                GetComponent<HierarchyComponent>(lastSib).nextSibling = e;
                hierarchy.prevSibling = lastSib;
            }
        }
        else
        {
            auto view = View<HierarchyComponent>();
            Entity sib = Null_Entity; 
            for (auto entity : view)
            {
                if (entity == e) continue;
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
                GetComponent<HierarchyComponent>(sib).nextSibling = e;
                if (oldNext != Null_Entity) GetComponent<HierarchyComponent>(oldNext).prevSibling = e;
            }
        }
        return e;
    }

    void Scene::DestroyHierarchy(Entity entity)
    {
        if (!m_Registry.valid(entity)) return;

        auto hierarchy = GetComponent<HierarchyComponent>(entity);
        Entity currentChild = hierarchy.firstChild;
        while(currentChild != Null_Entity)
        {
            Entity next = GetComponent<HierarchyComponent>(currentChild).nextSibling;
            DestroyHierarchy(currentChild);
            currentChild = next;
        }

        if (hierarchy.prevSibling != Null_Entity) GetComponent<HierarchyComponent>(hierarchy.prevSibling).nextSibling = hierarchy.nextSibling;
        if (hierarchy.nextSibling != Null_Entity) GetComponent<HierarchyComponent>(hierarchy.nextSibling).prevSibling = hierarchy.prevSibling;

        if (hierarchy.parent != Null_Entity)
        {
            auto& parentHie = GetComponent<HierarchyComponent>(hierarchy.parent);
            if (parentHie.firstChild == entity) parentHie.firstChild = hierarchy.nextSibling;
        }

        if (HasComponent<AnimatorComponent>(entity))
        {
            auto rigModule = AnimationSystem::GetModule<RigModule>();
            if (rigModule)
            {
                auto& comp = GetComponent<AnimatorComponent>(entity);
                if (comp.Cache.IsValid()) rigModule->DestroyCache(comp.Cache);
                if (comp.CurrentPose.IsValid()) rigModule->DestroyPose(comp.CurrentPose);
            }
        }

        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!m_Registry.valid(entity)) return;
        BreakParent(entity);

        if (HasComponent<AnimatorComponent>(entity))
        {
            auto rigModule = AnimationSystem::GetModule<RigModule>();
            if (rigModule)
            {
                auto& comp = GetComponent<AnimatorComponent>(entity);
                if (comp.Cache.IsValid()) rigModule->DestroyCache(comp.Cache);
                if (comp.CurrentPose.IsValid()) rigModule->DestroyPose(comp.CurrentPose);
            }
        }
        
        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    void Scene::MakeParent(Entity child, Entity parent)
    {
        if (!IsValid(child)) return;
        if (parent != Null_Entity && !IsValid(parent)) return;

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
        if (!m_Registry.valid(entity)) return;
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

    bool Scene::IsValid(Entity entity) const
    {
        return m_Registry.valid(entity);
    }

    void Scene::DirtyScan()
    {
        auto view = View<TransformComponent>();
        for (auto entity : view)
            if (GetComponent<TransformComponent>(entity).Dirty)
                MarkDirty(entity);
    }

    void Scene::BreadthFirstSearch()
    {
        m_HierarchyLevels.clear();
        std::queue<std::pair<Entity, uint32_t>> Queue;
        auto view = View<HierarchyComponent>();
        for (auto entity : view)
        {
            if (GetComponent<HierarchyComponent>(entity).parent != Null_Entity) continue;
            auto& t = GetComponent<TransformComponent>(entity);
            if (t.Dirty || t.SubtreeDirty || (HasComponent<ColliderComponent>(entity) && GetComponent<ColliderComponent>(entity).Type != MotionType::Static)) Queue.push({entity, 0});
        }

        while (!Queue.empty())
        {
            auto [entity, depth] = Queue.front(); Queue.pop();
            if (m_HierarchyLevels.size() <= depth) m_HierarchyLevels.push_back({});
            m_HierarchyLevels[depth].push_back(entity);

            auto& parentT = GetComponent<TransformComponent>(entity);
            bool parentDirty = parentT.Dirty || parentT.SubtreeDirty;

            Entity child = GetComponent<HierarchyComponent>(entity).firstChild;
            while (child != Null_Entity)
            {
                auto& childT = GetComponent<TransformComponent>(child);
                if (parentDirty || childT.Dirty || childT.SubtreeDirty || HasComponent<ColliderComponent>(entity))
                {
                    if (parentDirty) childT.Dirty = true; 
                    Queue.push({child, depth + 1});
                }
                child = GetComponent<HierarchyComponent>(child).nextSibling;
            }
        }
    }

    Entity Scene::FindEntity(UUID id) const
    {
        auto it = m_EntityLibrary.find(id);
        if(it != m_EntityLibrary.end()) return it->second;
        return Null_Entity;
    }

    std::vector<Entity> Scene::FindEntity(const std::string& tag) const
    {
        std::vector<Entity> entList;
        const auto& view = View<TagComponent>();
        for (auto& entity : view)
        {
            auto& t = GetComponent<TagComponent>(entity);
            if (t.Tag == tag) entList.push_back(entity);
        }
        return entList;
    }

    UUID Scene::GetUUID(Entity entity) const
    {
         if (!m_Registry.valid(entity)) return UUID(0);
         return m_Registry.get<IDComponent>(entity).ID;
    }

    void Scene::CreateNodeEntity(const RegisteredScene& reg, int nodeIdx, Entity parentEntity)
    {
        const Node& node = reg.hierarchy->nodes[nodeIdx];
        Entity e = CreateEntity(node.name, parentEntity);
        auto& t       = GetComponent<TransformComponent>(e);
        t.Translation = node.translation;
        t.Rotation    = glm::normalize(node.rotation);
        t.Scale       = node.scale;
        MarkDirty(e);

        if (node.meshIdx >= 0 && node.meshIdx < (int)reg.meshIDs.size())
        {
            auto& component = AddComponent<MeshComponent>(e);
            component.Mesh = AssetManager::GetHandle(reg.meshIDs[node.meshIdx]);
            component.Materials.Resize(reg.meshMap[node.meshIdx].size());
            for(size_t i = 0; i < reg.meshMap[node.meshIdx].size(); i++)
            {
                auto& id = reg.meshMap[node.meshIdx][i];
                component.Materials.SetDefault(i, AssetManager::GetHandle(id));
            }
        }

        if (node.animatorIdx >= 0 && node.animatorIdx < (int)reg.animators.size())
        {
            const auto& animator = reg.animators[node.animatorIdx];
            auto& comp = AddComponent<AnimatorComponent>(e);
            comp.Skeleton = animator.skeleton;
            comp.Clips = animator.clips;

            if (!comp.Clips.empty())
            {
                auto rigModule = AnimationSystem::GetModule<RigModule>();
                auto* skeletonAsset = AssetManager::GetAsset<Skeleton>(comp.Skeleton);
                auto* clipAsset = AssetManager::GetAsset<Clip>(comp.Clips[0]);
                if (skeletonAsset && clipAsset)
                {
                    comp.Cache = rigModule->CreateCache(clipAsset->GetHandle());
                    comp.CurrentPose = rigModule->CreatePose(skeletonAsset->GetHandle());
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

    void Scene::ResolveBoneAttachments()
    {
        auto view = View<BoneAttachmentComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto& attach = GetComponent<BoneAttachmentComponent>(entity);
            auto& transform = GetComponent<TransformComponent>(entity);
            Entity animEnt = attach.AnimatorEntity;

            if (!IsValid(animEnt) || !HasComponent<AnimatorComponent>(animEnt)) continue;
            auto rigModule = AnimationSystem::GetModule<RigModule>();
            auto& animComp = GetComponent<AnimatorComponent>(animEnt);
            const glm::mat4& animatorWorld = GetComponent<TransformComponent>(animEnt).WorldTransform;
            
            auto* skeletonAsset = AssetManager::GetAsset<Skeleton>(animComp.Skeleton);
            if (skeletonAsset && animComp.CurrentPose.IsValid())
            {
                Handle<SkeletonTag> skelHnd = skeletonAsset->GetHandle();
                if (attach.JointIndex < 0) 
                    attach.JointIndex = rigModule->GetJointIndex(skelHnd, attach.JointName);
                
                auto [poseData, poseCount] = rigModule->GetPose(animComp.CurrentPose);
                if (poseData && attach.JointIndex >= 0 && (size_t)attach.JointIndex < poseCount)
                {
                    glm::mat4 ibm; 
                    rigModule->GetIBM(skelHnd, attach.JointIndex, ibm);
                    
                    glm::mat4 modelSpaceMat = poseData[attach.JointIndex] * glm::inverse(ibm);
                    glm::mat4 boneWorld = animatorWorld * modelSpaceMat;

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
    }

    void Scene::UpdateSubtreeTransforms(Entity entity, const glm::mat4& pTransform)
    {
        while (entity != Null_Entity && IsValid(entity))
        {
            auto& transform = GetComponent<TransformComponent>(entity);
            transform.WorldTransform = pTransform * transform.GetLocalTransform();
            if (HasComponent<HierarchyComponent>(entity))
            {
                auto& hierarchy = GetComponent<HierarchyComponent>(entity);
                if (hierarchy.firstChild != Null_Entity)
                    UpdateSubtreeTransforms(hierarchy.firstChild, transform.WorldTransform);
                entity = hierarchy.nextSibling;
            }
            else break; 
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

        bool isWorldTransformDirty = transform.Dirty || pDirty;
        bool hasRecalculatedWorld = false;

        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::mat4 boneMat = glm::mat4(1.0f);

        if (HasComponent<ColliderComponent>(entity))
        {
            auto& rbComp = GetComponent<ColliderComponent>(entity);
            Handle<BodyTag> handle = rbComp.ColliderHandle;

            if (isWorldTransformDirty)
            {
                transform.WorldTransform = pTransform * boneMat * transform.GetLocalTransform();
                hasRecalculatedWorld = true;

                glm::decompose(transform.WorldTransform, scale, rotation, translation, skew, perspective);
                glm::vec3 worldOffset = rotation * rbComp.ColliderOffset;
                PhysicsSystem::SetPhysTransform(handle, {translation + worldOffset, rotation});
            }
            else if (PhysicsSystem::GetBodyInfo(handle)->motionType == MotionType::Dynamic)
            {
                PhysTransform physTrans = PhysicsSystem::GetPhysTransform(handle);
                glm::vec3 localOffset = rbComp.ColliderOffset;
                glm::vec3 translation = physTrans.translation - (physTrans.rotation * localOffset);

                if (hierarchy.parent == Null_Entity)
                {
                    transform.Translation = translation;
                    transform.Rotation = physTrans.rotation;
                }
                else
                {
                    glm::mat4 invParent = glm::inverse(pTransform);
                    glm::quat parentRot = glm::quat_cast(pTransform);
                    transform.Translation = glm::vec3(invParent * glm::vec4(translation, 1.0f));
                    transform.Rotation = glm::inverse(parentRot) * physTrans.rotation;
                }

                isWorldTransformDirty = true;
                transform.Dirty = true;
            }
        }

        if (isWorldTransformDirty)
        {
            if (!hasRecalculatedWorld)
                transform.WorldTransform = pTransform * boneMat * transform.GetLocalTransform();

            bool needsDecompose = HasComponent<AudioSourceComponent>(entity) || HasComponent<LightComponent>(entity);
            if (needsDecompose)
                glm::decompose(transform.WorldTransform, scale, rotation, translation, skew, perspective);

            if (HasComponent<AudioSourceComponent>(entity))
                AudioSystem::SetPosition(GetComponent<AudioSourceComponent>(entity).SourceID, translation);

            if (HasComponent<LightComponent>(entity))
                GetComponent<LightComponent>(entity).Config.position = translation;

            transform.LastUpdate = m_CurrentFrame;
        }

        transform.Dirty = false;
        transform.SubtreeDirty = false;
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

    void Scene::Update(Timestep ts, EditorCamera* camera)
    {
        { 
            PhysicsSystem::Update(ts);
        }

        {
            m_CurrentFrame++;
            DirtyScan();
            BreadthFirstSearch();
            for (auto& level : m_HierarchyLevels) 
                JobSystem::ParallelFor(level.size(), m_Threshold, level, AE_MAKE_LAMBDA((&, this), (Entity entity), void,
                    this->UpdateTransform(entity); 
                ));
        }

        {
            auto scriptView = View<ScriptComponent>();

            for (auto entity : scriptView)
            {
                auto& instance = GetComponent<ScriptComponent>(entity).ScriptHandle;
                ScriptEngine::UpdateInstance(instance, ts);
            }

            ScriptEngine::FlushEvent();
        }

        { // render
            auto camView = View<CameraComponent>();
            
            CameraComponent* mainCamera = nullptr;
            for (auto entity : camView)
            {
                auto& camComp = GetComponent<CameraComponent>(entity);
                if (camComp.Primary)
                {
                    mainCamera = &camComp;
                    break;
                }
            }

            if (mainCamera || camera != nullptr)
            {

                glm::mat4 vp = (camera ? camera->GetViewProjection() : mainCamera->Camera.GetViewProjection());
                Utils::Frustum frustum = Utils::GetFrustum(vp);

                // light culling
                m_SceneLights.clear();
                auto lightView = View<LightComponent>();

                JobSystem::ParallelFor(lightView.size(), m_Threshold, lightView->data(), AE_MAKE_LAMBDA((&, this),  (Entity entity), void,
                    auto& lightComp = lightView.get<LightComponent>(entity);
                    auto& light = lightComp.Config;
                    if (light.type == LightType::None)
                    {
                        lightComp.Culled = true;
                        return;
                    }
                    if (light.type == LightType::Spot)
                    {
                        float r = light.range / (2.0f * glm::cos(light.outerCone));
                        glm::vec3 c = light.position + light.direction * r;
                        lightComp.Culled = !Utils::CheckSphereVisible(frustum, c, r);
                    }
                    else lightComp.Culled = false;
                ));
                for (auto entity : lightView)
                {
                    auto& lightComp = lightView.get<LightComponent>(entity);
                    auto& light = lightComp.Config;
                    if (!lightComp.Culled) m_SceneLights.push_back(light);
                }

                // mesh and animator culling
                auto meshView = View<MeshComponent>();

                JobSystem::ParallelFor(meshView.size(), m_Threshold, meshView->data(), AE_MAKE_LAMBDA((&, this), (Entity entity), void,
                    auto& transform = this->GetComponent<TransformComponent>(entity);
                    auto& meshcmp = this->GetComponent<MeshComponent>(entity);
                    Mesh* mesh = AssetManager::GetAsset<Mesh>(meshcmp.Mesh); 
                    if (!mesh)
                    {
                        meshcmp.Culled = true;
                        if (this->HasComponent<AnimatorComponent>(entity)) 
                            this->GetComponent<AnimatorComponent>(entity).Culled = true;
                        return; 
                    }
                    meshcmp.Culled = false;
                    if (this->HasComponent<AnimatorComponent>(entity)) 
                            this->GetComponent<AnimatorComponent>(entity).Culled = false;
                    glm::mat4 world = transform.WorldTransform;
                    glm::vec3 worldMin, worldMax;
                    if (HasComponent<AnimatorComponent>(entity) && mesh->HasAnimatedBounds())
                        Utils::TransformBound(mesh->GetAnimatedBoundsMin(), mesh->GetAnimatedBoundsMax(), world, worldMin, worldMax);
                    else Utils::TransformBound(mesh->GetBoundsMin(), mesh->GetBoundsMax(), world, worldMin, worldMax);
                    meshcmp.Culled = !Utils::CheckBoundVisible(frustum, worldMin, worldMax);
                    if (meshcmp.Culled && this->HasComponent<AnimatorComponent>(entity)) 
                            this->GetComponent<AnimatorComponent>(entity).Culled = true;
                ));

                auto rigModule = AnimationSystem::GetModule<RigModule>();
                auto animView  = View<AnimatorComponent>();
                rigModule->ClearTasks();

                for (auto entity : animView)
                {
                    auto& comp = GetComponent<AnimatorComponent>(entity);
                    if (comp.Clips.empty() || !comp.Cache.IsValid() || comp.Culled) continue;

                    auto* skeletonAsset = AssetManager::GetAsset<Skeleton>(comp.Skeleton);
                    auto* clipAsset = AssetManager::GetAsset<Clip>(comp.Clips[comp.ActiveClipIdx]);
                    if (!skeletonAsset || !clipAsset) continue;

                    auto skelHandle = skeletonAsset->GetHandle();
                    auto clipHandle = clipAsset->GetHandle();

                    if (comp.IsPlaying)
                    {
                        float duration = rigModule->GetDuration(clipHandle);
                        comp.CurrentTime += ts * comp.Speed;
                        if (comp.Loop && duration > 0.0f)
                            comp.CurrentTime = std::fmod(comp.CurrentTime, duration);
                        else
                        {
                            if (comp.CurrentTime >= duration)
                            {
                                comp.CurrentTime = duration;
                                comp.IsPlaying   = false;  
                            }
                        }
                    }

                    rigModule->ScheduleSample(skelHandle, clipHandle, comp.Cache, comp.CurrentPose, comp.CurrentTime);
                    rigModule->ScheduleFinalize(skelHandle, comp.CurrentPose);
                }

                rigModule->ProcessTasks();

                ResolveBoneAttachments();

                // render
                if (camera != nullptr) Renderer::BeginScene(*camera, m_SceneLights); 
                else Renderer::BeginScene(mainCamera->Camera, m_SceneLights); 

                for (auto entity : meshView)
                {
                    auto& transform = GetComponent<TransformComponent>(entity);
                    auto& meshcmp = GetComponent<MeshComponent>(entity);
                    Mesh* mesh = AssetManager::GetAsset<Mesh>(meshcmp.Mesh); if (!mesh) continue;
                    UUID animatorID = UUID(0);
                    Handle<PoseTag> pose = Handle<PoseTag>::MakeInvalid();
                    if (HasComponent<AnimatorComponent>(entity)) pose = GetComponent<AnimatorComponent>(entity).CurrentPose;
                    if (!meshcmp.Culled) 
                        Renderer::DrawMesh(mesh, meshcmp.Materials.CachedPtr, pose, transform.WorldTransform);
                }

                Renderer::EndScene();

                // draw debug box
                for (auto entity : meshView)
                {
                    auto& meshcmp = GetComponent<MeshComponent>(entity);
                    if (!meshcmp.ShowBounds) continue;

                    auto& transform = GetComponent<TransformComponent>(entity);
                    Mesh* mesh = AssetManager::GetAsset<Mesh>(meshcmp.Mesh); if (!mesh) continue;

                    glm::mat4 world = transform.WorldTransform;
                    glm::vec3 worldMin, worldMax;
                    if (HasComponent<AnimatorComponent>(entity) && mesh->HasAnimatedBounds())
                        Utils::TransformBound(mesh->GetAnimatedBoundsMin(), mesh->GetAnimatedBoundsMax(), world, worldMin, worldMax);
                    else Utils::TransformBound(mesh->GetBoundsMin(), mesh->GetBoundsMax(), world, worldMin, worldMax);
                    if (!Utils::CheckBoundVisible(frustum, worldMin, worldMax)) continue;
                    Renderer::RenderBox(worldMin, worldMax, glm::mat4(1.0f), RED);
                }

                auto rbView = View<ColliderComponent>();
                for (auto entity : rbView)
                {
                    auto& component = GetComponent<ColliderComponent>(entity);
                    if (!component.Visible) continue;
                    Handle<BodyTag> handle = component.ColliderHandle;
                    PhysTransform pt = PhysicsSystem::GetPhysTransform(handle);
                    glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0f), pt.translation)
                                                * glm::toMat4(pt.rotation);
                    if (component.Shape == ColliderShape::Sphere)
                    {
                        float radius = component.Size.x;
                        Renderer::RenderSphere(radius, colliderTransform, GREEN);
                    }
                    if (component.Shape == ColliderShape::Box)
                    {
                        glm::vec3 bMin = -component.Size;
                        glm::vec3 bMax =  component.Size;
                        Renderer::RenderBox(bMin, bMax, colliderTransform, GREEN);
                    }
                    if (component.Shape == ColliderShape::Capsule)
                    {
                        float radius  = component.Size.x;
                        float halfCyl = std::max((component.Size.y * 0.5f) - radius, 0.0f);
                        Renderer::RenderCapsule(radius, halfCyl, colliderTransform, GREEN);
                    }
                }
            }
        }
    }
}