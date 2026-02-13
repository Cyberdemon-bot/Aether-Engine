#pragma once
#include <Aether.h>

class PBRDemoLayer : public Aether::Layer
{
public:
    PBRDemoLayer();
    virtual ~PBRDemoLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    Aether::EditorCamera m_Camera;
    
    glm::vec3 m_CubeAPos = glm::vec3(-2.0f, 0.0f, 0.0f);
    glm::vec3 m_CubeBPos = glm::vec3(2.0f, 0.0f, 0.0f);
    
    float m_Rotation = 0.0f;
    bool m_AutoRotate = true;
    float m_RotationSpeed = 1.0f;
};