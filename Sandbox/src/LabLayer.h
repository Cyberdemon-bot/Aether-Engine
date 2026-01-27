#pragma once
#include <Aether.h>
#include <glm/glm.hpp>
#include <vector>

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

private:
    Aether::EditorCamera m_Camera;
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    std::vector<Aether::UUID> m_MeshIDs;
    
    glm::vec3 m_ModelPos = glm::vec3(0.0f);
    glm::vec3 m_ModelRot = glm::vec3(0.0f);
    glm::vec3 m_ModelScale = glm::vec3(1.0f);
    
    bool m_AutoRotate = false;
    float m_RotationSpeed = 1.0f;
};