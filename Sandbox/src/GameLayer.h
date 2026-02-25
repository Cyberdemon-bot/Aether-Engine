#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include "Aether/Scene/Scene.h"
#include "Aether/Physics/PhysicsSystem.h"

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void Attach()        override;
    virtual void Detach()        override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void LoadModelAsync(const std::vector<std::string>& args);
    void AddEntity(const std::vector<std::string>& args);
    void DrainParseQueue();
    void RegisterPhysicsBody(Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic = true);

    void DrawHierarchyPanel();
    void DrawEntityNode(Entity entity);
    void DrawScenePanel();
    void DrawAnimationPanel();
    void DrawLightingPanel();

private:
    Aether::Scene        m_Scene;
    Aether::EditorCamera m_Camera;

    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    std::vector<Aether::UUID> m_Meshes;
    std::vector<Aether::UUID> m_Animators;

    Entity m_LightEntity    = Null_Entity;
    Entity m_DirLightEntity = Null_Entity;
    Entity m_SelectedEntity = Null_Entity;

    glm::vec3 m_DirLightEuler = glm::vec3(-45.0f, 0.0f, 0.0f);

    std::queue<Aether::ParsedScene> m_CompletedParses;
    std::mutex                      m_ParseMutex;

    uint32_t m_LightIdx = 0;

    float m_VolDensity   = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;
    float m_ShadowBias   = 0.00001f;

    bool  m_AutoRotate    = false;
    float m_RotationSpeed = 1.0f;

    int m_BindMeshIndex     = -1;
    int m_BindAnimatorIndex = -1;

    struct PhysicsEntry
    {
        Aether::UUID bodyID;
        bool         enabled    = false;
        bool         lastActive = false;   
        bool         isDynamic  = true;
    };
    std::unordered_map<Entity, PhysicsEntry> m_PhysicsBodies;

    Entity      m_PhysSelectedEntity = Null_Entity;
    int         m_PhysMeshIdx        = -1;
    bool        m_PhysDynamic        = true;

    glm::vec3   m_ForceInput    = glm::vec3(0.0f);
    glm::vec3   m_VelocityInput = glm::vec3(0.0f);

    glm::vec3                        m_RayOrigin    = glm::vec3(0.0f);
    glm::vec3                        m_RayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float                            m_RayDistance  = 100.0f;
    std::vector<Aether::RaycastHit>  m_LastRayHits;
    bool                             m_RayHasFired  = false;
};