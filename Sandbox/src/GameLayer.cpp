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
    m_VAO.reset(); m_VBO.reset(); m_IBO.reset();
    m_Shader.reset(); m_ShadowShader.reset();
    m_Texture.reset(); m_ShadowFBO.reset();
    m_CameraUBO.reset();
    
    // Clean up skybox resources
    m_SkyboxVAO.reset();
    m_SkyboxVBO.reset();
    m_SkyboxShader.reset();
    m_SkyboxTexture.reset();
}

void GameLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    Aether::Legacy::LegacyAPI::Init();

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

    unsigned int indices[] = {
        0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,10, 10,11,8,
        12,13,14, 14,15,12, 16,17,18, 18,19,16, 20,21,22, 22,23,20
    };

    m_VAO = std::make_shared<Aether::Legacy::VertexArray>();
    m_VBO = std::make_shared<Aether::Legacy::VertexBuffer>(vertices, sizeof(vertices));
    Aether::Legacy::VertexBufferLayout layout;
    layout.Push<float>(3); layout.Push<float>(3); layout.Push<float>(2);
    m_VAO->AddBuffer(*m_VBO, layout);
    m_IBO = std::make_shared<Aether::Legacy::IndexBuffer>(indices, 36);

    // UBO setup
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = std::make_shared<Aether::Legacy::UniformBuffer>(uboSize, 0);

    m_Shader = std::make_shared<Aether::Legacy::Shader>("assets/shaders/LightingShadow.shader");
    m_ShadowShader = std::make_shared<Aether::Legacy::Shader>("assets/shaders/ShadowMap.shader");
    m_Texture = std::make_shared<Aether::Legacy::Texture>("assets/textures/wood.jpg");

    m_Shader->Bind();
    m_Shader->BindUniformBlock("CameraData", 0);

    // Initialize Skybox
    InitSkybox();

    Aether::Legacy::FramebufferSpecification fbSpec;
    fbSpec.Width = m_ShadowMapResolution;
    fbSpec.Height = m_ShadowMapResolution;
    fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
    m_ShadowFBO = std::make_shared<Aether::Legacy::FrameBuffer>(fbSpec);
}

void GameLayer::InitSkybox()
{
    // Skybox cube vertices
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    m_SkyboxVAO = std::make_shared<Aether::Legacy::VertexArray>();
    m_SkyboxVBO = std::make_shared<Aether::Legacy::VertexBuffer>(skyboxVertices, sizeof(skyboxVertices));
    
    Aether::Legacy::VertexBufferLayout layout;
    layout.Push<float>(3); // Only position
    m_SkyboxVAO->AddBuffer(*m_SkyboxVBO, layout);

    m_SkyboxShader = std::make_shared<Aether::Legacy::Shader>("assets/shaders/Skybox.shader");
    m_SkyboxTexture = std::make_shared<Aether::Legacy::TextureCube>("assets/textures/skybox.png");

    m_SkyboxShader->Bind();
    m_SkyboxShader->BindUniformBlock("CameraData", 0);
}

void GameLayer::RenderSkybox()
{
    glDepthFunc(GL_LEQUAL);
    
    m_SkyboxShader->Bind();
    m_SkyboxTexture->Bind(0);
    m_SkyboxShader->SetUniform1i("u_Skybox", 0);
    
    m_SkyboxVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    glDepthFunc(GL_LESS);
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
    Aether::Legacy::LegacyAPI::SetViewport(0, 0, m_ShadowMapResolution, m_ShadowMapResolution);
    Aether::Legacy::LegacyAPI::Clear();

    m_ShadowShader->Bind();
    m_ShadowShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
    RenderScene(m_ShadowShader);
    m_ShadowFBO->Unbind();
}

void GameLayer::RenderMainPass(uint32_t width, uint32_t height, const glm::mat4& lightSpaceMatrix)
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

    // Render skybox first
    RenderSkybox();

    // Render scene
    m_Shader->Bind();
    
    m_Texture->Bind(0);
    m_Shader->SetUniform1i("u_Texture", 0);
    m_ShadowFBO->BindDepthTexture(1);
    m_Shader->SetUniform1i("u_ShadowMap", 1);

    m_Shader->SetUniform3f("u_LightPos", m_LightPos);
    m_Shader->SetUniform3f("u_LightDir", m_LightDir);
    m_Shader->SetUniform1f("u_CutOff", glm::cos(glm::radians(m_InnerAngle)));
    m_Shader->SetUniform1f("u_OuterCutOff", glm::cos(glm::radians(m_OuterAngle)));
    m_Shader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);

    m_Shader->SetUniform1i("u_IsLightSource", 0);
    RenderScene(m_Shader);

    // Draw light source cube
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_LightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    m_Shader->SetUniformMat4f("u_Model", model);
    m_Shader->SetUniform1i("u_IsLightSource", 1);
    m_Shader->SetUniform3f("u_FlatColor", glm::vec3(1.0f, 1.0f, 0.0f));
    Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *m_Shader);
    m_Shader->SetUniform1i("u_IsLightSource", 0);

    m_Shader->SetUniform1i("u_FogEnabled", m_FogEnabled);
    m_Shader->SetUniform3f("u_FogColor", m_FogColor);
    m_Shader->SetUniform1f("u_FogStart", m_FogStart);
    m_Shader->SetUniform1f("u_FogEnd", m_FogEnd);
}

void GameLayer::RenderScene(std::shared_ptr<Aether::Legacy::Shader> shader)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
    model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetUniformMat4f("u_Model", model);
    Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);

    model = glm::translate(glm::mat4(1.0f), m_TranslationB);
    model = glm::rotate(model, m_Rotation * 0.7f, glm::vec3(1.0f, 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetUniformMat4f("u_Model", model);
    Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);

    for (const auto& pos : m_RandomCubes) {
        model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(m_CubeScale));
        shader->SetUniformMat4f("u_Model", model);
        Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);
    }

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_FloorScale, 0.1f, m_FloorScale));
    shader->SetUniformMat4f("u_Model", model);
    Aether::Legacy::LegacyAPI::Draw(*m_VAO, *m_IBO, *shader);
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
        ImGui::SliderFloat("FOV (Zoom)", &m_Camera.Zoom, 30.0f, 120.0f);
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
            float x = static_cast<float>(rand() % 300 - 150) / 10.0f;
            float y = static_cast<float>(rand() % 50 + 10) / 10.0f;
            float z = static_cast<float>(rand() % 300 - 150) / 10.0f;
            m_RandomCubes.push_back({ x, y, z });
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Random Cubes")) {
            m_RandomCubes.clear();
        }
        ImGui::Text("Count: %d", (int)m_RandomCubes.size());
    }

    if (ImGui::CollapsingHeader("Animation")) {
        ImGui::Checkbox("Enable Rotation", &m_EnableRotation);
        ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, 0.0f, 2.0f);
        if (ImGui::Button("Reset Rotation")) {
            m_Rotation = 0.0f;
        }
    }

    if (ImGui::CollapsingHeader("Rendering")) {
        ImGui::ColorEdit3("Background Color", &m_BackgroundColor.x);
        ImGui::Text("Shadow Map Settings:");
        if (ImGui::SliderInt("Shadow Resolution", &m_ShadowMapResolution, 512, 4096)) {
            Aether::Legacy::FramebufferSpecification fbSpec;
            fbSpec.Width = m_ShadowMapResolution;
            fbSpec.Height = m_ShadowMapResolution;
            fbSpec.Attachments = { Aether::Legacy::FramebufferTextureFormat::DEPTH24STENCIL8 };
            m_ShadowFBO = std::make_shared<Aether::Legacy::FrameBuffer>(fbSpec);
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