#include "aepch.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Scene/Component.h"

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
            scene.AddComponent<MeshComponent>(e).MeshID = reg.meshIDs[node.meshIdx];

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

        bool dirty = transform.Dirty || pDirty;

        if (dirty)
        {
            transform.WorldTransform = pTransfrom * transform.GetLocalTransform();
            transform.Dirty = false;
        }

        Entity currentChild = hierarchy.firstChild;
        while (currentChild != Null_Entity)
        {
            UpdateTransform(currentChild, transform.WorldTransform, dirty);
            currentChild = GetComponent<HierarchyComponent>(currentChild).nextSibling;
        }
    }

    void Scene::Update(Timestep ts, EditorCamera* camera)
    {
        { // update hierarchy
            auto view = View<HierarchyComponent>();
            for (auto entity : view)
            {
                const auto& hierarchy = GetComponent<HierarchyComponent>(entity);
                if (hierarchy.parent == Null_Entity) UpdateTransform(entity, glm::mat4(1.0f), false);
            }
        }

        { // 2. Render
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
                auto meshView = View<MeshComponent, TransformComponent>();

                for (auto entity : meshView)
                {
                    auto& transform = GetComponent<TransformComponent>(entity).WorldTransform;
                    UUID meshID = GetComponent<MeshComponent>(entity).MeshID;
                    
                    UUID animatorID = UUID(0);
                    if (HasComponent<AnimatorComponent>(entity)) animatorID = GetComponent<AnimatorComponent>(entity).AnimatorID;

                    Renderer::DrawMesh(meshID, animatorID, transform);
                }

                Renderer::EndScene();
            }
        }
    }
}