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
    glm::vec3 m_ModelPos = glm::vec3(0.0f);
    glm::vec3 m_ModelRot = glm::vec3(0.0f);
    glm::vec3 m_ModelScale = glm::vec3(1.0f);
};

class LabLayer : public Aether::Layer
{
public:
    LabLayer();
    virtual ~LabLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void RenderScene();
    void LoadModelAsync(const std::vector<std::string>& args);
    void PrintSceneLog(const Aether::RegisteredScene& result);

private:
    Aether::EditorCamera m_Camera;
    Aether::Ref<Aether::Shader> m_Shader;
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    Aether::Ref<Aether::UniformBuffer> m_BoneUBO;

    std::vector<Aether::UUID> m_Meshes;
    std::vector<Aether::UUID> m_Skeletons;
    std::vector<Aether::UUID> m_Animators; 
    std::vector<Aether::UUID> m_Clips;
    std::vector<Transform> m_Transforms;
    
    std::unordered_map<Aether::UUID, Aether::UUID> m_SkeletonToAnimator;
    std::unordered_map<Aether::UUID, Aether::UUID> m_MeshToSkeleton;
    
    std::queue<Aether::ParsedScene> m_CompletedParses;
    std::mutex m_ParseMutex;

    int m_SelectedMeshIndex = -1;
    bool m_AutoRotate = false;
    float m_RotationSpeed = 1.0f;

    std::vector<std::string> m_ConsoleItems = { 
        "[System] Console Initialized...",
        "[Info] Type 'help' for commands."
    };
    char m_InputBuf[256] = "";
    bool m_ScrollToBottom = true;
};