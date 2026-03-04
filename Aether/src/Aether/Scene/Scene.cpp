#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Aether/Assets/AssetsManager.h"

namespace Aether {
    Scene::Scene() 
    {
        m_SceneLights.reserve(16); 
    }

    Scene::~Scene() 
    {
    }
    
    Entity Scene::CreateEntity()
    {
        return CreateEntity("default entity");
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

        const UUID id = m_Registry.get<IDComponent>(entity).ID;
        m_EntityLibrary.erase(id);
        m_Registry.destroy(entity);
    }

    void Scene::DestroyEntity(Entity entity)
    {
        if (!m_Registry.valid(entity)) return;
        BreakParent(entity);
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

                if (parentHie.firstChild == Null_Entity)
                {
                    parentHie.firstChild = child;
                }
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

        hierarchy.parent      = Null_Entity;
        hierarchy.prevSibling = Null_Entity;
        hierarchy.nextSibling = Null_Entity;
        hierarchy.firstChild  = Null_Entity;
    }

    bool Scene::IsValid(Entity entity) const
    {
        return m_Registry.valid(entity);
    }

    Entity Scene::FindEntity(UUID id) const
    {
        auto it = m_EntityLibrary.find(id);
        if(it != m_EntityLibrary.end()) return it->second;
        return Null_Entity;
    }

    UUID Scene::GetUUID(Entity entity) const
    {
         if (!m_Registry.valid(entity)) return UUID(0);
         return m_Registry.get<IDComponent>(entity).ID;
    }

    static void CreateNodeEntity(Scene& scene, const RegisteredScene& reg, int nodeIdx, Entity parentEntity)
    {
        const Node& node = reg.hierarchy->nodes[nodeIdx];

        Entity e = scene.CreateEntity(node.name, parentEntity);

        auto& t       = scene.GetComponent<TransformComponent>(e);
        t.Translation = node.translation;
        t.Rotation    = glm::normalize(node.rotation);
        t.Scale       = node.scale;
        t.Dirty       = true;

        if (node.meshIdx >= 0 && node.meshIdx < (int)reg.meshIDs.size())
        {
            auto& component = scene.AddComponent<MeshComponent>(e);
            component.MeshPtr = AssetsManager::GetResource<Mesh>(reg.meshIDs[node.meshIdx]);
            component.Materials = reg.meshMap[node.meshIdx];
        }

        if (node.animatorIdx >= 0 && node.animatorIdx < (int)reg.animatorIDS.size())
            scene.AddComponent<AnimatorComponent>(e).AnimatorID = reg.animatorIDS[node.animatorIdx];

        for (int childIdx : node.children)
            CreateNodeEntity(scene, reg, childIdx, e);
    }

     void Scene::LoadHierarchy(const RegisteredScene& registered, Entity parent)
    {
        for (int rootIdx : registered.hierarchy->roots)
            CreateNodeEntity(*this, registered, rootIdx, parent);
    }

    void Scene::UpdateTransform(Entity entity, const glm::mat4& pTransfrom, bool pDirty)
    {
        auto& transform = GetComponent<TransformComponent>(entity);
        auto& hierarchy = GetComponent<HierarchyComponent>(entity);
    
        bool isWorldTransformDirty = transform.Dirty || pDirty;
        bool hasRecalculatedWorld = false; 

        if (HasComponent<ColliderComponent>(entity))
        {
            auto& rbComp = GetComponent<ColliderComponent>(entity);
            UUID id = rbComp.BodyID;  

            if (isWorldTransformDirty) 
            {
                transform.WorldTransform = pTransfrom * transform.GetLocalTransform();
                hasRecalculatedWorld = true;

                glm::vec3 scale, translation, skew;
                glm::quat rotation;
                glm::vec4 perspective;
                glm::decompose(transform.WorldTransform, scale, rotation, translation, skew, perspective);
                glm::vec3 worldOffset = rotation * rbComp.ColliderOffset;
                PhysicsSystem::SetPhysTransform(id, {translation + worldOffset, rotation});
            }
            else if (PhysicsSystem::GetMotionType(id) == MotionType::Dynamic)
            {
                PhysTransform physTrans = PhysicsSystem::GetPhysTransform(id);
                glm::vec3 localOffset = rbComp.ColliderOffset;
                glm::vec3 translation = physTrans.translation - (physTrans.rotation * localOffset);

                if (hierarchy.parent == Null_Entity)
                {
                    transform.Translation = translation;
                    transform.Rotation = physTrans.rotation;
                }
                else
                {
                    glm::mat4 invParent = glm::inverse(pTransfrom);
                    glm::quat parentRot = glm::quat_cast(pTransfrom);
                    transform.Translation = glm::vec3(invParent * glm::vec4(translation, 1.0f));
                    transform.Rotation = glm::inverse(parentRot) * physTrans.rotation; 
                }
                
                isWorldTransformDirty = true; 
            }
        }

        if (isWorldTransformDirty)
        {
            if (!hasRecalculatedWorld) transform.WorldTransform = pTransfrom * transform.GetLocalTransform();
            transform.Dirty = false;
        }
        Entity currentChild = hierarchy.firstChild;
        while (currentChild != Null_Entity)
        {
            UpdateTransform(currentChild, transform.WorldTransform, isWorldTransformDirty); 
            currentChild = GetComponent<HierarchyComponent>(currentChild).nextSibling;
        }
    }

    void Scene::Update(Timestep ts, EditorCamera* camera)
    {
        { 
            AnimationSystem::Update(ts);
            PhysicsSystem::Update(ts);
        }

        { 
            auto view = View<HierarchyComponent>();
            for (auto entity : view)
            {
                const auto& hierarchy = GetComponent<HierarchyComponent>(entity);
                if (hierarchy.parent == Null_Entity) UpdateTransform(entity, glm::mat4(1.0f), false);
            }
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

            m_SceneLights.clear();
            auto lightView = View<LightComponent, TransformComponent>();
            for (auto entity : lightView)
            {
                const auto& lightComp = lightView.get<LightComponent>(entity);
                const auto& transform = lightView.get<TransformComponent>(entity);
                LightParam param = lightComp.Config;
                param.position = glm::vec3(transform.WorldTransform[3]);
                param.direction = glm::normalize(glm::vec3(-transform.WorldTransform[2]));
                m_SceneLights.push_back(param);
            }

            if (mainCamera || camera != nullptr)
            {
                if (camera != nullptr) Renderer::BeginScene(*camera, m_SceneLights); 
                else Renderer::BeginScene(mainCamera->Camera, m_SceneLights); 

                glm::mat4 vp = (camera ? camera->GetViewProjection() : mainCamera->Camera.GetViewProjection());
                Utils::Frustum frustum = Utils::GetFrustum(vp);

                // draw meshes
                auto meshView = View<MeshComponent, TransformComponent>();
                for (auto entity : meshView)
                {
                    auto& transform = GetComponent<TransformComponent>(entity);
                    auto& meshcmp = GetComponent<MeshComponent>(entity);
                    auto mesh = meshcmp.MeshPtr; if (!mesh) continue;
                    UUID animatorID = UUID(0);
                    if (HasComponent<AnimatorComponent>(entity)) animatorID = GetComponent<AnimatorComponent>(entity).AnimatorID;

                    glm::mat4 world = transform.WorldTransform;
                    glm::vec3 worldMin, worldMax;
                    if (animatorID != UUID(0))
                    {
                        glm::mat4 rootMat = AnimationSystem::GetModule<RigModule>()->GetRootMat(animatorID);
                        glm::vec3 scale(glm::length(glm::vec3(rootMat[0])), glm::length(glm::vec3(rootMat[1])), glm::length(glm::vec3(rootMat[2])));
                        world *= glm::scale(glm::mat4(1.0f), scale);
                    }
                    Utils::TransformBound(mesh->GetBoundsMin(), mesh->GetBoundsMax(), world, worldMin, worldMax);
                    if (!Utils::CheckBoundVisible(frustum, worldMin, worldMax)) continue;

                    Renderer::DrawMesh(meshcmp.MeshPtr, meshcmp.Materials, animatorID, transform.WorldTransform);
                }

                Renderer::EndScene();

                auto rbView = View<ColliderComponent>();
                if (!rbView.empty())
                {
                    for (auto entity : rbView)
                    {
                        auto& component = GetComponent<ColliderComponent>(entity);
                        if (!component.Visible) continue;
                        UUID bodyID = component.BodyID;
                        PhysTransform pt = PhysicsSystem::GetPhysTransform(bodyID);
                        glm::mat4 colliderTransform = glm::translate(glm::mat4(1.0f), pt.translation)
                                                    * glm::toMat4(pt.rotation);
                        if (component.Shape == ColliderShape::Box)
                        {
                            glm::vec3 bMin(-0.5f), bMax(0.5f);
                            if (HasComponent<MeshComponent>(entity))
                            {
                                auto mesh = GetComponent<MeshComponent>(entity).MeshPtr;
                                if (mesh) 
                                { 
                                    glm::vec3 half = component.Size * 0.5f;
                                    glm::vec3 meshCenter = mesh->GetBoundsCenter();
                                    bMin = meshCenter - half;
                                    bMax = meshCenter + half;
                                }
                            }
                            Renderer::RenderBox(bMin, bMax, colliderTransform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
                        }
                        if (component.Shape == ColliderShape::Capsule)
                        {
                            float radius  = component.Size.x;
                            float halfCyl = std::max((component.Size.y * 0.5f) - radius, 0.0f);
                            Renderer::RenderCapsule(radius, halfCyl, colliderTransform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
                        }
                    }
                }
            }
        }
    }
}