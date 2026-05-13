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
    void LoadCacheModelAsync(const std::vector<std::string>& args);
    void AddEntity(const std::vector<std::string>& args);
    void DrainParseQueue();
    void RegisterPhysicsBody(Aether::Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic = true);

    // Rebuilds (or clears) the onPostEvaluate callback on m_IKAnimatorEntity
    // based on the current IK/blend enabled flags.  Call this whenever any
    // IK/blend toggle or parameter changes in the UI.
    void RebuildPostEvaluate();

    void DrawHierarchyPanel();
    void DrawScenePanel();
    void DrawAnimationPanel();
    void DrawLightingPanel();
    void DrawScriptingPanel();
    void DrawBoneAttachmentPanel();

private:
    Aether::Scene        m_Scene;
    Aether::EditorCamera m_Camera;

    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    Aether::Ref<Aether::FrameBuffer> m_MainFbo;
    Aether::Ref<Aether::FrameBuffer> m_VolFbo;

    std::vector<Aether::RenderPass> m_Pipeline;

    std::vector<Aether::UUID> m_MeshIDs;

    Aether::Entity m_LightEntity    = Aether::Null_Entity;
    Aether::Entity m_SelectedEntity = Aether::Null_Entity;

    std::queue<Aether::Ref<Aether::ParsedScene>> m_CompletedParses;
    std::mutex                      m_ParseMutex;

    float m_VolDensity   = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;
    float m_ShadowBias   = 0.00001f;

    int m_BindMeshIndex     = -1;
    int m_BindAnimatorIndex = -1;

    Aether::Entity m_PhysSelectedEntity = Aether::Null_Entity;
    int            m_PhysMeshIdx        = -1;
    bool           m_PhysDynamic        = true;

    glm::vec3 m_ForceInput    = glm::vec3(0.0f);
    glm::vec3 m_VelocityInput = glm::vec3(0.0f);

    glm::vec3                       m_RayOrigin    = glm::vec3(0.0f);
    glm::vec3                       m_RayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float                           m_RayDistance  = 100.0f;
    std::vector<Aether::RaycastHit> m_LastRayHits;
    bool                            m_RayHasFired  = false;

    std::string    m_ScriptPath          = "";
    Aether::Entity m_ScriptTargetEntity  = Aether::Null_Entity;

    // -------------------------------------------------------------------------
    //  Bone Attachment panel state
    // -------------------------------------------------------------------------
    Aether::Entity m_BoneAttachChildEntity    = Aether::Null_Entity;
    Aether::Entity m_BoneAttachAnimatorEntity = Aether::Null_Entity;
    char           m_BoneNameBuf[128]         = {};

    // -------------------------------------------------------------------------
    //  Animation panel — IK / advanced state
    // -------------------------------------------------------------------------

    // Entity whose AnimatorComponent carries the onPostEvaluate callback
    Aether::Entity m_IKAnimatorEntity = Aether::Null_Entity;

    // Two-bone IK
    struct TwoBoneIKState
    {
        int       rootIdx  = -1;
        int       midIdx   = -1;
        int       endIdx   = -1;
        glm::vec3 target   = { 0.f, 0.f, 0.f };
        glm::vec3 pole     = { 0.f, 1.f, 0.f };
        float     weight   = 1.f;
        bool      enabled  = false;
    } m_TwoBoneIK;

    // Look-at IK
    struct LookAtState
    {
        int       boneIdx    = -1;
        glm::vec3 target     = { 0.f, 0.f, 1.f };
        glm::vec3 forward    = { 0.f, 0.f, 1.f };
        glm::vec3 up         = { 0.f, 1.f, 0.f };
        float     weight     = 1.f;
        float     angleLimit = glm::half_pi<float>();
        bool      enabled    = false;
    } m_LookAt;

    // Blend
    struct BlendState
    {
        int   clipAIdx = 0;
        int   clipBIdx = 1;
        float alpha    = 0.5f;
        bool  additive = false;
        bool  enabled  = false;
    } m_Blend;

    // Joint browser (shared between IK pickers and bone attachment)
    Aether::Entity              m_JointBrowserEntity = Aether::Null_Entity;
    std::vector<std::string>    m_CachedJointNames;

    char m_SceneSavePath[256] = ".cache/untitled.yaml";
    char m_SceneLoadPath[256] = ".cache/untitled.yaml";
};