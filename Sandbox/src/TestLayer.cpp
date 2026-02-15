#include "TestLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

TestLayer::TestLayer()
    : Layer("Test Layer")
    , m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(5.0f);
}

void TestLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    auto modelData = Aether::Importer::Import("assets/models/human.glb");
    auto result = Aether::Importer::Upload(modelData);
    m_LoadedMeshIDs.insert(m_LoadedMeshIDs.end(), result.meshIDs.begin(), result.meshIDs.end());

    Aether::FramebufferSpecification shadowFbSpec;
    shadowFbSpec.Width = 2048;
    shadowFbSpec.Height = 2048;
    shadowFbSpec.Attachments = { Aether::FramebufferTextureFormat::DEPTH24STENCIL8 };

    Aether::RenderPass shadowPass;
    shadowPass.TargetFBO = Aether::FrameBuffer::Create(shadowFbSpec);
    shadowPass.Shader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    shadowPass.Shader->Bind();
    shadowPass.Shader->SetUBOSlot("Bones", 1);
    shadowPass.Shader->SetUBOSlot("Lights", 2);
    shadowPass.ClearDepth = true;
    shadowPass.ClearColor = false;
    shadowPass.ColorTexIdx = -1;
    shadowPass.DepthTexIdx = -1;
    shadowPass.OnScreen = false;

    Aether::FramebufferSpecification sceneFbSpec;
    sceneFbSpec.Width = Aether::Application::Get().GetWindow().GetWidth();
    sceneFbSpec.Height = Aether::Application::Get().GetWindow().GetHeight();
    sceneFbSpec.Attachments = { 
        Aether::FramebufferTextureFormat::RGBA8,          
        Aether::FramebufferTextureFormat::DEPTH24STENCIL8  
    };

    Aether::RenderPass mainPass;
    mainPass.TargetFBO = Aether::FrameBuffer::Create(sceneFbSpec);
    mainPass.Shader = Aether::Shader::Create("assets/shaders/Standard.shader");
    mainPass.Shader->Bind();
    mainPass.Shader->SetUBOSlot("Camera", 0);
    mainPass.Shader->SetUBOSlot("Bones", 1);
    mainPass.Shader->SetUBOSlot("Lights", 2);
    mainPass.ClearColor = true;
    mainPass.ClearDepth = true;
    mainPass.UsingSkybox = true;
    mainPass.ClearValue = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    mainPass.ColorTexIdx = -1;
    mainPass.DepthTexIdx = 0;
    mainPass.OnScreen = true;
    mainPass.m_LutIntensity = 0.0f;

    pipeline.push_back(shadowPass);
    pipeline.push_back(mainPass);
    Aether::Renderer::SetPipeline(pipeline);

    Aether::LightParam spotLight;
    spotLight.type = Aether::LightType::Spot;
    spotLight.position = glm::vec3(0, 5, 0);
    spotLight.direction = glm::vec3(0, -1, 0);
    spotLight.color = glm::vec3(1, 1, 1);
    spotLight.intensity = 3.0f;
    spotLight.range = 20.0f;
    spotLight.innerCone = glm::cos(glm::radians(12.5f));
    spotLight.outerCone = glm::cos(glm::radians(25.0f));
    spotLight.castShadows = true;
    m_Lights.push_back(spotLight);

    AE_CORE_INFO("TestLayer initialized!");
}

void TestLayer::Detach()
{
    
}

void TestLayer::Update(Aether::Timestep ts)
{
    if (m_AutoRotate)
        m_Rotation += ts * m_RotationSpeed;

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_CubePos);
    transform = glm::rotate(transform, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));

    glm::vec3 modelCenter = glm::vec3(transform[3]);
    m_Lights[0].direction = glm::normalize(modelCenter - m_Lights[0].position);

    Aether::Renderer::BeginScene(m_Camera, m_Lights);
    for (auto id : m_LoadedMeshIDs) Aether::Renderer::DrawMesh(id, Aether::UUID(0), transform);
    Aether::Renderer::EndScene();
}

void TestLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled)
        m_Camera.OnEvent(event);
}

void TestLayer::OnImGuiRender()
{
    ImGui::Begin("Test Layer");

    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", 
        m_Camera.GetPosition().x, 
        m_Camera.GetPosition().y, 
        m_Camera.GetPosition().z);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);

    ImGui::Separator();
    ImGui::DragFloat3("Model Position", glm::value_ptr(m_CubePos), 0.1f);

    ImGui::Separator();
    ImGui::Text("Spotlight");
    ImGui::DragFloat3("Light Position", glm::value_ptr(m_Lights[0].position), 0.1f);
    ImGui::ColorEdit3("Light Color", glm::value_ptr(m_Lights[0].color));
    ImGui::SliderFloat("Intensity", &m_Lights[0].intensity, 0.0f, 10.0f);
    ImGui::SliderFloat("Range", &m_Lights[0].range, 1.0f, 50.0f);
    
    float innerDegrees = glm::degrees(glm::acos(m_Lights[0].innerCone));
    float outerDegrees = glm::degrees(glm::acos(m_Lights[0].outerCone));
    
    if (ImGui::SliderFloat("Inner Cone", &innerDegrees, 1.0f, 89.0f))
        m_Lights[0].innerCone = glm::cos(glm::radians(innerDegrees));
    
    if (ImGui::SliderFloat("Outer Cone", &outerDegrees, innerDegrees, 90.0f))
        m_Lights[0].outerCone = glm::cos(glm::radians(outerDegrees));
    
    ImGui::Checkbox("Cast Shadows", &m_Lights[0].castShadows);

    ImGui::End();
}