#pragma once

#include <Aether.h>
#include <vector>
#include <glm/glm.hpp>

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    // Hủy tài nguyên khi layer bị gỡ
    virtual void Detach() override;

    // Khởi tạo tài nguyên khi layer được gắn vào
    virtual void Attach() override;

    // Cập nhật logic mỗi frame
    virtual void Update(Aether::Timestep ts) override;

    // Vẽ giao diện ImGui
    virtual void OnImGuiRender() override;

    // Xử lý sự kiện
    virtual void OnEvent(Aether::Event& event) override;

private:
    void HandleInput(Aether::Timestep ts);
    glm::mat4 CalculateLightSpaceMatrix();
    void RenderShadowPass(const glm::mat4& lightSpaceMatrix);
    void RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix);
    void RenderScene(Aether::Ref<Aether::Legacy::Shader> shader);

private:
    Aether::Ref<Aether::Legacy::VertexArray> m_VAO;
    Aether::Ref<Aether::Legacy::VertexBuffer> m_VBO;
    Aether::Ref<Aether::Legacy::IndexBuffer> m_IBO;
    Aether::Ref<Aether::Legacy::Shader> m_Shader;
    Aether::Ref<Aether::Legacy::Shader> m_ShadowShader;
    Aether::Ref<Aether::Legacy::Texture> m_Texture;
    Aether::Ref<Aether::Legacy::FrameBuffer> m_ShadowFBO;

    Aether::Legacy::Camera m_Camera;
    bool m_CursorLocked = false;
    glm::vec2 m_LastMousePos = { 0.0f, 0.0f };

    glm::vec3 m_TranslationA = { -2.0f, 0.5f, 0.0f };
    glm::vec3 m_TranslationB = { 2.0f, 0.5f, 0.0f };
    std::vector<glm::vec3> m_RandomCubes;

    float m_CubeScale = 1.0f;
    float m_FloorScale = 15.0f;
    float m_Rotation = 0.0f;
    float m_RotationSpeed = 0.5f;
    bool m_EnableRotation = true;

    glm::vec3 m_LightPos = { 0.0f, 8.0f, 0.0f };
    glm::vec3 m_LightDir = { 0.0f, -1.0f, 0.0f };
    float m_InnerAngle = 20.0f;
    float m_OuterAngle = 30.0f;

    glm::vec4 m_BackgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    int m_ShadowMapResolution = 2048;

    bool m_FogEnabled = true;
    glm::vec3 m_FogColor = { 0.1f, 0.1f, 0.1f };
    float m_FogStart = 10.0f;
    float m_FogEnd = 40.0f;
};