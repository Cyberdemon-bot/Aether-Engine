#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

struct Transform
{
    glm::vec3 m_ModelPos   = glm::vec3(0.0f);
    glm::vec3 m_ModelRot   = glm::vec3(0.0f);
    glm::vec3 m_ModelScale = glm::vec3(1.0f);
};

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void Attach()         override;
    virtual void Detach()         override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender()  override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    // --- Scene ---
    void LoadModelAsync(const std::vector<std::string>& args);
    void DrainParseQueue();
    void BuildTransformMatrix(const Transform& t, glm::mat4& out) const;

    // --- ImGui panels ---
    void DrawScenePanel();
    void DrawAnimationPanel();
    void DrawLightingPanel();

private:
    // ── Camera ────────────────────────────────────────────────
    Aether::EditorCamera m_Camera;

    // ── Shaders ───────────────────────────────────────────────
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    // ── Scene data ────────────────────────────────────────────
    std::vector<Aether::UUID>    m_Meshes;
    std::vector<Aether::UUID>    m_Animators;
    std::vector<Transform>       m_Transforms;

    std::unordered_map<Aether::UUID, Aether::UUID> m_MeshToAnimator;

    // ── Async loading ─────────────────────────────────────────
    std::queue<Aether::ParsedScene> m_CompletedParses;
    std::mutex                      m_ParseMutex;

    // ── Lighting ──────────────────────────────────────────────
    std::vector<Aether::LightParam> m_Lights;
    uint32_t m_LightIdx = 0;

    // ── Volumetric ────────────────────────────────────────────
    float m_VolDensity   = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;

    // ── Shadow ────────────────────────────────────────────────
    float m_ShadowBias = 0.00001f;

    // ── Transform editor ─────────────────────────────────────
    int  m_SelectedMeshIndex = -1;
    bool m_AutoRotate        = false;
    float m_RotationSpeed    = 1.0f;

    // ── Animation bind UI ─────────────────────────────────────
    int m_BindMeshIndex     = -1;
    int m_BindAnimatorIndex = -1;
};