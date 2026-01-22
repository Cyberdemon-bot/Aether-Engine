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
    m_CubeMesh.reset();
    m_ShadowFBO.reset();
    m_CameraUBO.reset();
    m_InstanceVBO.reset();
    
    m_SkyboxMesh.reset();
    m_SkyboxTexture.reset();
    
    m_SceneFBO.reset();
    m_ScreenQuadMesh.reset();

    m_LightingMaterial.reset();
    m_ShadowMaterial.reset();
    m_SkyboxMaterial.reset();
    m_LUTMaterial.reset();
}

void GameLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    // ===== LOAD ALL SHADERS =====
    m_ShaderLibrary.Load("Lighting", "assets/shaders/LightingShadow.shader");
    m_ShaderLibrary.Load("Shadow", "assets/shaders/ShadowMap.shader");
    m_ShaderLibrary.Load("Skybox", "assets/shaders/Skybox.shader");
    m_ShaderLibrary.Load("LUT", "assets/shaders/LUT.shader");

    // ===== LOAD ALL TEXTURES =====
    m_TextureLibrary.Load("Wood", "assets/textures/wood.jpg");
    m_TextureLibrary.Load("LUT", "assets/textures/LUT.png", true, false);

    m_ShadowMaterial = Aether::CreateRef<Aether::Material>(m_ShaderLibrary.Get("Shadow"));
    m_SkyboxMaterial = Aether::CreateRef<Aether::Material>(m_ShaderLibrary.Get("Skybox"));

    m_LightingMaterial = Aether::CreateRef<Aether::Material>(m_ShaderLibrary.Get("Lighting"));
    m_LightingMaterial->SetTexture("u_Texture", m_TextureLibrary.Get("Wood"));

    m_LUTMaterial = Aether::CreateRef<Aether::Material>(m_ShaderLibrary.Get("LUT"));
    m_LUTMaterial->SetTexture("u_LutTexture", m_TextureLibrary.Get("LUT"));

    // ===== CREATE CUBE GEOMETRY USING MESH =====
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

    Aether::BufferLayout cubeLayout = {
        { "a_Position", Aether::ShaderDataType::Float3 },
        { "a_Normal",   Aether::ShaderDataType::Float3 },
        { "a_TexCoord", Aether::ShaderDataType::Float2 }
    };

    m_CubeMesh = Aether::CreateRef<Aether::Mesh>(vertices, 24, indices, 36, cubeLayout);

    // ===== CREATE CAMERA UBO =====
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::UniformBuffer::Create(uboSize, 0);

    // ===== INITIALIZE SUBSYSTEMS =====
    InitSkybox();
    InitScreenQuad();

    // ===== CREATE SHADOW FRAMEBUFFER =====
    Aether::FramebufferSpecification fbSpec;
    fbSpec.Width = m_ShadowMapResolution;
    fbSpec.Height = m_ShadowMapResolution;
    fbSpec.Attachments = { Aether::FramebufferTextureFormat::DEPTH24STENCIL8 };
    m_ShadowFBO = Aether::FrameBuffer::Create(fbSpec);

    // ===== CREATE SCENE FRAMEBUFFER =====
    Aether::FramebufferSpecification sceneFbSpec;
    sceneFbSpec.Width = Aether::Application::Get().GetWindow().GetWidth();
    sceneFbSpec.Height = Aether::Application::Get().GetWindow().GetHeight();
    sceneFbSpec.Attachments = { 
        Aether::FramebufferTextureFormat::RGBA8, 
        Aether::FramebufferTextureFormat::DEPTH24STENCIL8 
    };
    m_SceneFBO = Aether::FrameBuffer::Create(sceneFbSpec);

    AE_CORE_INFO("GameLayer initialized successfully!");
}

void GameLayer::InitScreenQuad()
{
    float quadVertices[] = { 
        // a_Position   // a_TexCoord
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    uint32_t quadIndices[] = { 
        0, 1, 2, 
        2, 3, 0 
    };

    Aether::BufferLayout quadLayout = {
        { "a_Position", Aether::ShaderDataType::Float2 },
        { "a_TexCoord", Aether::ShaderDataType::Float2 }
    };

    m_ScreenQuadMesh = Aether::CreateRef<Aether::Mesh>(quadVertices, 4, quadIndices, 6, quadLayout);
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

    Aether::BufferLayout skyboxLayout = {
        { "a_Position", Aether::ShaderDataType::Float3 }
    };

    m_SkyboxMesh = Aether::CreateRef<Aether::Mesh>(skyboxVertices, 8, skyboxIndices, 36, skyboxLayout);
    m_SkyboxTexture = Aether::TextureCube::Create("assets/textures/skybox.png");
}

void GameLayer::RenderSkybox()
{
    m_SkyboxMaterial->Bind(0);
    
    // Manually bind cubemap since Material only handles Texture2D
    m_SkyboxTexture->Bind(0);
    m_SkyboxMaterial->GetShader()->SetInt("u_Skybox", 0);
    
    Aether::RenderCommand::SetDepthFuncEqual();
    Aether::RenderCommand::DrawIndexed(m_SkyboxMesh->GetVertexArray());
    Aether::RenderCommand::SetDepthFuncEqual(false);
}

void GameLayer::Update(Aether::Timestep ts)
{
    if (m_EnableRotation) m_Rotation += ts * m_RotationSpeed;

    auto& window = Aether::Application::Get().GetWindow();
    m_EditorCamera.Update(ts);

    // ===== SHADOW PASS =====
    glm::mat4 lightSpaceMatrix = CalculateLightSpaceMatrix();
    RenderShadowPass(lightSpaceMatrix);
    
    // ===== MAIN RENDERING PASS =====
    m_SceneFBO->Bind();
    RenderMainPass(m_SceneFBO->GetSpecification().Width, m_SceneFBO->GetSpecification().Height, lightSpaceMatrix);
    m_SceneFBO->Unbind();

    // ===== POST-PROCESSING PASS (LUT) =====
    Aether::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());

    // Bind scene texture manually to slot 0, then bind LUT material which will bind LUT texture to slot 1
    m_SceneFBO->BindColorTexture(0);
    m_LUTMaterial->GetShader()->SetInt("u_SceneTexture", 0);
    
    m_LUTMaterial->Bind(1); // LUT texture starts at slot 1
    m_LUTMaterial->SetFloat("u_LutIntensity", m_LutIntensity);
    m_LUTMaterial->UploadMaterial();

    Aether::RenderCommand::DrawIndexed(m_ScreenQuadMesh->GetVertexArray());
}

void GameLayer::RenderScene(Aether::Ref<Aether::Material> material)
{
    auto cubeVAO = m_CubeMesh->GetVertexArray();
    auto shader = material->GetShader();

    // Cube A
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
    model = glm::rotate(model, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetMat4("u_Model", model);
    shader->SetInt("u_UseInstancing", 0);
    Aether::RenderCommand::DrawIndexed(cubeVAO);

    // Cube B
    model = glm::translate(glm::mat4(1.0f), m_TranslationB);
    model = glm::rotate(model, m_Rotation * 0.7f, glm::vec3(1.0f, 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(m_CubeScale));
    shader->SetMat4("u_Model", model);
    Aether::RenderCommand::DrawIndexed(cubeVAO);

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

        if (!m_InstanceVBO || m_InstanceVBO->GetSize() < dataSize) 
        {
            uint32_t newSize = dataSize * 2;
            m_InstanceVBO = Aether::VertexBuffer::Create(newSize);
            
            Aether::BufferLayout instanceLayout = {
                { "a_InstanceModel", Aether::ShaderDataType::Mat4 }
            };
            m_InstanceVBO->SetLayout(instanceLayout);
            cubeVAO->AddInstanceBuffer(m_InstanceVBO, 3);
        }
        
        m_InstanceVBO->SetData(m_InstanceModels.data(), dataSize, 0);

        shader->SetInt("u_UseInstancing", 1);
        Aether::RenderCommand::DrawInstanced(cubeVAO, (uint32_t)m_RandomCubes.size());
        shader->SetInt("u_UseInstancing", 0);
    }

    // Floor
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    model = glm::scale(model, glm::vec3(m_FloorScale, 0.1f, m_FloorScale));
    shader->SetMat4("u_Model", model);
    Aether::RenderCommand::DrawIndexed(cubeVAO);
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

    m_ShadowMaterial->Bind(0);
    m_ShadowMaterial->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
    m_ShadowMaterial->UploadMaterial();
    
    RenderScene(m_ShadowMaterial);
    
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

    // Main scene rendering with lighting material
    m_LightingMaterial->Bind(0); // Wood texture at slot 0
    
    // Shadow map at slot 1 (manual bind to avoid overwriting)
    m_ShadowFBO->BindDepthTexture(1);
    m_LightingMaterial->GetShader()->SetInt("u_ShadowMap", 1);

    // Set lighting uniforms
    m_LightingMaterial->SetFloat3("u_LightPos", m_LightPos);
    m_LightingMaterial->SetFloat3("u_LightDir", m_LightDir);
    m_LightingMaterial->SetFloat("u_CutOff", glm::cos(glm::radians(m_InnerAngle)));
    m_LightingMaterial->SetFloat("u_OuterCutOff", glm::cos(glm::radians(m_OuterAngle)));
    m_LightingMaterial->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

    m_LightingMaterial->SetInt("u_IsLightSource", 0);
    m_LightingMaterial->SetInt("u_FogEnabled", m_FogEnabled);
    m_LightingMaterial->SetFloat3("u_FogColor", m_FogColor);
    m_LightingMaterial->SetFloat("u_FogStart", m_FogStart);
    m_LightingMaterial->SetFloat("u_FogEnd", m_FogEnd);
    
    m_LightingMaterial->UploadMaterial();
    
    RenderScene(m_LightingMaterial);

    // Render light source indicator
    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_LightPos);
    model = glm::scale(model, glm::vec3(0.2f));
    m_LightingMaterial->GetShader()->SetMat4("u_Model", model);
    m_LightingMaterial->GetShader()->SetInt("u_IsLightSource", 1);
    m_LightingMaterial->GetShader()->SetFloat3("u_FlatColor", glm::vec3(1.0f, 1.0f, 0.0f));
    Aether::RenderCommand::DrawIndexed(m_CubeMesh->GetVertexArray());
    m_LightingMaterial->GetShader()->SetInt("u_IsLightSource", 0);
}

void GameLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled) 
    {
        m_EditorCamera.OnEvent(event);
    }
}

void GameLayer::OnImGuiRender()
{
    ImGui::SetNextWindowSize(ImVec2(420, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Reset Camera")) {
                m_EditorCamera.SetDistance(10.0f);
            }
            if (ImGui::MenuItem("Top View")) {
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
        
        // Color Grading (LUT)
        ImGui::Text("Color Grading (LUT)");
        ImGui::SliderFloat("Intensity", &m_LutIntensity, 0.0f, 1.0f);
        
        // Display LUT texture preview
        ImGui::Image((void*)(intptr_t)m_TextureLibrary.Get("LUT")->GetRendererID(), ImVec2(256, 16));
        
        ImGui::Spacing();
        ImGui::Separator();
    }

    ImGui::End();
}