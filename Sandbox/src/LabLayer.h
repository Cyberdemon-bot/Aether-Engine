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
    std::string MeshName;
    std::string AnimatorName;
    std::string AnimationName;
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
    
    std::vector<std::string> m_MeshNames;
    std::vector<std::string> m_AnimatorNames;
    std::vector<std::string> m_AnimationNames;
    
    std::queue<Aether::SceneLoadResult> m_CompletedParses;
    std::mutex m_ParseMutex;
    
    glm::vec3 m_ModelPos = glm::vec3(0.0f);
    glm::vec3 m_ModelRot = glm::vec3(0.0f);
    glm::vec3 m_ModelScale = glm::vec3(1.0f);
    
    bool m_AutoRotate = false;
    float m_RotationSpeed = 1.0f;

    std::vector<AnimationBinding> m_AnimationBindings;
};