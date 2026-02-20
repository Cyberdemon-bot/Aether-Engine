#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include "Aether/Scene/Scene.h"

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void Attach()                          override;
    virtual void Detach()                          override;
    virtual void Update(Aether::Timestep ts)       override;
    virtual void OnImGuiRender()                   override;
    virtual void OnEvent(Aether::Event& event)     override;

private:
    void LoadModelAsync(const std::vector<std::string>& args);
    void AddEntity(const std::vector<std::string>& args);
    void DrainParseQueue();

    void DrawHierarchyPanel();
    void DrawEntityNode(Entity entity);
    void DrawScenePanel();
    void DrawAnimationPanel();
    void DrawLightingPanel();

private:
    Aether::Scene m_Scene;
    Aether::EditorCamera m_Camera;

    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Shader> m_MainShader;
    Aether::Ref<Aether::Shader> m_VolShader;

    std::vector<Aether::UUID> m_Meshes;
    std::vector<Aether::UUID> m_Animators;

    Entity m_LightEntity = Null_Entity;
    Entity m_SelectedEntity = Null_Entity;

    std::queue<Aether::ParsedScene> m_CompletedParses;
    std::mutex m_ParseMutex;

    uint32_t m_LightIdx = 0;

    float m_VolDensity   = 0.03f;
    float m_VolIntensity = 1.0f;
    int   m_VolSteps     = 64;

    float m_ShadowBias    = 0.00001f;

    bool  m_AutoRotate    = false;
    float m_RotationSpeed = 1.0f;

    int m_BindMeshIndex     = -1;
    int m_BindAnimatorIndex = -1;
};