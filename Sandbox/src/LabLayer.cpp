#include "LabLayer.h"
#include "Aether/Core/JobSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

Aether::UUID id_ShaderPBR = Aether::AssetsRegister::Register("Shader_PBR");

LabLayer::LabLayer() 
    : Layer("Lab Layer")
    , m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(5.0f);
}

void LabLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    Aether::ShaderLibrary::Load("assets/shaders/PBR.shader", id_ShaderPBR);
    m_CameraUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 2 + sizeof(glm::vec4), 0);
    
    // Load model async
    LoadModelAsync("assets/models/robot.glb");
}

void LabLayer::LoadModelAsync(const std::string& path)
{
    m_IsLoading = true;
    
    // Submit parsing job to worker thread
    Aether::JobSystem::SubmitJob([this, path]() {
        AE_CORE_INFO("Worker thread: Parsing {0}", path);
        
        // Parse on worker thread (no OpenGL calls)
        auto modelData = Aether::ModelLoader::Parsing(path);
        
        // Push result to queue (thread-safe)
        {
            std::lock_guard<std::mutex> lock(m_ParseMutex);
            m_CompletedParses.push(std::move(modelData));
        }
        
        AE_CORE_INFO("Worker thread: Parsing complete for {0}", path);
    });
}

void LabLayer::Detach()
{
    m_CameraUBO.reset();
    m_MeshIDs.clear();
}

void LabLayer::Update(Aether::Timestep ts)
{
    // Check for completed parses on main thread
    if (m_IsLoading)
    {
        std::lock_guard<std::mutex> lock(m_ParseMutex);
        if (!m_CompletedParses.empty())
        {
            // Upload to GPU on main thread
            auto modelData = std::move(m_CompletedParses.front());
            m_CompletedParses.pop();
            
            AE_CORE_INFO("Main thread: Uploading to GPU...");
            m_MeshIDs = Aether::ModelLoader::UploadModel(modelData, id_ShaderPBR);
            m_IsLoading = false;
            
            AE_CORE_INFO("Main thread: Loaded {0} meshes", m_MeshIDs.size());
        }
    }
    
    if (m_AutoRotate) m_ModelRot.y += ts * m_RotationSpeed;
    
    m_Camera.Update(ts);
    
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    
    glm::mat4 viewProj = m_Camera.GetProjection() * m_Camera.GetViewMatrix();
    glm::mat4 view = m_Camera.GetViewMatrix();
    glm::vec3 camPos = m_Camera.GetPosition();
    
    m_CameraUBO->SetData(glm::value_ptr(viewProj), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(camPos), sizeof(glm::vec3), 2 * sizeof(glm::mat4));
    
    Aether::RenderCommand::SetClearColor({0.2f, 0.2f, 0.25f, 1.0f});
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());
    
    if (!m_IsLoading)
    {
        RenderScene();
    }
}

void LabLayer::RenderScene()
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_ModelPos);
    transform = glm::rotate(transform, glm::radians(m_ModelRot.x), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRot.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRot.z), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, m_ModelScale);

    for (auto meshID : m_MeshIDs)
    {
        auto mesh = Aether::MeshLibrary::Get(meshID);
        const auto& submeshes = mesh->GetSubMeshes();
        
        for (const auto& submesh : submeshes)
        {
            if (submesh.MaterialID && Aether::MaterialLibrary::Exists(submesh.MaterialID))
            {
                auto material = Aether::MaterialLibrary::Get(submesh.MaterialID);
                material->Bind(0);
                material->SetMat4("u_Model", transform);
                material->UploadMaterial();
                
                void* indexOffset = (void*)(submesh.BaseIndex * sizeof(uint32_t));
                Aether::RenderCommand::DrawIndexedBaseVertex(
                    mesh->GetVertexArray(),
                    submesh.IndexCount,
                    indexOffset,
                    submesh.BaseVertex
                );
            }
        }
    }
}

void LabLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled) m_Camera.OnEvent(event);
}

void LabLayer::OnImGuiRender()
{
    ImGui::Begin("Model Viewer");
    
    if (m_IsLoading)
    {
        ImGui::Text("Loading model...");
    }
    else
    {
        ImGui::Text("Meshes: %d", (int)m_MeshIDs.size());
    }
    
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 pos = m_Camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Distance: %.1f", m_Camera.GetDistance());
        
        if (ImGui::Button("Reset Camera"))
        {
            m_Camera.SetDistance(5.0f);
        }
    }
    
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Position", &m_ModelPos.x, 0.1f);
        ImGui::DragFloat3("Rotation", &m_ModelRot.x, 1.0f);
        ImGui::DragFloat3("Scale", &m_ModelScale.x, 0.1f, 0.01f, 10.0f);
        
        if (ImGui::Button("Reset Transform"))
        {
            m_ModelPos = glm::vec3(0.0f);
            m_ModelRot = glm::vec3(0.0f);
            m_ModelScale = glm::vec3(1.0f);
        }
    }
    
    if (ImGui::CollapsingHeader("Animation"))
    {
        ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
        if (m_AutoRotate)
        {
            ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);
        }
    }
    
    ImGui::End();
}