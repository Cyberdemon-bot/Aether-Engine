#pragma once

#include <Aether.h>
#include <vector>
#include <glm/glm.hpp> 

class GameLayer : public Aether::Layer
{
public:
    GameLayer();
    virtual ~GameLayer() = default;

    virtual void Detach() override;
    virtual void Attach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    glm::mat4 CalculateLightSpaceMatrix();
    void RenderShadowPass(const glm::mat4& lightSpaceMatrix);
    void RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix);
    void RenderScene(Aether::Ref<Aether::Shader> shader);

private:
    // New API rendering objects
    Aether::Ref<Aether::VertexArray> m_VAO;
    Aether::Ref<Aether::Shader> m_Shader;
    Aether::Ref<Aether::Shader> m_ShadowShader;
    Aether::Ref<Aether::Texture2D> m_Texture;
    Aether::Ref<Aether::FrameBuffer> m_ShadowFBO;
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    Aether::Ref<Aether::VertexBuffer> m_InstanceVBO;

    // Skybox (using Legacy TextureCube temporarily until ported)
    Aether::Ref<Aether::VertexArray> m_SkyboxVAO;
    Aether::Ref<Aether::Shader> m_SkyboxShader;
    Aether::Ref<Aether::TextureCube> m_SkyboxTexture;

    Aether::Ref<Aether::FrameBuffer> m_SceneFBO;      // FBO lưu ảnh game
    Aether::Ref<Aether::Texture2D> m_LutTexture;      // Texture LUT.png
    Aether::Ref<Aether::Shader> m_LutShader;          // Shader xử lý LUT

    Aether::Ref<Aether::VertexArray> m_ScreenQuadVAO;
    float m_LutIntensity = 1.0f;
    
    void InitSkybox();
    void RenderSkybox();
    void InitScreenQuad();

    Aether::EditorCamera m_EditorCamera;
    bool m_CursorLocked = false;
    glm::vec2 m_LastMousePos = { 0.0f, 0.0f };

    // Scene objects
    glm::vec3 m_TranslationA = { -2.0f, 0.5f, 0.0f };
    glm::vec3 m_TranslationB = { 2.0f, 0.5f, 0.0f };
    std::vector<glm::vec3> m_RandomCubes;
    std::vector<float> m_CubesSize;
    std::vector<float> m_CubeRot;
    std::vector<glm::mat4> m_InstanceModels;

    float m_CubeScale = 1.0f;
    float m_FloorScale = 15.0f;
    float m_Rotation = 0.0f;
    float m_RotationSpeed = 0.5f;
    bool m_EnableRotation = true;

    // Lighting
    glm::vec3 m_LightPos = { 0.0f, 8.0f, 0.0f };
    glm::vec3 m_LightDir = { 0.0f, -1.0f, 0.0f };
    float m_InnerAngle = 20.0f;
    float m_OuterAngle = 30.0f;

    // Rendering settings
    glm::vec4 m_BackgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    int m_ShadowMapResolution = 2048;

    // Fog
    bool m_FogEnabled = true;
    glm::vec3 m_FogColor = { 0.1f, 0.1f, 0.1f };
    float m_FogStart = 10.0f;
    float m_FogEnd = 40.0f;
};