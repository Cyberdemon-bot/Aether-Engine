#include "GameLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

GameLayer::GameLayer()
    : Layer("Spotlight Shadow Demo"), m_Camera(glm::vec3(0.0f, 5.0f, 10.0f))
{
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
    HandleInput(ts);

    glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix();
    RenderShadowPass(lightSpaceMatrix);
    RenderMainPass(window.GetFramebufferWidth(), window.GetFramebufferHeight(), lightSpaceMatrix);
}

void GameLayer::HandleInput(Aether::Timestep ts)
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
    float aspectRatio = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(m_Camera.Zoom), aspectRatio, 0.1f, 100.0f);
    glm::mat4 view = m_Camera.GetViewMatrix();

    m_CameraUBO->SetData(glm::value_ptr(projection), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(m_Camera.Position), sizeof(glm::vec3), 2 * sizeof(glm::mat4));

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

void GameLayer::OnImGuiRender()
{
    ImGui::Begin("Spotlight Controls");

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

        ImGui::Text("Direction:");
        ImGui::SliderFloat3("##Dir", &m_LightDir.x, -1.0f, 1.0f);
        if (ImGui::Button("Point Down")) m_LightDir = glm::vec3(0.0f, -1.0f, 0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Point at Center")) m_LightDir = glm::normalize(glm::vec3(0.0f) - m_LightPos);

        ImGui::Separator();
        ImGui::SliderFloat("Inner Angle", &m_InnerAngle, 1.0f, 80.0f);
        ImGui::SliderFloat("Outer Angle", &m_OuterAngle, m_InnerAngle, 90.0f);

        if (m_InnerAngle > m_OuterAngle) m_InnerAngle = m_OuterAngle;
    }

    if (ImGui::CollapsingHeader("Scene Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Cube A:");
        ImGui::SliderFloat3("Position##CubeA", &m_TranslationA.x, -8.0f, 8.0f);
        ImGui::Spacing();

        ImGui::Text("Cube B:");
        ImGui::SliderFloat3("Position##CubeB", &m_TranslationB.x, -8.0f, 8.0f);
        ImGui::Spacing();

        ImGui::SliderFloat("Cube Scale", &m_CubeScale, 0.5f, 3.0f);
        ImGui::SliderFloat("Floor Scale", &m_FloorScale, 5.0f, 30.0f);

        ImGui::Separator();
        if (ImGui::Button("Spawn Random Cube")) {
            float min = 0.5f;
            float max = 3.0f;
            float x = static_cast<float>(rand() % 300 - 150) / 10.0f;
            float y = static_cast<float>(rand() % 50 + 10) / 10.0f;
            float z = static_cast<float>(rand() % 300 - 150) / 10.0f;
            float size = min + ((float)rand() / RAND_MAX) * (max - min);
            float rot = -1.0f + ((float)rand() / RAND_MAX) * (1.0f - (-1.0f));
            m_RandomCubes.push_back({ x, y, z });
            m_CubesSize.push_back(size);
            m_CubeRot.push_back(rot);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Random Cubes")) {
            m_RandomCubes.clear();
            m_CubesSize.clear();
            m_CubeRot.clear();
        }
        ImGui::Text("Count: %d", (int)m_RandomCubes.size());
    }

    if (ImGui::CollapsingHeader("Animation")) {
        ImGui::Checkbox("Enable Rotation", &m_EnableRotation);
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, -2.0f, 2.0f);
        if (ImGui::Button("Reset Rotation")) {
            m_Rotation = 0.0f;
        }
    }

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::ColorEdit3("Background Color", &m_BackgroundColor.x);
        ImGui::Text("Shadow Map Settings:");
        if (ImGui::SliderInt("Shadow Resolution", &m_ShadowMapResolution, 512, 4096)) {
            Aether::FramebufferSpecification fbSpec;
            fbSpec.Width = m_ShadowMapResolution;
            fbSpec.Height = m_ShadowMapResolution;
            fbSpec.Attachments = { Aether::FramebufferTextureFormat::DEPTH24STENCIL8 };
            m_ShadowFBO = Aether::FrameBuffer::Create(fbSpec);
        }

        ImGui::Separator();
        ImGui::Text("Fog Settings:");
        ImGui::Checkbox("Enable Fog", &m_FogEnabled);
        ImGui::ColorEdit3("Fog Color", &m_FogColor.x);
        ImGui::DragFloat("Fog Start", &m_FogStart, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Fog End", &m_FogEnd, 0.1f, 0.0f, 200.0f);
        if (ImGui::Button("Sync Fog with BG")) {
            m_BackgroundColor = glm::vec4(m_FogColor, 1.0f);
        }
    }

    ImGui::End();
}

void GameLayer::OnEvent(Aether::Event& event) {}