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
        inline void GetWorldPosAndRot(const glm::mat4& matrix, glm::vec3& outPos, glm::quat& outRot)
        {
            outPos = glm::vec3(matrix[3]);
            glm::vec3 x = glm::normalize(glm::vec3(matrix[0]));
            glm::vec3 y = glm::normalize(glm::vec3(matrix[1]));
            glm::vec3 z = glm::normalize(glm::vec3(matrix[2]));
            outRot = glm::quat_cast(glm::mat3(x, y, z));
        }
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

        inline void GetTRS(const glm::mat4& matrix, glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale)
        {
            outPos = glm::vec3(matrix[3]);
            glm::vec3 col0(matrix[0]);
            glm::vec3 col1(matrix[1]);
            glm::vec3 col2(matrix[2]);
            outScale.x = glm::length(col0);
            outScale.y = glm::length(col1);
            outScale.z = glm::length(col2);
            glm::vec3 x = (outScale.x > 0.00001f) ? (col0 / outScale.x) : glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 y = (outScale.y > 0.00001f) ? (col1 / outScale.y) : glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 z = (outScale.z > 0.00001f) ? (col2 / outScale.z) : glm::vec3(0.0f, 0.0f, 1.0f);
            outRot = glm::quat_cast(glm::mat3(x, y, z));
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
                Entity a = FromNumber(physys->GetUserData(m_PhysicsInstance, ev.bodyA));
                Entity b = FromNumber(physys->GetUserData(m_PhysicsInstance, ev.bodyB));

                if (a == Null_Entity || b == Null_Entity) return;
                if (HasComponent<ScriptComponent>(a)) 
                {
                    auto& cmp = this->GetComponent<ScriptComponent>(a);
                    if (cmp.IsActive)
                    {
                        CollisionData data;
                        data.contactPoint = ev.contactPoint;
                        data.contactNormal = ev.contactNormal;
                        data.entityID = GetComponent<IDComponent>(b).ID;
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
                        data.entityID = GetComponent<IDComponent>(a).ID;
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

    std::vector<Entity> Scene::FindEntity(std::string_view tag) const
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
        auto* jobsys = ServiceManager::GetService<JobSystem>(); jobsys->FlushCompletions();
        auto* physys = ServiceManager::GetService<PhysicsSystem>();

        { // destroy queue flush
            for (auto& info : m_DestroyQueue)
            {
                if (info.clearHierarchy) ExcDestroyHierarchy(info.entity);
                else ExcDestroyEntity(info.entity, info.repairHie);
            }
            m_DestroyQueue.clear();
        }

        { // update transform
            m_CurrentFrame++;
            DirtyScan();
            BreadthFirstSearch();
            physys->UpdateInstance(m_PhysicsInstance, ts);
            for (auto& level : m_HierarchyLevels) 
                jobsys->ParallelFor(level.size(), m_Threshold, level, AE_MAKE_LAMBDA((&, this), (Entity entity), void,
                    this->UpdateTransform(entity); 
                ));
        }

        { // update physics
            auto view = View<ColliderComponent>();
            for (auto entity : view)
            {
                auto& rbComp = GetComponent<ColliderComponent>(entity);
                Handle<RigidBody>& handle = rbComp.ColliderHandle;
                physys->SetActive(m_PhysicsInstance, handle, rbComp.IsActive);

                if (!handle.IsValid())
                {
                    auto& t = GetComponent<TransformComponent>(entity);
                    glm::vec3 translation, scale; 
                    glm::quat rotation;
                    Utils::GetTRS(t.WorldTransform, translation, rotation, scale);

                    BodyConfig config;
                    config.motionType = rbComp.Type;
                    config.shape = rbComp.Shape;
                    config.size = glm::max(rbComp.Size * scale, glm::vec3(0.5f));
                    config.transform = { translation + (rotation * rbComp.ColliderOffset), rotation };
                    config.mass = rbComp.Mass;
                    config.friction = rbComp.Friction;
                    config.restitution = rbComp.Restitution;
                    config.isSensor = rbComp.IsSensor;

                    handle = physys->CreateBody(m_PhysicsInstance, config);
                    physys->SetUserData(m_PhysicsInstance, handle, ToNumber(entity));
                }
                physys->SetActive(m_PhysicsInstance, handle, rbComp.IsActive);
            }
        }

        { // update scirpts
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

        // { //update audio
        //     auto audioView = View<AudioSourceComponent>();
        //     auto* audsys = ServiceManager::GetService<AudioSystem>();
        //     for (auto entity : audioView)
        //     {
        //         auto& audio = GetComponent<AudioSourceComponent>(entity);
        //         if (!audio.SourceHandle.IsValid() && !audio.Path.empty())
        //         {
        //             audio.SourceHandle = audsys->CreateSource(audio.Path, audio.Type);
        //             if (audio.SourceHandle.IsValid())
        //             {
        //                 audsys->SetVolume(audio.SourceHandle, audio.Volume);
        //                 audsys->SetPan(audio.SourceHandle, audio.Pan);
        //                 audsys->SetPlaybackSpeed(audio.SourceHandle, audio.PlaybackSpeed);
        //                 audsys->SetLooping(audio.SourceHandle, audio.Looping);
        //                 if (audio.Type == AudioType::Audio3D)
        //                 {
        //                     audsys->SetDistance(audio.SourceHandle,
        //                         audio.Config3D.minDistance, audio.Config3D.maxDistance);
        //                     audsys->SetAttenuation(audio.SourceHandle, audio.Config3D.attenuation);
        //                 }
        //                 if (audio.PlayOnStart)
        //                 {
        //                     audsys->Play(audio.SourceHandle);
        //                     audio.IsPlaying = true;
        //                 }
        //             }
        //         }
        //         else if (audio.SourceHandle.IsValid())
        //         {
        //             audsys->SetVolume(audio.SourceHandle,        audio.Volume);
        //             audsys->SetPan(audio.SourceHandle,           audio.Pan);
        //             audsys->SetPlaybackSpeed(audio.SourceHandle, audio.PlaybackSpeed);
        //             audsys->SetLooping(audio.SourceHandle,       audio.Looping);
        //         }
        //     }
        // }

        { // render
            auto camView = View<CameraComponent>();
            auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
            auto* rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            
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
                    AMesh* mesh = asset_manager->GetAsset<AMesh>(meshcmp.Mesh); 
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

                auto animView  = View<AnimatorComponent>();

                rigModule->ClearTasks();
                for (auto entity : animView)
                {
                    auto& comp = GetComponent<AnimatorComponent>(entity);
                    if (comp.Clips.empty() || comp.Culled || !comp.RunSampling) continue;

                    auto* skeletonAsset = asset_manager->GetAsset<ASkeleton>(comp.Skeleton);
                    auto* clipAsset = asset_manager->GetAsset<AClip>(comp.Clips[comp.ActiveClipIdx]);
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
                    AMesh* mesh = asset_manager->GetAsset<AMesh>(meshcmp.Mesh); if (!mesh) continue;

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
                    switch (component.Shape)
                    {
                        case ColliderShape::Sphere:
                        {
                            float radius = component.Size.x;
                            renderer->RenderSphere(radius, colliderTransform, GREEN);
                            break;
                        }
                        case ColliderShape::Box:
                        {
                            glm::vec3 bMin = -component.Size;
                            glm::vec3 bMax =  component.Size;
                            renderer->RenderBox(bMin, bMax, colliderTransform, GREEN);
                            break;
                        }
                        case ColliderShape::Capsule:
                        {
                            float radius  = component.Size.x;
                            float halfCyl = std::max((component.Size.y * 0.5f) - radius, 0.0f);
                            renderer->RenderCapsule(radius, halfCyl, colliderTransform, GREEN);
                            break;
                        }
                        default: break;
                    }
                }
            }
        }
    }
}