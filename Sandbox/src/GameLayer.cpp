#include "GameLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

GameLayer::GameLayer()
    : Layer("Spotlight Shadow Demo")
{
    m_EditorCamera = Aether::EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
}

void GameLayer::Detach()
{
    m_VAO.reset();
    m_Shader.reset();
    m_ShadowShader.reset();
    m_Texture.reset();
    m_ShadowFBO.reset();
    m_CameraUBO.reset();
    m_InstanceVBO.reset();
    
    m_SkyboxVAO.reset();
    m_SkyboxShader.reset();
    m_SkyboxTexture.reset();
}

void GameLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    // Create cube geometry
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
    };

    uint32_t indices[] = {
        0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };

    // Create VAO using new API
    m_VAO = Aether::VertexArray::Create();
    
    // Create VBO
    Aether::Ref<Aether::VertexBuffer> vbo = Aether::VertexBuffer::Create(vertices, sizeof(vertices));
    Aether::BufferLayout layout = {
        { "a_Position", Aether::ShaderDataType::Float3 },
        { "a_Normal",   Aether::ShaderDataType::Float3 },
        { "a_TexCoord", Aether::ShaderDataType::Float2 }
    };
    vbo->SetLayout(layout);
    m_VAO->AddVertexBuffer(vbo);
    
    // Create IBO
    Aether::Ref<Aether::IndexBuffer> ibo = Aether::IndexBuffer::Create(indices, 36);
    m_VAO->SetIndexBuffer(ibo);

    // Create Camera UBO
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::UniformBuffer::Create(uboSize, 0);

    // Load shaders
    m_Shader = Aether::Shader::Create("assets/shaders/LightingShadow.shader");
    m_ShadowShader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    
    // Load texture
    m_Texture = Aether::Texture2D::Create("assets/textures/wood.jpg");

    // Initialize skybox
    InitSkybox();

    // Create shadow framebuffer
    Aether::FramebufferSpecification fbSpec;
    fbSpec.Width = m_ShadowMapResolution;
    fbSpec.Height = m_ShadowMapResolution;
    fbSpec.Attachments = { Aether::FramebufferTextureFormat::DEPTH24STENCIL8 };
    m_ShadowFBO = Aether::FrameBuffer::Create(fbSpec);

    AE_CORE_INFO("GameLayer initialized with new API!");


    Aether::FramebufferSpecification sceneFbSpec;
    sceneFbSpec.Width = Aether::Application::Get().GetWindow().GetWidth();
    sceneFbSpec.Height = Aether::Application::Get().GetWindow().GetHeight();

    sceneFbSpec.Attachments = { 
        Aether::FramebufferTextureFormat::RGBA8, 
        Aether::FramebufferTextureFormat::DEPTH24STENCIL8 
    };
    m_SceneFBO = Aether::FrameBuffer::Create(sceneFbSpec);

    m_LutShader = Aether::Shader::Create("assets/shaders/LUT.shader");
    m_LutTexture = Aether::Texture2D::Create("assets/textures/LUT.png", true, false);

    InitScreenQuad();

    AE_CORE_INFO("LUT System Initialized!");
}

void GameLayer::InitScreenQuad()
{
    // 1. Dữ liệu Đỉnh (4 đỉnh duy nhất: Trái-Trên, Trái-Dưới, Phải-Dưới, Phải-Trên)
    float quadVertices[] = { 
        // a_Position        // a_TexCoord
        -1.0f,  1.0f,        0.0f, 1.0f, // 0: Top-Left
        -1.0f, -1.0f,        0.0f, 0.0f, // 1: Bottom-Left
         1.0f, -1.0f,        1.0f, 0.0f, // 2: Bottom-Right
         1.0f,  1.0f,        1.0f, 1.0f  // 3: Top-Right
    };

    // 2. Dữ liệu Index (Thứ tự nối đỉnh thành 2 tam giác)
    // Tam giác 1: 0->1->2, Tam giác 2: 2->3->0
    uint32_t quadIndices[] = { 
        0, 1, 2, 
        2, 3, 0 
    };

    // 3. Khởi tạo VAO
    m_ScreenQuadVAO = Aether::VertexArray::Create();

    // 4. Tạo Vertex Buffer (VBO)
    Aether::Ref<Aether::VertexBuffer> quadVBO = Aether::VertexBuffer::Create(quadVertices, sizeof(quadVertices));
    Aether::BufferLayout layout = {
        { "a_Position", Aether::ShaderDataType::Float2 },
        { "a_TexCoord", Aether::ShaderDataType::Float2 }
    };
    quadVBO->SetLayout(layout);
    m_ScreenQuadVAO->AddVertexBuffer(quadVBO);

    // 5. Tạo Index Buffer (IBO) và gắn vào VAO <-- QUAN TRỌNG
    Aether::Ref<Aether::IndexBuffer> quadIBO = Aether::IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
    m_ScreenQuadVAO->SetIndexBuffer(quadIBO);
}

void GameLayer::InitSkybox()
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

    uint32_t skyboxIndices[] = {
        1, 2, 6, 6, 5, 1,
        0, 4, 7, 7, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 3, 2, 2, 1, 0,
        0, 1, 5, 5, 4, 0,
        3, 7, 6, 6, 2, 3
    };

    m_SkyboxVAO = Aether::VertexArray::Create();
    
    Aether::Ref<Aether::VertexBuffer> skyboxVBO = Aether::VertexBuffer::Create(skyboxVertices, sizeof(skyboxVertices));
    Aether::BufferLayout skyboxLayout = {
        { "a_Position", Aether::ShaderDataType::Float3 }
    };
    skyboxVBO->SetLayout(skyboxLayout);
    m_SkyboxVAO->AddVertexBuffer(skyboxVBO);
    
    Aether::Ref<Aether::IndexBuffer> skyboxIBO = Aether::IndexBuffer::Create(skyboxIndices, 36);
    m_SkyboxVAO->SetIndexBuffer(skyboxIBO);

    m_SkyboxShader = Aether::Shader::Create("assets/shaders/Skybox.shader");
    m_SkyboxTexture = Aether::TextureCube::Create("assets/textures/skybox.png");
}

void GameLayer::RenderSkybox()
{
    m_SkyboxTexture->Bind(0);
    m_SkyboxShader->Bind();
    m_SkyboxShader->SetInt("u_Skybox", 0);
    
    Aether::RenderCommand::SetDepthFunc(GL_LEQUAL);
    Aether::RenderCommand::DrawIndexed(m_SkyboxVAO);
    Aether::RenderCommand::SetDepthFunc(GL_LESS);
}

void GameLayer::Update(Aether::Timestep ts)
{
    if (m_EnableRotation) m_Rotation += ts * m_RotationSpeed;

    auto& window = Aether::Application::Get().GetWindow();
    m_EditorCamera.Update(ts);

    glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix();
    RenderShadowPass(lightSpaceMatrix);
    m_SceneFBO->Bind();
    RenderMainPass(m_SceneFBO->GetSpecification().Width, m_SceneFBO->GetSpecification().Height, lightSpaceMatrix);
    m_SceneFBO->Unbind();

    Aether::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());

    m_LutShader->Bind();

    m_SceneFBO->BindColorTexture(0); 

    m_LutShader->SetInt("u_SceneTexture", 0);

    m_LutTexture->Bind(1);
    m_LutShader->SetInt("u_LutTexture", 1);

    m_LutShader->SetFloat("u_LutIntensity", m_LutIntensity);

    Aether::RenderCommand::DrawIndexed(m_ScreenQuadVAO);
}

glm::mat4 GameLayer::CalculateLightSpaceMatrix()
{
    float aspect = 1.0f;
    float nearPlane = 1.0f;
    float farPlane = 50.0f;
    glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
    glm::mat4 lightView = glm::lookAt(m_LightPos, m_LightPos + m_LightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

void GameLayer::RenderShadowPass(const glm::mat4& lightSpaceMatrix)
{
    m_ShadowFBO->Bind();
    Aether::RenderCommand::SetViewport(0, 0, m_ShadowMapResolution, m_ShadowMapResolution);
    Aether::RenderCommand::Clear();

    m_ShadowShader->Bind();
    m_ShadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
    RenderScene(m_ShadowShader);
    
    m_ShadowFBO->Unbind();
}

void GameLayer::RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix)
{
    Aether::RenderCommand::SetViewport(0, 0, width, height);

    if (m_FogEnabled)
        Aether::RenderCommand::SetClearColor(glm::vec4(m_FogColor, 1.0f));
    else
        Aether::RenderCommand::SetClearColor(m_BackgroundColor);
    
    Aether::RenderCommand::Clear();

    // Update Camera UBO
    m_EditorCamera.SetViewportSize((float)width, (float)height);

    glm::mat4 projection = m_EditorCamera.GetProjection();
    glm::mat4 view = m_EditorCamera.GetViewMatrix();
    glm::vec3 camPos = m_EditorCamera.GetPosition();

    m_CameraUBO->SetData(glm::value_ptr(projection), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(camPos), sizeof(glm::vec3), 2 * sizeof(glm::mat4));

    // Render skybox
    RenderSkybox();

    // Main scene rendering
    m_Shader->Bind();
    
    m_Texture->Bind(0);
    m_Shader->SetInt("u_Texture", 0);
    m_ShadowFBO->BindDepthTexture(1);
    m_Shader->SetInt("u_ShadowMap", 1);

    m_Shader->SetFloat3("u_LightPos", m_LightPos);
    m_Shader->SetFloat3("u_LightDir", m_LightDir);
    m_Shader->SetFloat("u_CutOff", glm::cos(glm::radians(m_InnerAngle)));
    m_Shader->SetFloat("u_OuterCutOff", glm::cos(glm::radians(m_OuterAngle)));
    m_Shader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

    m_Shader->SetInt("u_IsLightSource", 0);
    m_Shader->SetInt("u_FogEnabled", m_FogEnabled);
    m_Shader->SetFloat3("u_FogColor", m_FogColor);
    m_Shader->SetFloat("u_FogStart", m_FogStart);
    m_Shader->SetFloat("u_FogEnd", m_FogEnd);
    
    RenderScene(m_Shader);

    // Render light source indicator
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_LightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    m_Shader->SetMat4("u_Model", model);
    m_Shader->SetInt("u_IsLightSource", 1);
    m_Shader->SetFloat3("u_FlatColor", glm::vec3(1.0f, 1.0f, 0.0f));
    Aether::RenderCommand::DrawIndexed(m_VAO);
    m_Shader->SetInt("u_IsLightSource", 0);
}

void GameLayer::RenderScene(Aether::Ref<Aether::Shader> shader)
{
    // Cube A
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
    model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetMat4("u_Model", model);
    shader->SetInt("u_UseInstancing", 0);
    Aether::RenderCommand::DrawIndexed(m_VAO);

    // Cube B
    model = glm::translate(glm::mat4(1.0f), m_TranslationB);
    model = glm::rotate(model, m_Rotation * 0.7f, glm::vec3(1.0f, 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetMat4("u_Model", model);
    Aether::RenderCommand::DrawIndexed(m_VAO);

    // Random cubes with instancing
    if (!m_RandomCubes.empty())
    {
        m_InstanceModels.clear();
        m_InstanceModels.reserve(m_RandomCubes.size());

        for (size_t i = 0; i < m_RandomCubes.size(); i++) 
        {
            glm::vec3 pos = m_RandomCubes[i];
            float size = m_CubesSize[i];
            float rot = m_CubeRot[i];
            glm::mat4 instModel = glm::translate(glm::mat4(1.0f), pos);
            instModel = glm::rotate(instModel, m_Rotation * rot, glm::vec3(0.5f, 1.0f, 0.0f));
            instModel = glm::scale(instModel, glm::vec3(size * m_CubeScale));
            m_InstanceModels.push_back(instModel);
        }

        uint32_t dataSize = (uint32_t)m_InstanceModels.size() * sizeof(glm::mat4);

        // Check if we need to recreate the buffer (doesn't exist or too small)
        if (!m_InstanceVBO || m_InstanceVBO->GetSize() < dataSize) 
        {
            // Recreate with larger size (2x current need to reduce reallocations)
            uint32_t newSize = dataSize * 2;
            m_InstanceVBO = Aether::VertexBuffer::Create(newSize);
            
            Aether::BufferLayout instanceLayout = {
                { "a_InstanceModel_Row0", Aether::ShaderDataType::Float4 },
                { "a_InstanceModel_Row1", Aether::ShaderDataType::Float4 },
                { "a_InstanceModel_Row2", Aether::ShaderDataType::Float4 },
                { "a_InstanceModel_Row3", Aether::ShaderDataType::Float4 }
            };
            m_InstanceVBO->SetLayout(instanceLayout);
            m_VAO->AddInstanceBuffer(m_InstanceVBO, 3);
        }
        
        // Upload instance data
        m_InstanceVBO->SetData(m_InstanceModels.data(), dataSize, 0);

        shader->SetInt("u_UseInstancing", 1);
        Aether::RenderCommand::DrawInstanced(m_VAO, (uint32_t)m_RandomCubes.size());
        shader->SetInt("u_UseInstancing", 0);
    }

    // Floor
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_FloorScale, 0.1f, m_FloorScale));
    shader->SetMat4("u_Model", model);
    Aether::RenderCommand::DrawIndexed(m_VAO);
}

void GameLayer::OnEvent(Aether::Event& event)
{
    // Chuyển sự kiện vào camera xử lý
    m_EditorCamera.OnEvent(event);
}

// ... (Keep all other methods the same, only replacing OnImGuiRender)

void GameLayer::OnImGuiRender()
{
    // Main control panel
    ImGui::SetNextWindowSize(ImVec2(420, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_MenuBar);

    // Menu bar for quick actions
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset Camera")) {
                m_EditorCamera.SetDistance(10.0f);
            }
            if (ImGui::MenuItem("Top View")) {
                // Set camera to look down from above
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem("Reset All")) {
                m_TranslationA = { -2.0f, 0.5f, 0.0f };
                m_TranslationB = { 2.0f, 0.5f, 0.0f };
                m_Rotation = 0.0f;
                m_RandomCubes.clear();
                m_CubesSize.clear();
                m_CubeRot.clear();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Performance stats at the top
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
    ImGui::Text("FPS: %.1f (%.2fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();
    ImGui::Separator();

    // ===== CAMERA SECTION =====
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        ImGui::TextWrapped("Controls:");
        ImGui::PopStyleColor();
        
        ImGui::Indent(10.0f);
        ImGui::BulletText("Right Mouse: Rotate (orbit around focus)");
        ImGui::BulletText("Middle Mouse: Pan view");
        ImGui::BulletText("Scroll Wheel: Zoom in/out");
        ImGui::Spacing();
        ImGui::BulletText("WASD: Move horizontally");
        ImGui::BulletText("Q/E: Move down/up");
        ImGui::BulletText("Hold Shift: Move faster");
        ImGui::BulletText("Hold Ctrl: Move slower");
        ImGui::Unindent(10.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Camera info
        glm::vec3 pos = m_EditorCamera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Distance: %.1f units", m_EditorCamera.GetDistance());
        ImGui::Text("Pitch: %.1f° | Yaw: %.1f°", 
            glm::degrees(m_EditorCamera.GetPitch()), 
            glm::degrees(m_EditorCamera.GetYaw()));
        
        if (ImGui::Button("Reset View", ImVec2(-1, 0))) {
            m_EditorCamera.SetDistance(10.0f);
        }
    }

    // ===== LIGHTING SECTION =====
    if (ImGui::CollapsingHeader("Spotlight", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::Text("Position");
        ImGui::DragFloat3("##LightPos", &m_LightPos.x, 0.1f, -20.0f, 20.0f);

        ImGui::Spacing();
        ImGui::Text("Direction");
        ImGui::SliderFloat3("##LightDir", &m_LightDir.x, -1.0f, 1.0f);
        
        // Quick direction presets
        if (ImGui::Button("Point Down", ImVec2(100, 0))) 
            m_LightDir = glm::vec3(0.0f, -1.0f, 0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Point Center", ImVec2(100, 0))) 
            m_LightDir = glm::normalize(glm::vec3(0.0f) - m_LightPos);

        ImGui::Spacing();
        ImGui::Separator();
        
        // Cone angles with visual feedback
        ImGui::Text("Cone Shape");
        ImGui::SliderFloat("Inner Angle", &m_InnerAngle, 1.0f, 80.0f, "%.1f°");
        ImGui::SliderFloat("Outer Angle", &m_OuterAngle, m_InnerAngle, 90.0f, "%.1f°");

        if (m_InnerAngle > m_OuterAngle) m_InnerAngle = m_OuterAngle;
        
        // Visual indicator
        float coneRatio = m_InnerAngle / m_OuterAngle;
        ImGui::ProgressBar(coneRatio, ImVec2(-1, 0), "");
        ImGui::SameLine(0, 10);
        ImGui::Text("Sharpness");
    }

    // ===== SCENE OBJECTS =====
    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::PushID("CubeA");
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cube A");
        ImGui::SameLine(80);
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat3("##PosA", &m_TranslationA.x, 0.1f, -15.0f, 15.0f, "%.1f");
        ImGui::PopID();

        ImGui::PushID("CubeB");
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cube B");
        ImGui::SameLine(80);
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat3("##PosB", &m_TranslationB.x, 0.1f, -15.0f, 15.0f, "%.1f");
        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Separator();
        
        ImGui::Text("Scaling");
        ImGui::SliderFloat("Cube Size", &m_CubeScale, 0.5f, 3.0f, "%.1fx");
        ImGui::SliderFloat("Floor Size", &m_FloorScale, 5.0f, 30.0f, "%.1fx");

        ImGui::Spacing();
        ImGui::Separator();
        
        // Random cubes section with better layout
        ImGui::Text("Random Cubes (%d)", (int)m_RandomCubes.size());
        
        float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) * 0.5f;
        if (ImGui::Button("+ Spawn Cube", ImVec2(buttonWidth, 30))) {
            float min = 0.5f, max = 3.0f;
            float x = static_cast<float>(rand() % 300 - 150) / 10.0f;
            float y = static_cast<float>(rand() % 50 + 10) / 10.0f;
            float z = static_cast<float>(rand() % 300 - 150) / 10.0f;
            float size = min + ((float)rand() / RAND_MAX) * (max - min);
            float rot = -1.0f + ((float)rand() / RAND_MAX) * 2.0f;
            m_RandomCubes.push_back({ x, y, z });
            m_CubesSize.push_back(size);
            m_CubeRot.push_back(rot);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All", ImVec2(buttonWidth, 30))) {
            m_RandomCubes.clear();
            m_CubesSize.clear();
            m_CubeRot.clear();
        }
    }

    // ===== ANIMATION =====
    if (ImGui::CollapsingHeader("Animation")) 
    {
        ImGui::Checkbox("Enable Rotation", &m_EnableRotation);
        
        if (m_EnableRotation) {
            ImGui::SliderFloat("Speed", &m_RotationSpeed, -2.0f, 2.0f, "%.2fx");
            
            if (ImGui::Button("Reset", ImVec2(100, 0))) {
                m_Rotation = 0.0f;
            }
        } else {
            ImGui::TextDisabled("Rotation paused");
        }
    }

    // ===== RENDERING =====
    if (ImGui::CollapsingHeader("Rendering & Effects")) 
    {
        ImGui::Text("Background");
        ImGui::ColorEdit3("Color##BG", &m_BackgroundColor.x, ImGuiColorEditFlags_NoInputs);
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Fog settings
        ImGui::Checkbox("Enable Fog", &m_FogEnabled);
        
        if (m_FogEnabled) {
            ImGui::Indent(10.0f);
            ImGui::ColorEdit3("Fog Color", &m_FogColor.x, ImGuiColorEditFlags_NoInputs);
            ImGui::DragFloat("Start Distance", &m_FogStart, 0.5f, 0.0f, m_FogEnd, "%.1f");
            ImGui::DragFloat("End Distance", &m_FogEnd, 0.5f, m_FogStart, 200.0f, "%.1f");
            
            if (ImGui::Button("Match Background", ImVec2(-1, 0))) {
                m_BackgroundColor = glm::vec4(m_FogColor, 1.0f);
            }
            ImGui::Unindent(10.0f);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Shadow settings
        ImGui::Text("Shadow Quality");
        const char* resolutions[] = { "512", "1024", "2048", "4096" };
        int currentRes = 0;
        if (m_ShadowMapResolution == 512) currentRes = 0;
        else if (m_ShadowMapResolution == 1024) currentRes = 1;
        else if (m_ShadowMapResolution == 2048) currentRes = 2;
        else if (m_ShadowMapResolution == 4096) currentRes = 3;
        
        if (ImGui::Combo("Resolution", &currentRes, resolutions, 4)) {
            int newRes[] = { 512, 1024, 2048, 4096 };
            m_ShadowMapResolution = newRes[currentRes];
            
            Aether::FramebufferSpecification fbSpec;
            fbSpec.Width = m_ShadowMapResolution;
            fbSpec.Height = m_ShadowMapResolution;
            fbSpec.Attachments = { Aether::FramebufferTextureFormat::DEPTH24STENCIL8 };
            m_ShadowFBO = Aether::FrameBuffer::Create(fbSpec);
        }

            ImGui::Spacing();
            ImGui::Separator();
            
            // --- THÊM PHẦN NÀY ---
            ImGui::Text("Color Grading (LUT)");
            ImGui::SliderFloat("Intensity", &m_LutIntensity, 0.0f, 1.0f);
            
            // Hiển thị ảnh LUT cho đẹp (Optional)
            ImGui::Image((void*)(intptr_t)m_LutTexture->GetRendererID(), ImVec2(256, 16));
            // ---------------------

            ImGui::Spacing();
            ImGui::Separator();
    }

    ImGui::End();
}