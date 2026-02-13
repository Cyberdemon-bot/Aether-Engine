#include "PBRDemoLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Register IDs like DemoLayer does
Aether::UUID id_ShaderStandard = Aether::AssetsRegister::Register("Shader_Standard");
Aether::UUID id_MaterialStandard = Aether::AssetsRegister::Register("Material_Standard");
Aether::UUID id_PBRCubeMesh = Aether::AssetsRegister::Register("Mesh_PBRCube");
Aether::UUID id_Tex = Aether::AssetsRegister::Register("Tex_Wood");
Aether::UUID id_TexDefaultMR = Aether::AssetsRegister::Register("Tex_DefaultMR");
Aether::UUID id_TexDefaultNormal = Aether::AssetsRegister::Register("Tex_DefaultNormal");

PBRDemoLayer::PBRDemoLayer()
    : Layer("PBR Demo Layer")
    , m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(10.0f);
}

void PBRDemoLayer::Attach()
{
    // Initialize renderer
    Aether::Renderer::Init();
    Aether::MaterialLibrary::Init();
    Aether::MeshLibrary::Init();
    Aether::Texture2DLibrary::Init();
    Aether::ShaderLibrary::Init();

    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    // Load wood texture
    auto texWood = Aether::Texture2D::Create("assets/textures/wood.jpg");
    Aether::Texture2DLibrary::Add(texWood, id_Tex);

    // Create default 1x1 textures
    Aether::TextureSpec texSpec;
    texSpec.Width = 1;
    texSpec.Height = 1;
    texSpec.Format = Aether::ImageFormat::RGBA8;
    texSpec.GenerateMips = false;

    // Default metallic-roughness: Green=0.8 (roughness), Blue=0.0 (non-metallic)
    uint32_t mrPixel = 0xFF0000CC; // ABGR: A=255, B=0 (metallic), G=204 (roughness ~0.8), R=0
    auto texMR = Aether::Texture2D::Create(texSpec);
    texMR->SetData(&mrPixel, sizeof(uint32_t));
    Aether::Texture2DLibrary::Add(texMR, id_TexDefaultMR);

    // Flat normal map: RGB = (128, 128, 255) pointing up in tangent space
    uint32_t normalPixel = 0xFFFF8080; // ABGR: A=255, B=255, G=128, R=128
    auto texNormal = Aether::Texture2D::Create(texSpec);
    texNormal->SetData(&normalPixel, sizeof(uint32_t));
    Aether::Texture2DLibrary::Add(texNormal, id_TexDefaultNormal);

    // Load shader
    auto shader = Aether::Shader::Create("assets/shaders/Standard.shader");
    Aether::ShaderLibrary::Add(shader, id_ShaderStandard);
    shader->SetUBOSlot("Camera", 0);
    shader->SetUBOSlot("Bones", 1);

    // Create material with all textures set
    auto material = Aether::Material::Create(id_ShaderStandard);
    material->SetTexture("u_AlbedoMap", id_Tex);
    material->SetTexture("u_MetallicRoughnessMap", id_TexDefaultMR);
    material->SetTexture("u_NormalMap", id_TexDefaultNormal);
    material->SetFloat4("u_AlbedoColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // White tint
    material->SetFloat("u_Metallic", 1.0f);    // Multiplier (1.0 = use texture value)
    material->SetFloat("u_Roughness", 1.0f);   // Multiplier (1.0 = use texture value)
    material->SetInt("u_HasNormalMap", 0);     // Disable normal mapping for now
    Aether::MaterialLibrary::Add(material, id_MaterialStandard);

    // Create cube mesh - simplified, matching DemoLayer style
    float vertices[] = {
        // Pos                Normal              Tangent                  UV      Joints      Weights
        -0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,0,1,  0,0,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,0,1,  1,0,  0,0,0,0,  0,0,0,0,
         0.5f, 0.5f,-0.5f,  0,0,-1,  1,0,0,1,  1,1,  0,0,0,0,  0,0,0,0,
        -0.5f, 0.5f,-0.5f,  0,0,-1,  1,0,0,1,  0,1,  0,0,0,0,  0,0,0,0,
        
        -0.5f,-0.5f, 0.5f,  0,0,1,   1,0,0,1,  0,0,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f, 0.5f,  0,0,1,   1,0,0,1,  1,0,  0,0,0,0,  0,0,0,0,
         0.5f, 0.5f, 0.5f,  0,0,1,   1,0,0,1,  1,1,  0,0,0,0,  0,0,0,0,
        -0.5f, 0.5f, 0.5f,  0,0,1,   1,0,0,1,  0,1,  0,0,0,0,  0,0,0,0,
        
        -0.5f, 0.5f, 0.5f,  -1,0,0,  0,0,1,1,  1,0,  0,0,0,0,  0,0,0,0,
        -0.5f, 0.5f,-0.5f,  -1,0,0,  0,0,1,1,  1,1,  0,0,0,0,  0,0,0,0,
        -0.5f,-0.5f,-0.5f,  -1,0,0,  0,0,1,1,  0,1,  0,0,0,0,  0,0,0,0,
        -0.5f,-0.5f, 0.5f,  -1,0,0,  0,0,1,1,  0,0,  0,0,0,0,  0,0,0,0,
        
         0.5f, 0.5f, 0.5f,  1,0,0,   0,0,-1,1, 1,0,  0,0,0,0,  0,0,0,0,
         0.5f, 0.5f,-0.5f,  1,0,0,   0,0,-1,1, 1,1,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f,-0.5f,  1,0,0,   0,0,-1,1, 0,1,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f, 0.5f,  1,0,0,   0,0,-1,1, 0,0,  0,0,0,0,  0,0,0,0,
        
        -0.5f,-0.5f,-0.5f,  0,-1,0,  1,0,0,1,  0,1,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f,-0.5f,  0,-1,0,  1,0,0,1,  1,1,  0,0,0,0,  0,0,0,0,
         0.5f,-0.5f, 0.5f,  0,-1,0,  1,0,0,1,  1,0,  0,0,0,0,  0,0,0,0,
        -0.5f,-0.5f, 0.5f,  0,-1,0,  1,0,0,1,  0,0,  0,0,0,0,  0,0,0,0,
        
        -0.5f, 0.5f,-0.5f,  0,1,0,   1,0,0,1,  0,1,  0,0,0,0,  0,0,0,0,
         0.5f, 0.5f,-0.5f,  0,1,0,   1,0,0,1,  1,1,  0,0,0,0,  0,0,0,0,
         0.5f, 0.5f, 0.5f,  0,1,0,   1,0,0,1,  1,0,  0,0,0,0,  0,0,0,0,
        -0.5f, 0.5f, 0.5f,  0,1,0,   1,0,0,1,  0,0,  0,0,0,0,  0,0,0,0
    };

    uint32_t indices[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        8,9,10, 10,11,8,
        12,13,14, 14,15,12,
        16,17,18, 18,19,16,
        20,21,22, 22,23,20
    };

    // Create submesh with material
    Aether::SubMesh submesh;
    submesh.MaterialID = id_MaterialStandard;
    submesh.VertexCount = 24;
    submesh.IndexCount = 36;

    auto mesh = Aether::Mesh::Create(Aether::MeshSpec{
        {Aether::VertexStream{vertices, 24, Aether::MeshLayout::PBRSkinned()}},
        indices,
        36,
        {submesh}
    });
    Aether::MeshLibrary::Add(mesh, id_PBRCubeMesh);

    AE_CORE_INFO("PBRDemoLayer initialized!");
}

void PBRDemoLayer::Detach()
{
    Aether::Renderer::Shutdown();
    Aether::MaterialLibrary::Shutdown();
    Aether::MeshLibrary::Shutdown();
    Aether::Texture2DLibrary::Shutdown();
    Aether::ShaderLibrary::Shutdown();
}

void PBRDemoLayer::Update(Aether::Timestep ts)
{
    if (m_AutoRotate)
        m_Rotation += ts * m_RotationSpeed;

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    // Clear
    Aether::RenderCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());

    // Render with new Renderer API
    Aether::Renderer::BeginScene(m_Camera);

    // Cube A
    glm::mat4 transformA = glm::translate(glm::mat4(1.0f), m_CubeAPos);
    transformA = glm::rotate(transformA, m_Rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    Aether::Renderer::AddMesh(id_PBRCubeMesh, Aether::UUID(0), transformA);

    // Cube B
    glm::mat4 transformB = glm::translate(glm::mat4(1.0f), m_CubeBPos);
    transformB = glm::rotate(transformB, m_Rotation * 0.7f, glm::vec3(1.0f, 0.5f, 0.0f));
    Aether::Renderer::AddMesh(id_PBRCubeMesh, Aether::UUID(0), transformB);

    Aether::Renderer::EndScene();
}

void PBRDemoLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled)
        m_Camera.OnEvent(event);
}

void PBRDemoLayer::OnImGuiRender()
{
    ImGui::Begin("PBR Demo");

    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", 
        m_Camera.GetPosition().x, 
        m_Camera.GetPosition().y, 
        m_Camera.GetPosition().z);

    ImGui::Separator();
    ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
    if (m_AutoRotate)
        ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);

    ImGui::Separator();
    ImGui::DragFloat3("Cube A Position", glm::value_ptr(m_CubeAPos), 0.1f);
    ImGui::DragFloat3("Cube B Position", glm::value_ptr(m_CubeBPos), 0.1f);

    ImGui::End();
}