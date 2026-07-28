#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Core/ServiceManager.h"
#include <glm/gtx/matrix_decompose.hpp>

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

    void Scene::Init()
    {
        m_EntityLibrary.reserve(32);
        m_SceneLights.reserve(32);
        m_DestroyQueue.reserve(32);
        auto* physys = ServiceManager::GetService<PhysicsSystem>();
        m_PhysicsInstance = physys->CreateInstance();
        physys->RegisterCallback(m_PhysicsInstance, [this, physys](const CollisionEvent& ev) 
        {
            if (ev.type == CollisionType::Enter || ev.type == CollisionType::Exit) 
            {
                UUID aID = physys->GetUUID(m_PhysicsInstance, ev.bodyA);
                UUID bID = physys->GetUUID(m_PhysicsInstance, ev.bodyB);
                Entity a = this->FindEntity(aID);
                Entity b = this->FindEntity(bID);

                if (a == Null_Entity || b == Null_Entity) return;
                if (HasComponent<ScriptComponent>(a)) 
                {
                    auto& cmp = this->GetComponent<ScriptComponent>(a);
                    if (cmp.IsActive)
                    {
                        CollisionData data;
                        data.contactPoint = ev.contactPoint;
                        data.contactNormal = ev.contactNormal;
                        data.entityID = bID;
                        data.type = ev.type;

                        Handle<ScriptInstance> handle = cmp.ScriptHandle;
                        ServiceManager::GetService<ScriptEngine>()->OnInstanceCollision(handle, data);
                    }
                }

                if (HasComponent<ScriptComponent>(b)) 
                {
                    auto& cmp = this->GetComponent<ScriptComponent>(b);
                    if (cmp.IsActive)
                    {
                        CollisionData data;
                        data.contactPoint = ev.contactPoint;
                        data.contactNormal = -ev.contactNormal; 
                        data.entityID = aID;
                        data.type = ev.type;

                        Handle<ScriptInstance> handle = cmp.ScriptHandle;
                        ServiceManager::GetService<ScriptEngine>()->OnInstanceCollision(handle, data);
                    }
                }
            }
        });
    }

    void Scene::Shutdown()
    {
        for (auto entity : View<HierarchyComponent>())
            DestroyEntity(entity, false);

        for (auto& info : m_DestroyQueue)
            ExcDestroyEntity(info.entity, info.repairHie);

        m_EntityLibrary.clear();
        m_SceneLights.clear();
        m_HierarchyLevels.clear();
        m_DestroyQueue.clear();

        ServiceManager::GetService<PhysicsSystem>()->DestroyInstance(m_PhysicsInstance);
    }

    void Scene::ImportPrefab(Entity entity, const Prefab& prefab, bool override)
    {
        LoadComponent(entity, prefab.tag, override);
        LoadComponent(entity, prefab.transform, override);
        LoadComponent(entity, prefab.hierarchy, override);
        LoadComponent(entity, prefab.mesh, override);
        LoadComponent(entity, prefab.light, override);
        LoadComponent(entity, prefab.camera, override);
        LoadComponent(entity, prefab.animator, override);
        LoadComponent(entity, prefab.collider, override);
        LoadComponent(entity, prefab.script, override);
        LoadComponent(entity, prefab.boneAttach, override);
    }

    Prefab Scene::ExportPrefab(Entity entity) const
    {
        Prefab prefab;

        if (HasComponent<TagComponent>(entity))
        {
            prefab.tag.IsExits = true;
            prefab.tag.data    = GetComponent<TagComponent>(entity);
        }
        if (HasComponent<TransformComponent>(entity))
        {
            prefab.transform.IsExits = true;
            prefab.transform.data    = GetComponent<TransformComponent>(entity);
        }
        if (HasComponent<HierarchyComponent>(entity))
        {
            prefab.hierarchy.IsExits = true;
            prefab.hierarchy.data    = GetComponent<HierarchyComponent>(entity);
        }
        if (HasComponent<MeshComponent>(entity))
        {
            prefab.mesh.IsExits = true;
            prefab.mesh.data    = GetComponent<MeshComponent>(entity);
        }
        if (HasComponent<LightComponent>(entity))
        {
            prefab.light.IsExits = true;
            prefab.light.data    = GetComponent<LightComponent>(entity);
        }
        if (HasComponent<CameraComponent>(entity))
        {
            prefab.camera.IsExits = true;
            prefab.camera.data    = GetComponent<CameraComponent>(entity);
        }
        if (HasComponent<AnimatorComponent>(entity))
        {
            prefab.animator.IsExits = true;
            prefab.animator.data    = GetComponent<AnimatorComponent>(entity);
        }
        if (HasComponent<ColliderComponent>(entity))
        {
            prefab.collider.IsExits = true;
            prefab.collider.data    = GetComponent<ColliderComponent>(entity);
        }
        if (HasComponent<ScriptComponent>(entity))
        {
            prefab.script.IsExits = true;
            prefab.script.data    = GetComponent<ScriptComponent>(entity);
        }
        if (HasComponent<BoneAttachmentComponent>(entity))
        {
            prefab.boneAttach.IsExits = true;
            prefab.boneAttach.data    = GetComponent<BoneAttachmentComponent>(entity);
        }

        return prefab;
    }

    bool Scene::IsValid(Entity entity) const
    {
        return m_Registry.valid(entity);
    }


    Entity Scene::FindEntity(UUID id) const
    {
        auto it = m_EntityLibrary.find(id);
        if(it != m_EntityLibrary.end()) return it->second;
        AE_CORE_INFO("Find: null");
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

    void Scene::ResolveBoneAttachments()
    {
        auto view = View<BoneAttachmentComponent, TransformComponent>();
        auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
        for (auto entity : view)
        {
            auto& attach = GetComponent<BoneAttachmentComponent>(entity);
            auto& transform = GetComponent<TransformComponent>(entity);
            Entity animEnt = attach.AnimatorEntity;

            if (!IsValid(animEnt) || !HasComponent<AnimatorComponent>(animEnt)) continue;
            auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            auto& animComp = GetComponent<AnimatorComponent>(animEnt);
            const glm::mat4& animatorWorld = GetComponent<TransformComponent>(animEnt).WorldTransform;
            
            auto* skeletonAsset = asset_manager->GetAsset<Skeleton>(animComp.Skeleton);
            if (skeletonAsset && animComp.CurrentPose.IsValid())
            {
                Handle<RSkeleton> skel = skeletonAsset->m_Handle;
                if (attach.JointIndex < 0) 
                    attach.JointIndex = rigModule->GetJointIndex(skel, attach.JointName);
                
                auto [poseData, poseCount] = rigModule->GetPose(animComp.CurrentPose);
                if (poseData && attach.JointIndex >= 0 && (size_t)attach.JointIndex < poseCount)
                {
                    glm::mat4 ibm; 
                    rigModule->GetIBM(skel, attach.JointIndex, ibm);
                    
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

        if (HasComponent<ColliderComponent>(entity))
        {
            auto* physys = ServiceManager::GetService<PhysicsSystem>();
            auto& rbComp = GetComponent<ColliderComponent>(entity);
            Handle<RigidBody>& handle = rbComp.ColliderHandle;

            if (handle.IsValid()) 
            {
                if (isWorldTransformDirty)
                {
                    transform.WorldTransform = pTransform * transform.GetLocalTransform();
                    hasRecalculatedWorld = true;

                    glm::vec3 scale, translation, skew; glm::quat rotation; glm::vec4 perspective;
                    glm::decompose(transform.WorldTransform, scale, rotation, translation, skew, perspective);
                    glm::vec3 worldOffset = rotation * rbComp.ColliderOffset;
                    PhysTransform target = {translation + worldOffset, rotation};
                    if (physys->GetBodyInfo(m_PhysicsInstance, handle)->motionType == MotionType::Kinematic)
                    {
                        if (physys->CanMove(m_PhysicsInstance, handle, target)) physys->SetPhysTransform(m_PhysicsInstance, handle, target);
                        else
                        {
                            PhysTransform physTrans = physys->GetPhysTransform(m_PhysicsInstance, handle);
                            glm::vec3 trans = physTrans.translation - (physTrans.rotation * rbComp.ColliderOffset);

                            if (hierarchy.parent == Null_Entity)
                            {
                                transform.Translation = trans;
                                transform.Rotation = physTrans.rotation;
                            }
                            else
                            {
                                glm::mat4 invParent = glm::inverse(pTransform);
                                glm::quat parentRot = glm::quat_cast(pTransform);
                                transform.Translation = glm::vec3(invParent * glm::vec4(trans, 1.0f));
                                transform.Rotation = glm::inverse(parentRot) * physTrans.rotation;
                            }
                            transform.Dirty = true;
                        }
                    }
                    else physys->SetPhysTransform(m_PhysicsInstance, handle, target);
                }
                else if (physys->GetBodyInfo(m_PhysicsInstance, handle)->motionType == MotionType::Dynamic)
                {
                    PhysTransform physTrans = physys->GetPhysTransform(m_PhysicsInstance, handle);
                    glm::vec3 localOffset = rbComp.ColliderOffset;
                    glm::vec3 trans = physTrans.translation - (physTrans.rotation * localOffset);

                    if (hierarchy.parent == Null_Entity)
                    {
                        transform.Translation = trans;
                        transform.Rotation = physTrans.rotation;
                    }
                    else
                    {
                        glm::mat4 invParent = glm::inverse(pTransform);
                        glm::quat parentRot = glm::quat_cast(pTransform);
                        transform.Translation = glm::vec3(invParent * glm::vec4(trans, 1.0f));
                        transform.Rotation = glm::inverse(parentRot) * physTrans.rotation;
                    }

                    isWorldTransformDirty = true;
                    transform.Dirty = true;
                }
            }
        }

        if (isWorldTransformDirty)
        {
            if (!hasRecalculatedWorld)
                transform.WorldTransform = pTransform * transform.GetLocalTransform();

            if (HasComponent<AudioSourceComponent>(entity))
            {
                auto& audio = GetComponent<AudioSourceComponent>(entity);
                if (audio.SourceHandle.IsValid())
                    ServiceManager::GetService<AudioSystem>()->SetPosition(audio.SourceHandle, glm::vec3(transform.WorldTransform[3]));
            }

            if (HasComponent<LightComponent>(entity))
                GetComponent<LightComponent>(entity).Config.position = glm::vec3(transform.WorldTransform[3]);

            transform.LastUpdate = m_CurrentFrame;
        }

        transform.Dirty = false;
        transform.SubtreeDirty = false;
    }

    glm::vec3 Scene::GetWorldPosition(Entity entity)
    {
        if (entity == Null_Entity || !HasComponent<TransformComponent>(entity)) return glm::vec3(0.0f);

        glm::mat4 ans = glm::mat4(1.0f);
        Entity current = entity;

        while (current != Null_Entity)
        {
            auto& transformComp = GetComponent<TransformComponent>(current);
            auto& hierarchy = GetComponent<HierarchyComponent>(current);
            ans = transformComp.GetLocalTransform() * ans;
            current = hierarchy.parent; 
        }
        return glm::vec3(ans[3]);
    }

    void Scene::Update(Timestep ts, EditorCamera* camera)
    {
        auto* jobsys = ServiceManager::GetService<JobSystem>();
        auto* physys = ServiceManager::GetService<PhysicsSystem>();
        {   
            for (auto& info : m_DestroyQueue)
            {
                if (info.clearHierarchy) ExcDestroyHierarchy(info.entity);
                else ExcDestroyEntity(info.entity, info.repairHie);
            }
            m_DestroyQueue.clear();
        }

        {
            m_CurrentFrame++;
            DirtyScan();
            BreadthFirstSearch();
            for (auto& level : m_HierarchyLevels) 
                jobsys->ParallelFor(level.size(), m_Threshold, level, AE_MAKE_LAMBDA((&, this), (Entity entity), void,
                    this->UpdateTransform(entity); 
                ));
        }

        {
            auto view = View<ColliderComponent>();
            for (auto entity : view)
            {
                auto& rbComp = GetComponent<ColliderComponent>(entity);
                Handle<RigidBody>& handle = rbComp.ColliderHandle;

                if (!handle.IsValid())
                {
                    glm::vec3 scale, translation, skew;
                    glm::quat rotation;
                    glm::vec4 perspective;
                    glm::decompose(GetComponent<TransformComponent>(entity).WorldTransform, scale, rotation, translation, skew, perspective);
                    glm::vec3 worldOffset = rotation * rbComp.ColliderOffset;

                    BodyConfig config;
                    config.motionType = rbComp.Type;
                    config.shape = rbComp.Shape;
                    config.size = glm::max(rbComp.Size * scale, glm::vec3(0.5f));
                    config.transform = { translation + worldOffset, rotation };
                    config.mass = rbComp.Mass;
                    config.friction = rbComp.Friction;
                    config.restitution = rbComp.Restitution;
                    config.isSensor = rbComp.IsSensor;

                    handle = physys->CreateBody(m_PhysicsInstance, config);
                    if (rbComp.Type == MotionType::Dynamic)
                        physys->SetActive(m_PhysicsInstance, handle, true);

                    physys->SetUUID(m_PhysicsInstance, handle, GetComponent<IDComponent>(entity).ID);
                }
                else physys->SetActive(m_PhysicsInstance, handle, rbComp.IsActive);
            }
            physys->UpdateInstance(m_PhysicsInstance, ts);
        }

        {
            auto scriptView = View<ScriptComponent>();
            auto* script_engine = ServiceManager::GetService<ScriptEngine>();
            if (script_engine->IsExecOrderChanged())
                Sort<ScriptComponent>([script_engine](const ScriptComponent& a, const ScriptComponent& b) 
                {return script_engine->GetExecOrder(a.ScriptHandle) <  script_engine->GetExecOrder(b.ScriptHandle);});

            for (auto entity : scriptView)
            {
                auto& cmp = GetComponent<ScriptComponent>(entity);
                auto& instance = cmp.ScriptHandle;
                if (cmp.PendingStart)
                {
                    cmp.PendingStart = false;
                    script_engine->StartInstance(cmp.ScriptHandle);
                }
                else if (cmp.IsActive)
                    script_engine->UpdateInstance(instance, ts);
            }

            script_engine->Update(ts);
        }

        {
            auto audioView = View<AudioSourceComponent>();
            auto* audsys = ServiceManager::GetService<AudioSystem>();
            for (auto entity : audioView)
            {
                auto& audio = GetComponent<AudioSourceComponent>(entity);
                if (!audio.SourceHandle.IsValid() && !audio.Path.empty())
                {
                    audio.SourceHandle = audsys->CreateSource(audio.Path, audio.Type);
                    if (audio.SourceHandle.IsValid())
                    {
                        audsys->SetVolume(audio.SourceHandle, audio.Volume);
                        audsys->SetPan(audio.SourceHandle, audio.Pan);
                        audsys->SetPlaybackSpeed(audio.SourceHandle, audio.PlaybackSpeed);
                        audsys->SetLooping(audio.SourceHandle, audio.Looping);
                        if (audio.Type == AudioType::Audio3D)
                        {
                            audsys->SetDistance(audio.SourceHandle,
                                audio.Config3D.minDistance, audio.Config3D.maxDistance);
                            audsys->SetAttenuation(audio.SourceHandle, audio.Config3D.attenuation);
                        }
                        if (audio.PlayOnStart)
                        {
                            audsys->Play(audio.SourceHandle);
                            audio.IsPlaying = true;
                        }
                    }
                }
                else if (audio.SourceHandle.IsValid())
                {
                    audsys->SetVolume(audio.SourceHandle,        audio.Volume);
                    audsys->SetPan(audio.SourceHandle,           audio.Pan);
                    audsys->SetPlaybackSpeed(audio.SourceHandle, audio.PlaybackSpeed);
                    audsys->SetLooping(audio.SourceHandle,       audio.Looping);
                }
            }
        }

        { // render
            auto camView = View<CameraComponent>();
            auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
            
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

                jobsys->ParallelFor(lightView.size(), m_Threshold, lightView->data(), AE_MAKE_LAMBDA((&, this),  (Entity entity), void,
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

                jobsys->ParallelFor(meshView.size(), m_Threshold, meshView->data(), AE_MAKE_LAMBDA((&, this), (Entity entity), void,
                    auto& transform = this->GetComponent<TransformComponent>(entity);
                    auto& meshcmp = this->GetComponent<MeshComponent>(entity);
                    Mesh* mesh = asset_manager->GetAsset<Mesh>(meshcmp.Mesh); 
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
                    if (HasComponent<AnimatorComponent>(entity) && mesh->m_HasAnimatedBounds)
                        Utils::TransformBound(mesh->m_AnimatedBoundsMin, mesh->m_AnimatedBoundsMax, world, worldMin, worldMax);
                    else Utils::TransformBound(mesh->m_BoundsMin, mesh->m_BoundsMax, world, worldMin, worldMax);
                    meshcmp.Culled = !Utils::CheckBoundVisible(frustum, worldMin, worldMax);
                    if (meshcmp.Culled && this->HasComponent<AnimatorComponent>(entity)) 
                            this->GetComponent<AnimatorComponent>(entity).Culled = true;
                ));

                auto rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
                auto animView  = View<AnimatorComponent>();

                rigModule->ClearTasks();
                for (auto entity : animView)
                {
                    auto& comp = GetComponent<AnimatorComponent>(entity);
                    if (comp.Clips.empty() || comp.Culled || !comp.RunSampling) continue;

                    auto* skeletonAsset = asset_manager->GetAsset<Skeleton>(comp.Skeleton);
                    auto* clipAsset = asset_manager->GetAsset<Clip>(comp.Clips[comp.ActiveClipIdx]);
                    if (!skeletonAsset || !clipAsset) continue;

                    if (!comp.Cache.IsValid()) comp.Cache = rigModule->CreateCache(clipAsset->m_Handle);
                    if (!comp.CurrentPose.IsValid()) comp.CurrentPose = rigModule->CreatePose(skeletonAsset->m_Handle);

                    if (comp.CacheDirty)
                    {
                        rigModule->RepairCache(comp.Cache, clipAsset->m_Handle);
                        comp.CacheDirty = false;
                    }

                    auto skelHandle = skeletonAsset->m_Handle;
                    auto clipHandle = clipAsset->m_Handle;

                    if (comp.IsPlaying)
                    {
                        float duration = clipAsset->m_Duration;
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

                rigModule->ClearTasks();
                for (auto entity : animView)
                {
                    auto& comp = GetComponent<AnimatorComponent>(entity);
                    if (comp.Clips.empty() || comp.Culled || !comp.RunPostEval) continue;
                    if (comp.onPostEvaluate)
                        comp.onPostEvaluate(entity, rigModule, float(ts));
                }
                rigModule->ProcessTasks();

                ResolveBoneAttachments();

                // render
                auto* renderer = ServiceManager::GetService<Renderer>(); 
                if (camera != nullptr) renderer->BeginScene(*camera, m_SceneLights.data(), m_SceneLights.size()); 
                else renderer->BeginScene(mainCamera->Camera, m_SceneLights.data(), m_SceneLights.size()); 

                for (auto entity : meshView)
                {
                    auto& transform = GetComponent<TransformComponent>(entity);
                    auto& meshcmp = GetComponent<MeshComponent>(entity);
                    Handle<Pose> pose = Handle<Pose>::MakeInvalid();
                    if (HasComponent<AnimatorComponent>(entity)) pose = GetComponent<AnimatorComponent>(entity).CurrentPose;
                    if (!meshcmp.Culled) 
                        renderer->DrawMesh(meshcmp.Mesh, meshcmp.SharedSheet, pose, transform.WorldTransform);
                }

                renderer->EndScene();

                // draw debug box
                for (auto entity : meshView)
                {
                    auto& meshcmp = GetComponent<MeshComponent>(entity);
                    if (!meshcmp.ShowBounds) continue;

                    auto& transform = GetComponent<TransformComponent>(entity);
                    Mesh* mesh = asset_manager->GetAsset<Mesh>(meshcmp.Mesh); if (!mesh) continue;

                    glm::mat4 world = transform.WorldTransform;
                    glm::vec3 worldMin, worldMax;
                    if (HasComponent<AnimatorComponent>(entity) && mesh->m_HasAnimatedBounds)
                        Utils::TransformBound(mesh->m_AnimatedBoundsMin, mesh->m_AnimatedBoundsMax, world, worldMin, worldMax);
                    else Utils::TransformBound(mesh->m_BoundsMin, mesh->m_BoundsMax, world, worldMin, worldMax);
                    if (!Utils::CheckBoundVisible(frustum, worldMin, worldMax)) continue;
                    renderer->RenderBox(worldMin, worldMax, glm::mat4(1.0f), RED);
                }

                auto rbView = View<ColliderComponent>();
                for (auto entity : rbView)
                {
                    auto& component = GetComponent<ColliderComponent>(entity);
                    if (!component.Visible || !component.ColliderHandle.IsValid()) continue;
                    Handle<RigidBody> handle = component.ColliderHandle;
                    PhysTransform pt = physys->GetPhysTransform(m_PhysicsInstance, handle);
                    glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0f), pt.translation)
                                                * glm::toMat4(pt.rotation);
                    if (component.Shape == ColliderShape::Sphere)
                    {
                        float radius = component.Size.x;
                        renderer->RenderSphere(radius, colliderTransform, GREEN);
                    }
                    if (component.Shape == ColliderShape::Box)
                    {
                        glm::vec3 bMin = -component.Size;
                        glm::vec3 bMax =  component.Size;
                        renderer->RenderBox(bMin, bMax, colliderTransform, GREEN);
                    }
                    if (component.Shape == ColliderShape::Capsule)
                    {
                        float radius  = component.Size.x;
                        float halfCyl = std::max((component.Size.y * 0.5f) - radius, 0.0f);
                        renderer->RenderCapsule(radius, halfCyl, colliderTransform, GREEN);
                    }
                }
            }
        }
    }
}