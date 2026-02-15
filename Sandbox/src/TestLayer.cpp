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
    // Initialize systems
    Aether::MaterialLibrary::Init();
    Aether::MeshLibrary::Init();
    Aether::Texture2DLibrary::Init();
    Aether::ShaderLibrary::Init();

    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    auto modelData = Aether::Importer::Import("assets/models/human.glb");
    auto result = Aether::Importer::Upload(modelData);
    m_LoadedMeshIDs.insert(m_LoadedMeshIDs.end(), result.meshIDs.begin(), result.meshIDs.end());

    AE_CORE_INFO("TestLayer initialized!");
}

void TestLayer::Detach()
{
    Aether::MaterialLibrary::Shutdown();
    Aether::MeshLibrary::Shutdown();
    Aether::Texture2DLibrary::Shutdown();
    Aether::ShaderLibrary::Shutdown();
}

void TestLayer::Update(Aether::Timestep ts)
{
    if (m_AutoRotate)
        m_Rotation += ts * m_RotationSpeed;

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    // Use new Renderer API
    Aether::Renderer::BeginScene(m_Camera);

    // Add rotating cube
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_CubePos);
    transform = glm::rotate(transform, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    for (auto id : m_LoadedMeshIDs)
    {
        Aether::Renderer::AddMesh(id, Aether::UUID(0), transform);
    }
    

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
    ImGui::DragFloat3("Cube Position", glm::value_ptr(m_CubePos), 0.1f);

    ImGui::End();
}