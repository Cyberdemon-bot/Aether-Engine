#pragma once

#include <Aether.h>
#include <vector>
#include <glm/glm.hpp>

class PBRLayer : public Aether::Layer
{
public:
    PBRLayer();
    virtual ~PBRLayer() = default;

    virtual void Detach() override;
    virtual void Attach() override;
    virtual void Update(Aether::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Aether::Event& event) override;

private:
    void HandleInput(Aether::Timestep ts);
    glm::mat4 CalculateLightSpaceMatrix();
    void RenderShadowPass(const glm::mat4& lightSpaceMatrix);
    void RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix);
    void RenderMeshes(Aether::Ref<Aether::Shader> shader);
    
    // Mesh creation helpers
    Aether::Mesh CreateCubeMesh(const std::string& name = "Cube");
    void SetupMeshRendering(Aether::Mesh& mesh);

private:
    // Mesh storage
    std::vector<Aether::Mesh> m_Meshes;
    std::vector<Aether::Ref<Aether::VertexArray>> m_MeshVAOs;
    
    // Shaders (New API)
    Aether::Ref<Aether::Shader> m_PBRShader;
    Aether::Ref<Aether::Shader> m_ShadowShader;
    
    // Shadow mapping (New API)
    Aether::Ref<Aether::FrameBuffer> m_ShadowFBO;
    
    // Camera UBO (New API)
    Aether::Ref<Aether::UniformBuffer> m_CameraUBO;
    
    // Skybox (New API)
    Aether::Ref<Aether::VertexArray> m_SkyboxVAO;
    Aether::Ref<Aether::Shader> m_SkyboxShader;
    Aether::Ref<Aether::TextureCube> m_SkyboxTexture;
    
    void InitSkybox();
    void RenderSkybox();

    // Camera (Still using Legacy temporarily)
    Aether::Camera m_Camera;
    bool m_CursorLocked = false;
    glm::vec2 m_LastMousePos = { 0.0f, 0.0f };

    // Scene transforms
    glm::vec3 m_FloorPosition = { 0.0f, -2.0f, 0.0f };
    glm::vec3 m_FloorScale = { 15.0f, 0.1f, 15.0f };
    
    // Test mesh transforms and materials
    struct MeshInstance {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        // Hard-coded PBR material properties
        glm::vec3 albedo;
        float metallic;
        float roughness;
        float ao;
    };
    
    std::vector<MeshInstance> m_MeshInstances;
    int m_SelectedMesh = 0;
    
    float m_Rotation = 0.0f;
    float m_RotationSpeed = 0.5f;
    bool m_EnableRotation = true;

    // Spotlight
    glm::vec3 m_LightPos = { 0.0f, 8.0f, 0.0f };
    glm::vec3 m_LightDir = { 0.0f, -1.0f, 0.0f };
    glm::vec3 m_LightColor = { 1.0f, 1.0f, 1.0f };
    float m_LightIntensity = 300.0f;
    float m_InnerAngle = 20.0f;
    float m_OuterAngle = 30.0f;

    // Rendering settings
    glm::vec4 m_BackgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    int m_ShadowMapResolution = 2048;
    
    bool m_FogEnabled = false;
    glm::vec3 m_FogColor = { 0.1f, 0.1f, 0.1f };
    float m_FogStart = 10.0f;
    float m_FogEnd = 40.0f;
};