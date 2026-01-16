#include "PBRLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

PBRLayer::PBRLayer()
    : Layer("PBR Mesh Demo"), m_Camera(glm::vec3(0.0f, 5.0f, 15.0f))
{
}

void PBRLayer::Detach()
{
    m_MeshVAOs.clear();
    m_MeshVBOs.clear();
    m_MeshIBOs.clear();
    m_Meshes.clear();
    
    m_PBRShader.reset();
    m_ShadowShader.reset();
    m_ShadowFBO.reset();
    m_CameraUBO.reset();
    
    m_SkyboxVAO.reset();
    m_SkyboxVBO.reset();
    m_SkyboxIBO.reset();
    m_SkyboxShader.reset();
    m_SkyboxTexture.reset();
}

void PBRLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    Aether::Legacy::LegacyAPI::Init();

    // Create camera UBO
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::CreateRef<Aether::Legacy::UniformBuffer>(uboSize, 0);

    // Load shaders
    m_PBRShader = Aether::CreateRef<Aether::Legacy::Shader>("assets/shaders/PBR.shader");
    m_ShadowShader = Aether::CreateRef<Aether::Legacy::Shader>("assets/shaders/PBRShadowMap.shader");
    
    m_PBRShader->Bind();
    m_PBRShader->BindUniformBlock("CameraData", 0);

    // Initialize skybox
    InitSkybox();

    // Setup shadow framebuffer
    Aether::Legacy::FramebufferSpecification fbSpec;
    fbSpec.Width = m_ShadowMapResolution;
    fbSpec.Height = m_ShadowMapResolution;
    fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
    m_ShadowFBO = Aether::CreateRef<Aether::Legacy::FrameBuffer>(fbSpec);

    // Create test meshes
    m_Meshes.push_back(CreateCubeMesh("Floor"));
    m_Meshes.push_back(CreateCubeMesh("Test Cube 1"));
    m_Meshes.push_back(CreateCubeMesh("Test Cube 2"));
    m_Meshes.push_back(CreateCubeMesh("Test Cube 3"));
    m_Meshes.push_back(CreateCubeMesh("Test Cube 4"));

    // Setup rendering for each mesh
    for (auto& mesh : m_Meshes) {
        SetupMeshRendering(mesh);
    }

    // Setup mesh instances with different PBR materials
    // Floor
    m_MeshInstances.push_back({
        m_FloorPosition, glm::vec3(0.0f), m_FloorScale,
        glm::vec3(0.3f, 0.3f, 0.3f), 0.0f, 0.8f, 1.0f
    });
    
    // Rough plastic (non-metal)
    m_MeshInstances.push_back({
        glm::vec3(-4.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f), 0.0f, 0.9f, 1.0f
    });
    
    // Smooth plastic
    m_MeshInstances.push_back({
        glm::vec3(-2.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.2f, 1.0f
    });
    
    // Rough metal
    m_MeshInstances.push_back({
        glm::vec3(2.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f),
        glm::vec3(1.0f, 0.84f, 0.0f), 1.0f, 0.6f, 1.0f
    });
    
    // Polished metal
    m_MeshInstances.push_back({
        glm::vec3(4.0f, 1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f),
        glm::vec3(0.9f, 0.9f, 0.95f), 1.0f, 0.1f, 1.0f
    });
}

Aether::Mesh PBRLayer::CreateCubeMesh(const std::string& name)
{
    std::vector<Aether::Vertex> vertices;
    std::vector<uint32_t> indices;

    // Cube vertices with proper normals for each face
    // Front face (Z+)
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Back face (Z-)
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Left face (X-)
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Right face (X+)
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Bottom face (Y-)
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Top face (Y+)
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});
    vertices.emplace_back(Aether::Vertex{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {-1, -1, -1, -1}, {0.0f, 0.0f, 0.0f, 0.0f}, 0.0f});

    // Indices (2 triangles per face)
    for (uint32_t i = 0; i < 6; i++) {
        uint32_t base = i * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }

    return Aether::Mesh(vertices, indices, name);
}

void PBRLayer::SetupMeshRendering(Aether::Mesh& mesh)
{
    const auto& data = mesh.GetData();
    
    // Create VAO
    auto vao = Aether::CreateRef<Aether::Legacy::VertexArray>();
    
    // Create VBO
    auto vbo = Aether::CreateRef<Aether::Legacy::VertexBuffer>(
        data.Vertices.data(), 
        static_cast<uint32_t>(data.Vertices.size() * sizeof(Aether::Vertex))
    );
    
    // Setup vertex layout for Vertex struct
    Aether::Legacy::VertexBufferLayout layout;
    layout.Push<float>(3); // Position
    layout.Push<float>(3); // Normal
    layout.Push<float>(3); // Tangent
    layout.Push<float>(2); // TexCoord
    layout.Push<unsigned int>(4);   // BoneIDs
    layout.Push<float>(4); // Weights
    layout.Push<float>(1); // Orientation
    
    vao->AddBuffer(*vbo, layout);
    
    // Create IBO
    auto ibo = Aether::CreateRef<Aether::Legacy::IndexBuffer>(
        data.Indices.data(), 
        static_cast<uint32_t>(data.Indices.size())
    );
    
    m_MeshVAOs.push_back(vao);
    m_MeshVBOs.push_back(vbo);
    m_MeshIBOs.push_back(ibo);
}

void PBRLayer::InitSkybox()
{
    float skyboxVertices[] = {
        -1.0f, -1.0f,  1.0f, 
         1.0f, -1.0f,  1.0f, 
         1.0f, -1.0f, -1.0f, 
        -1.0f, -1.0f, -1.0f, 
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, 
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f  
    };

    unsigned int skyboxIndices[] = {
        1, 2, 6, 6, 5, 1,
        0, 4, 7, 7, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 3, 2, 2, 1, 0,
        0, 1, 5, 5, 4, 0,
        3, 7, 6, 6, 2, 3
    };

    m_SkyboxVAO = Aether::CreateRef<Aether::Legacy::VertexArray>();
    m_SkyboxVBO = Aether::CreateRef<Aether::Legacy::VertexBuffer>(skyboxVertices, sizeof(skyboxVertices));
    m_SkyboxIBO = Aether::CreateRef<Aether::Legacy::IndexBuffer>(skyboxIndices, 36);
    
    Aether::Legacy::VertexBufferLayout layout;
    layout.Push<float>(3);
    m_SkyboxVAO->AddBuffer(*m_SkyboxVBO, layout);

    m_SkyboxShader = Aether::CreateRef<Aether::Legacy::Shader>("assets/shaders/Skybox.shader");
    m_SkyboxTexture = Aether::CreateRef<Aether::Legacy::TextureCube>("assets/textures/skybox.png");
    
    m_SkyboxShader->Bind();
    m_SkyboxShader->BindUniformBlock("CameraData", 0);
    m_SkyboxShader->SetUniform1i("u_Skybox", 0);
}

void PBRLayer::RenderSkybox()
{
    m_SkyboxTexture->Bind(0);
    Aether::Legacy::LegacyAPI::SetDepthFunc(GL_LEQUAL);
    Aether::Legacy::LegacyAPI::Draw(*m_SkyboxVAO, *m_SkyboxIBO, *m_SkyboxShader);
    Aether::Legacy::LegacyAPI::SetDepthFunc(GL_LESS);
}

void PBRLayer::Update(Aether::Timestep ts)
{
    if (m_EnableRotation) m_Rotation += ts * m_RotationSpeed;

    auto& window = Aether::Application::Get().GetWindow();
    HandleInput(ts);

    glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix();
    RenderShadowPass(lightSpaceMatrix);
    RenderMainPass(window.GetFramebufferWidth(), window.GetFramebufferHeight(), lightSpaceMatrix);
}

void PBRLayer::HandleInput(Aether::Timestep ts)
{
    if (Aether::Input::IsKeyPressed(Aether::Key::Escape)) {
        m_CursorLocked = false;
        Aether::Input::SetCursorMode(Aether::CursorMode::Normal);
    }
    if (Aether::Input::IsMouseButtonPressed(Aether::Mouse::ButtonLeft) && !m_CursorLocked && !ImGui::GetIO().WantCaptureMouse) {
        m_CursorLocked = true;
        Aether::Input::SetCursorMode(Aether::CursorMode::Locked);
        m_LastMousePos = Aether::Input::GetMousePosition();
    }
    if (m_CursorLocked) {
        if (Aether::Input::IsKeyPressed(Aether::Key::W)) m_Camera.ProcessKeyboard(Aether::Legacy::FORWARD, ts);
        if (Aether::Input::IsKeyPressed(Aether::Key::S)) m_Camera.ProcessKeyboard(Aether::Legacy::BACKWARD, ts);
        if (Aether::Input::IsKeyPressed(Aether::Key::A)) m_Camera.ProcessKeyboard(Aether::Legacy::LEFT, ts);
        if (Aether::Input::IsKeyPressed(Aether::Key::D)) m_Camera.ProcessKeyboard(Aether::Legacy::RIGHT, ts);
        if (Aether::Input::IsKeyPressed(Aether::Key::Space)) m_Camera.ProcessKeyboard(Aether::Legacy::UP, ts);
        if (Aether::Input::IsKeyPressed(Aether::Key::LeftControl)) m_Camera.ProcessKeyboard(Aether::Legacy::DOWN, ts);

        glm::vec2 currentPos = Aether::Input::GetMousePosition();
        glm::vec2 delta = currentPos - m_LastMousePos;
        m_LastMousePos = currentPos;
        m_Camera.ProcessMouseMovement(delta.x, -delta.y);
    }
}

glm::mat4 PBRLayer::CalculateLightSpaceMatrix()
{
    float aspect = 1.0f;
    float nearPlane = 1.0f;
    float farPlane = 50.0f;
    glm::mat4 lightProjection = glm::perspective(glm::radians(m_OuterAngle * 2.0f), aspect, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(m_LightPos, m_LightPos + m_LightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

void PBRLayer::RenderShadowPass(const glm::mat4& lightSpaceMatrix)
{
    m_ShadowFBO->Bind();
    Aether::Legacy::LegacyAPI::SetViewport(0, 0, m_ShadowMapResolution, m_ShadowMapResolution);
    Aether::Legacy::LegacyAPI::Clear();

    m_ShadowShader->Bind();
    m_ShadowShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
    RenderMeshes(m_ShadowShader);
    
    m_ShadowFBO->Unbind();
}

void PBRLayer::RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix)
{
    Aether::Legacy::LegacyAPI::SetViewport(0, 0, width, height);

    if (m_FogEnabled)
        Aether::Legacy::LegacyAPI::SetClearColor(glm::vec4(m_FogColor, 1.0f));
    else
        Aether::Legacy::LegacyAPI::SetClearColor(m_BackgroundColor);
    
    Aether::Legacy::LegacyAPI::Clear();

    // Update Camera UBO
    float aspectRatio = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(m_Camera.Zoom), aspectRatio, 0.1f, 100.0f);
    glm::mat4 view = m_Camera.GetViewMatrix();

    m_CameraUBO->SetData(glm::value_ptr(projection), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(m_Camera.Position), sizeof(glm::vec3), 2 * sizeof(glm::mat4));

    RenderSkybox();

    // Render meshes with PBR
    m_PBRShader->Bind();
    
    // Bind shadow map
    m_ShadowFBO->BindDepthTexture(0);
    m_PBRShader->SetUniform1i("u_ShadowMap", 0);
    
    // Bind skybox for IBL
    m_SkyboxTexture->Bind(1);
    m_PBRShader->SetUniform1i("u_IrradianceMap", 1);
    
    // Set spotlight parameters
    m_PBRShader->SetUniform3f("u_LightPos", m_LightPos);
    m_PBRShader->SetUniform3f("u_LightDir", m_LightDir);
    m_PBRShader->SetUniform3f("u_LightColor", m_LightColor);
    m_PBRShader->SetUniform1f("u_LightIntensity", m_LightIntensity);
    m_PBRShader->SetUniform1f("u_CutOff", glm::cos(glm::radians(m_InnerAngle)));
    m_PBRShader->SetUniform1f("u_OuterCutOff", glm::cos(glm::radians(m_OuterAngle)));
    m_PBRShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
    
    // Fog settings
    m_PBRShader->SetUniform1i("u_FogEnabled", m_FogEnabled);
    m_PBRShader->SetUniform3f("u_FogColor", m_FogColor);
    m_PBRShader->SetUniform1f("u_FogStart", m_FogStart);
    m_PBRShader->SetUniform1f("u_FogEnd", m_FogEnd);
    
    RenderMeshes(m_PBRShader);
    
    // Render light source indicator
    if (m_MeshVAOs.size() > 1) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), m_LightPos);
        model = glm::scale(model, glm::vec3(0.3f));
        m_PBRShader->SetUniformMat4f("u_Model", model);
        m_PBRShader->SetUniform3f("u_Albedo", glm::vec3(1.0f, 1.0f, 0.0f));
        m_PBRShader->SetUniform1f("u_Metallic", 0.0f);
        m_PBRShader->SetUniform1f("u_Roughness", 1.0f);
        m_PBRShader->SetUniform1f("u_AO", 1.0f);
        m_PBRShader->SetUniform1i("u_IsLightSource", 1);
        Aether::Legacy::LegacyAPI::Draw(*m_MeshVAOs[1], *m_MeshIBOs[1], *m_PBRShader);
    }
}

void PBRLayer::RenderMeshes(Aether::Ref<Aether::Legacy::Shader> shader)
{
    for (size_t i = 0; i < m_Meshes.size(); i++) {
        if (i >= m_MeshInstances.size()) continue;
        
        const auto& instance = m_MeshInstances[i];
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), instance.position);
        
        // Apply rotation (with animation for non-floor meshes)
        if (i > 0 && m_EnableRotation) {
            model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
        }
        
        model = glm::scale(model, instance.scale);
        
        shader->SetUniformMat4f("u_Model", model);
        
        // Set PBR material properties (only for PBR shader)
        if (shader == m_PBRShader) {
            shader->SetUniform3f("u_Albedo", instance.albedo);
            shader->SetUniform1f("u_Metallic", instance.metallic);
            shader->SetUniform1f("u_Roughness", instance.roughness);
            shader->SetUniform1f("u_AO", instance.ao);
            shader->SetUniform1i("u_IsLightSource", 0);
        }
        
        Aether::Legacy::LegacyAPI::Draw(*m_MeshVAOs[i], *m_MeshIBOs[i], *shader);
    }
}

void PBRLayer::OnImGuiRender()
{
    ImGui::Begin("PBR Mesh Demo");

    ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)",
        m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Controls:");
        ImGui::BulletText("Left Click: Lock/Unlock cursor");
        ImGui::BulletText("WASD: Move horizontally");
        ImGui::BulletText("Space/Ctrl: Move up/down");
        ImGui::BulletText("Mouse: Look around");
        ImGui::BulletText("ESC: Unlock cursor");
        ImGui::Spacing();
        ImGui::SliderFloat("Movement Speed", &m_Camera.MovementSpeed, 1.0f, 20.0f);
        ImGui::SliderFloat("Mouse Sensitivity", &m_Camera.MouseSensitivity, 0.05f, 0.5f);
        ImGui::SliderFloat("FOV (Zoom)", &m_Camera.Zoom, 0.0f, 120.0f);
    }

    if (ImGui::CollapsingHeader("Spotlight Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Position", &m_LightPos.x, 0.1f);
        ImGui::DragFloat3("Direction", &m_LightDir.x, 0.01f, -1.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", &m_LightColor.x);
        ImGui::DragFloat("Intensity", &m_LightIntensity, 1.0f, 0.0f, 1000.0f);
        ImGui::SliderFloat("Inner Angle", &m_InnerAngle, 1.0f, 80.0f);
        ImGui::SliderFloat("Outer Angle", &m_OuterAngle, m_InnerAngle, 90.0f);
        if (m_InnerAngle > m_OuterAngle) m_InnerAngle = m_OuterAngle;
    }

    if (ImGui::CollapsingHeader("Mesh Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* meshNames[] = { "Floor", "Cube 1 (Rough Plastic)", "Cube 2 (Smooth Plastic)", 
                                     "Cube 3 (Rough Metal)", "Cube 4 (Polished Metal)" };
        ImGui::Combo("Select Mesh", &m_SelectedMesh, meshNames, IM_ARRAYSIZE(meshNames));
        
        if (m_SelectedMesh < (int)m_MeshInstances.size()) {
            auto& instance = m_MeshInstances[m_SelectedMesh];
            ImGui::Separator();
            ImGui::Text("Transform:");
            ImGui::DragFloat3("Position", &instance.position.x, 0.1f);
            ImGui::DragFloat3("Scale", &instance.scale.x, 0.1f, 0.1f, 10.0f);
            ImGui::Separator();
            ImGui::Text("PBR Material:");
            ImGui::ColorEdit3("Albedo", &instance.albedo.x);
            ImGui::SliderFloat("Metallic", &instance.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &instance.roughness, 0.0f, 1.0f);
            ImGui::SliderFloat("AO", &instance.ao, 0.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Animation")) {
        ImGui::Checkbox("Enable Rotation", &m_EnableRotation);
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, -2.0f, 2.0f);
        if (ImGui::Button("Reset Rotation")) m_Rotation = 0.0f;
    }

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::ColorEdit3("Background Color", &m_BackgroundColor.x);
        ImGui::Text("Shadow Map Settings:");
        if (ImGui::SliderInt("Shadow Resolution", &m_ShadowMapResolution, 512, 4096)) {
            Aether::Legacy::FramebufferSpecification fbSpec;
            fbSpec.Width = m_ShadowMapResolution;
            fbSpec.Height = m_ShadowMapResolution;
            fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
            m_ShadowFBO = Aether::CreateRef<Aether::Legacy::FrameBuffer>(fbSpec);
        }
        ImGui::Separator();
        ImGui::Text("Fog Settings:");
        ImGui::Checkbox("Enable Fog", &m_FogEnabled);
        ImGui::ColorEdit3("Fog Color", &m_FogColor.x);
        ImGui::DragFloat("Fog Start", &m_FogStart, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Fog End", &m_FogEnd, 0.1f, 0.0f, 200.0f);
    }

    ImGui::End();
}

void PBRLayer::OnEvent(Aether::Event& event) {}