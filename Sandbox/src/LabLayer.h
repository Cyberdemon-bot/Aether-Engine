#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

class LabLayer : public Aether::Layer
{
public:
    LabLayer();
    virtual ~LabLayer() = default;

    virtual void Attach()                          override;
    virtual void Detach()                          override;
    virtual void Update(Aether::Timestep ts)       override;
    virtual void OnImGuiRender()                   override;
    virtual void OnEvent(Aether::Event& event)     override;

private:
    void LoadModelAsync(const std::vector<std::string>& args);
    void AddEntity(const std::vector<std::string>& args);
    void DrainParseQueue();
    void RegisterPhysicsBody(Aether::Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic = true);

    void DrawHierarchyPanel();
    void DrawEntityNode(Aether::Entity entity);
    void DrawScenePanel();
    //void DrawAnimationPanel();
    void DrawLightingPanel();
    void DrawScriptingPanel();

private:
    Aether::Scene        m_Scene;
    Aether::EditorCamera m_Camera;

    // Shader
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    // FBOs stored as members so RenderPass raw pointers stay valid
    Aether::Ref<Aether::FrameBuffer> m_MainFbo;
    Aether::Ref<Aether::FrameBuffer> m_VolFbo;

    std::vector<Aether::RenderPass> m_Pipeline;

    // Loaded assets (UUIDs for AssetManager lookup)
    std::vector<Aether::UUID> m_MeshIDs;
    std::vector<Aether::UUID> m_AnimatorIDs;

    Aether::Entity m_LightEntity    = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity;

    // Async model loading
    std::queue<Aether::Ref<Aether::ParsedScene>> m_CompletedParses;
    std::mutex                      m_ParseMutex;

    // Volumetric / shadow settings
    float m_VolDensity   = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;
    float m_ShadowBias   = 0.00001f;

    // Animation panel
    int m_BindMeshIndex     = -1;
    int m_BindAnimatorIndex = -1;

    // Physics
    struct PhysicsEntry
    {
        Aether::BodyHandle handle;
        bool         enabled    = false;
        bool         lastActive = false;
        bool         isDynamic  = true;
    };
    std::unordered_map<Aether::Entity, PhysicsEntry> m_PhysicsBodies;

    Aether::Entity m_PhysSelectedEntity = Aether::Null_Entity;
    int            m_PhysMeshIdx        = -1;
    bool           m_PhysDynamic        = true;

    glm::vec3 m_ForceInput    = glm::vec3(0.0f);
    glm::vec3 m_VelocityInput = glm::vec3(0.0f);

    // Raycast
    glm::vec3                       m_RayOrigin    = glm::vec3(0.0f);
    glm::vec3                       m_RayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float                           m_RayDistance  = 100.0f;
    std::vector<Aether::RaycastHit> m_LastRayHits;
    bool                            m_RayHasFired  = false;

    std::string    m_ScriptPath          = "";
    Aether::Entity m_ScriptTargetEntity  = Aether::Null_Entity;
};