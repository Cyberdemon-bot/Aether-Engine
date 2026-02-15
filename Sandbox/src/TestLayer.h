#pragma once
#include <Aether.h>

class TestLayer : public Aether::Layer
{
public:
    TestLayer();
    virtual ~TestLayer() = default;

    virtual void Attach() override;
    virtual void Detach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    Aether::EditorCamera m_Camera;

    std::vector<Aether::UUID> m_LoadedMeshIDs;
    
    glm::vec3 m_CubePos = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_Rotation = 0.0f;
    bool m_AutoRotate = true;
    float m_RotationSpeed = 1.0f;
};