#pragma once
#include <Aether.h>
#include "Aether/Resources/SceneLoader.h"
#include <glm/glm.hpp>
#include <vector>
#include <mutex>
#include <queue>
#include <string>

struct AnimationBinding
{
    Aether::UUID mesh = Aether::UUID(0);
    Aether::UUID skeleton = Aether::UUID(0);
    Aether::UUID anim = Aether::UUID(0);
    bool IsActive = false;
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
    void LoadModelAsync(const std::string& path);

private:
    Aether::EditorCamera m_Camera;
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    Aether::Ref<Aether::UniformBuffer> m_BoneUBO;

    std::vector<Aether::UUID> m_Meshes;
    std::vector<Aether::UUID> m_SkeletalAnimator;
    std::vector<Aether::UUID> m_SkeletalAnim;
    
    std::queue<Aether::SceneLoadResult> m_CompletedParses;
    std::mutex m_ParseMutex;
    
    glm::vec3 m_ModelPos = glm::vec3(0.0f);
    glm::vec3 m_ModelRot = glm::vec3(0.0f);
    glm::vec3 m_ModelScale = glm::vec3(1.0f);
    
    bool m_AutoRotate = false;
    float m_RotationSpeed = 1.0f;

    std::vector<AnimationBinding> m_AnimationBindings;

    std::vector<std::string> m_ConsoleItems = { 
        "[System] Console Initialized...",
        "[Info] Type 'help' for commands."
    };
    char m_InputBuf[256] = ""; // Buffer chứa text đang nhập
    bool m_ScrollToBottom = true; // Cờ để tự động cuộn xuống dưới
};