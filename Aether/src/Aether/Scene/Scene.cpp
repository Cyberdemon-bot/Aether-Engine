#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Renderer/Renderer.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Scene/TransformMath.h"

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

    }

    void Scene::Init()
    {
        m_EntityLibrary.reserve(32);
        m_SceneLights.reserve(32);
        m_DestroyQueue.reserve(32);
        m_HierarchyBuffer.reserve(32);
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
        m_FirstRoot = Null_Entity;
        m_LastRoot = Null_Entity;

        m_EntityLibrary.clear();
        m_SceneLights.clear(); m_SceneLights.shrink_to_fit();
        m_HierarchyBuffer.clear(); m_HierarchyBuffer.shrink_to_fit();
        m_DestroyQueue.clear(); m_DestroyQueue.shrink_to_fit();

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
        const auto& storage = Storage<TagComponent>();
        for (const auto& [entity, cmp] : storage.each())
            if (cmp.Tag == tag) 
                entList.push_back(entity);
        return entList;
    }

    glm::vec3 Scene::GetWorldPosition(Entity entity)
    {
        if (entity == Null_Entity || !IsValid(entity) || !HasComponent<TransformComponent>(entity)) return glm::vec3(0.0f);

        glm::mat4 ans = glm::mat4(1.0f);
        Entity current = entity;

        while (current != Null_Entity && IsValid(current) &&
               HasComponent<TransformComponent>(current) && HasComponent<HierarchyComponent>(current))
        {
            auto& transformComp = GetComponent<TransformComponent>(current);
            auto& hierarchy = GetComponent<HierarchyComponent>(current);
            ans = transformComp.GetLocalTransform() * ans;
            current = hierarchy.parent; 
        }
        return glm::vec3(ans[3]);
    }

    void Scene::OnTick(Timestep ts)
    {
        auto* physys = ServiceManager::GetService<PhysicsSystem>();
        auto* jobsys = ServiceManager::GetService<JobSystem>(); jobsys->FlushCompletions();

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

            size_t offset = 0;
            const size_t totalSize = m_HierarchyBuffer.size();
            while (offset < totalSize)
            {
                uint32_t count = ToNumber32(m_HierarchyBuffer[offset]);
                if (count > 0)
                {
                    Entity* levelData = &m_HierarchyBuffer[offset + 1];
                    jobsys->ParallelFor(count, m_Threshold, levelData, [&](Entity entity) { UpdateTransform(entity); });
                }
                offset += 1 + count;
            }
        }

        { // update physics
            auto colliderGroup = Group<ColliderComponent>(get<TransformComponent>);
            colliderGroup.each([&physys, this](Entity entity, ColliderComponent& rbComp, const TransformComponent& tComp) 
            {
                Handle<RigidBody>& handle = rbComp.ColliderHandle;
                if (!handle.IsValid())
                {
                    glm::vec3 translation, scale; 
                    glm::quat rotation;
                    Utils::GetTRS(tComp.WorldTransform, translation, rotation, scale);

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
                    physys->SetUserData(m_PhysicsInstance, handle, ToNumber64(entity));
                }
                physys->SetActive(m_PhysicsInstance, handle, rbComp.IsActive);
            });
        }

        { // update scripts
            auto& storage = Storage<ScriptComponent>();
            auto* script_engine = ServiceManager::GetService<ScriptEngine>();
            if (script_engine->IsExecOrderChanged())
                Sort<ScriptComponent>([script_engine](const ScriptComponent& a, const ScriptComponent& b) 
                {return script_engine->GetExecOrder(a.ScriptHandle) <  script_engine->GetExecOrder(b.ScriptHandle);});

            for (auto& cmp : storage) 
            {
                if (cmp.PendingStart)
                {
                    cmp.PendingStart = false;
                    script_engine->StartInstance(cmp.ScriptHandle);
                }
                else if (cmp.IsActive) script_engine->UpdateInstance(cmp.ScriptHandle, ts);
            }

            script_engine->OnUpdate(ts);
        }

    }   

    void Scene::OnUpdate(Timestep ts, EditorCamera* camera)
    {
        auto* jobsys = ServiceManager::GetService<JobSystem>(); jobsys->FlushCompletions();
        auto* physys = ServiceManager::GetService<PhysicsSystem>();

        { // render
            auto& camStorage = Storage<CameraComponent>();
            auto* asset_manager = ServiceManager::GetService<AssetManager>(); 
            auto* rigModule = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            
            CameraComponent* mainCamera = nullptr;
            for (auto& cmp : camStorage)
                if (cmp.Primary) { mainCamera = &cmp; break; }

            if (mainCamera || camera != nullptr)
            {

                glm::mat4 vp = (camera ? camera->GetViewProjection() : mainCamera->Camera.GetViewProjection());
                Utils::Frustum frustum = Utils::GetFrustum(vp);

                // light culling
                m_SceneLights.clear();
                auto& lightStorage = Storage<LightComponent>();
                jobsys->ParallelFor(lightStorage.size(), m_Threshold, lightStorage.raw(), [&](LightComponent* cmp)
                {
                    auto& light = cmp->Config;
                    if (light.type == LightType::None)
                    {
                        cmp->Culled = true;
                        return;
                    }
                    if (light.type == LightType::Spot)
                    {
                        float r = light.range / (2.0f * glm::cos(light.outerCone));
                        glm::vec3 c = light.position + light.direction * r;
                        cmp->Culled = !Utils::CheckSphereVisible(frustum, c, r);
                    }
                    else cmp->Culled = false;
                });
                for (const auto& cmp : lightStorage)
                    if (!cmp.Culled) m_SceneLights.push_back(cmp.Config);

                // mesh and animator culling
                auto meshGroup = Group<MeshComponent>(get<TransformComponent>);
                auto meshView = View<MeshComponent>();
                jobsys->ParallelFor(meshView.size(), m_Threshold, meshView->data(), [&](Entity entity)
                {
                    auto [meshcmp, transform] = meshGroup.get<MeshComponent, TransformComponent>(entity);
                    AnimatorComponent* animator = TryGetComponent<AnimatorComponent>(entity);

                    AMesh* mesh = asset_manager->GetAsset<AMesh>(meshcmp.Mesh); 
                    if (!mesh)
                    {
                        meshcmp.Culled = true;
                        if (animator) animator->Culled = true;
                        return; 
                    }
                    meshcmp.Culled = false;
                    if (animator) animator->Culled = false;

                    glm::mat4 world = transform.WorldTransform;
                    glm::vec3 worldMin, worldMax;
                    if (animator && mesh->m_HasJointData)
                        Utils::TransformBound(mesh->m_AnimatedBoundsMin, mesh->m_AnimatedBoundsMax, world, worldMin, worldMax);
                    else Utils::TransformBound(mesh->m_BoundsMin, mesh->m_BoundsMax, world, worldMin, worldMax);

                    meshcmp.WorldBoundsMin = worldMin;
                    meshcmp.WorldBoundsMax = worldMax;
                    meshcmp.Culled = !Utils::CheckBoundVisible(frustum, worldMin, worldMax);
                    if (meshcmp.Culled && animator) animator->Culled = true;
                });


                rigModule->ClearTasks();
                auto& animStorage = Storage<AnimatorComponent>();
                for (auto& cmp : animStorage)
                {
                    if (cmp.Clips.empty() || cmp.Culled || !cmp.RunSampling) continue;

                    auto* skeletonAsset = asset_manager->GetAsset<ASkeleton>(cmp.Skeleton);
                    auto* clipAsset = asset_manager->GetAsset<AClip>(cmp.Clips[cmp.ActiveClipIdx]);
                    if (!skeletonAsset || !clipAsset) continue;
                    auto skelHandle = skeletonAsset->m_Handle;
                    auto clipHandle = clipAsset->m_Handle;

                    if (!cmp.Cache.IsValid()) cmp.Cache = rigModule->CreateCache(clipAsset->m_Handle);
                    if (!cmp.CurrentPose.IsValid()) cmp.CurrentPose = rigModule->CreatePose(skeletonAsset->m_Handle);
                    if (cmp.CacheDirty)
                    {
                        rigModule->RepairCache(cmp.Cache, clipAsset->m_Handle);
                        cmp.CacheDirty = false;
                    }
                    if (cmp.IsPlaying)
                    {
                        float duration = clipAsset->m_Duration;
                        cmp.CurrentTime += ts * cmp.Speed;
                        if (cmp.Loop && duration > 0.0f) cmp.CurrentTime = std::fmod(cmp.CurrentTime, duration);
                        else
                        {
                            if (cmp.CurrentTime >= duration)
                            {
                                cmp.CurrentTime = duration;
                                cmp.IsPlaying   = false;  
                            }
                        }
                    }
                    rigModule->ScheduleSample(skelHandle, clipHandle, cmp.Cache, cmp.CurrentPose, cmp.CurrentTime);
                    rigModule->ScheduleFinalize(skelHandle, cmp.CurrentPose);
                }
                rigModule->ProcessTasks();
                rigModule->ClearTasks();
                for (const auto& [entity, cmp] : animStorage.each())
                {
                    if (cmp.Clips.empty() || cmp.Culled || !cmp.RunPostEval) continue;
                    if (cmp.onPostEvaluate) cmp.onPostEvaluate(entity, rigModule, float(ts));
                }
                rigModule->ProcessTasks();
                ResolveBoneAttachments();

                // render
                auto* renderer = ServiceManager::GetService<Renderer>(); 
                if (camera != nullptr) renderer->BeginScene(*camera, m_SceneLights.data(), m_SceneLights.size()); 
                else renderer->BeginScene(mainCamera->Camera, m_SceneLights.data(), m_SceneLights.size()); 

                for (auto entity : meshGroup)
                {
                    auto [meshcmp, transform] = meshGroup.get<MeshComponent, TransformComponent>(entity);
                    if (meshcmp.Culled) continue;
                    Handle<Pose> pose = Handle<Pose>::Null();
                    if (auto* animator = TryGetComponent<AnimatorComponent>(entity)) pose = animator->CurrentPose;
                    renderer->DrawMesh(meshcmp.Mesh, meshcmp.SharedSheet, pose, transform.WorldTransform);
                }
                renderer->EndScene();

                // draw debug box (bounds already computed during culling)
                for (auto entity : meshGroup)
                {
                    auto& meshcmp = meshGroup.get<MeshComponent>(entity);
                    if (!meshcmp.ShowBounds || meshcmp.Culled) continue;
                    renderer->RenderBox(meshcmp.WorldBoundsMin, meshcmp.WorldBoundsMax, glm::mat4(1.0f), RED);
                }

                auto& rbStorage = Storage<ColliderComponent>();
                for (const auto& cmp : rbStorage)
                {
                    if (!cmp.Visible || !cmp.ColliderHandle.IsValid()) continue;
                    Handle<RigidBody> handle = cmp.ColliderHandle;
                    PhysTransform pt = physys->GetPhysTransform(m_PhysicsInstance, handle);
                    glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0f), pt.translation) * glm::toMat4(pt.rotation);
                    switch (cmp.Shape)
                    {
                        case ColliderShape::Sphere:
                        {
                            float radius = cmp.Size.x;
                            renderer->RenderSphere(radius, colliderTransform, GREEN);
                            break;
                        }
                        case ColliderShape::Box:
                        {
                            glm::vec3 bMin = -cmp.Size;
                            glm::vec3 bMax =  cmp.Size;
                            renderer->RenderBox(bMin, bMax, colliderTransform, GREEN);
                            break;
                        }
                        case ColliderShape::Capsule:
                        {
                            float radius  = cmp.Size.x;
                            float halfCyl = std::max((cmp.Size.y * 0.5f) - radius, 0.0f);
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