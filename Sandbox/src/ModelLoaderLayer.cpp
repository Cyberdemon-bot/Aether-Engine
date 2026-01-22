#include "ModelLoaderLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

ModelLoaderLayer::ModelLoaderLayer()
    : Layer("Model Loader")
{
    m_EditorCamera = Aether::EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
}

void ModelLoaderLayer::Attach()
{
    // ImGui context
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    // Register assets
    Aether::UUID id_ShaderBasic = Aether::AssetsRegister::Register("Shader_Basic");
    Aether::UUID id_TexWood = Aether::AssetsRegister::Register("Tex_Wood");

    // Load shader and texture
    Aether::ShaderLibrary::Load("assets/shaders/Basic.shader", id_ShaderBasic);
    Aether::Texture2DLibrary::Load("assets/textures/wood.jpg", id_TexWood);

    // Create material
    m_Material = Aether::CreateRef<Aether::Material>(id_ShaderBasic);
    m_Material->SetTexture("u_Texture", id_TexWood);
    m_Material->SetFloat4("u_Color", glm::vec4(1.0f));

    // Camera UBO
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::UniformBuffer::Create(uboSize, 0);

    // Load model
    LoadModel(m_ModelPath);

    AE_CORE_INFO("ModelLoaderLayer initialized successfully!");
}

void ModelLoaderLayer::Detach()
{
    m_ModelMesh.reset();
    m_Material.reset();
    m_CameraUBO.reset();
}

void ModelLoaderLayer::LoadModel(const std::string& filepath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_FlipUVs | 
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        AE_CORE_ERROR("Assimp Error: {0}", importer.GetErrorString());
        return;
    }

    // Simple layout: Position (3) + TexCoord (2) = 5 floats per vertex
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    m_SubMeshes.clear();

    m_ModelBoundsMin = glm::vec3(FLT_MAX);
    m_ModelBoundsMax = glm::vec3(-FLT_MAX);

    // Process all meshes
    for (uint32_t m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];
        
        Aether::SubMesh submesh;
        submesh.BaseVertex = (uint32_t)vertices.size() / 5; // 5 floats per vertex
        submesh.BaseIndex = (uint32_t)indices.size();
        submesh.VertexCount = mesh->mNumVertices;
        submesh.IndexCount = mesh->mNumFaces * 3;
        submesh.NodeName = mesh->mName.C_Str();

        glm::vec3 subMeshMin(FLT_MAX);
        glm::vec3 subMeshMax(-FLT_MAX);

        // Extract vertices
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            // Position
            glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);

            // Update bounds
            subMeshMin = glm::min(subMeshMin, pos);
            subMeshMax = glm::max(subMeshMax, pos);
            m_ModelBoundsMin = glm::min(m_ModelBoundsMin, pos);
            m_ModelBoundsMax = glm::max(m_ModelBoundsMax, pos);

            // TexCoord
            if (mesh->mTextureCoords[0])
            {
                vertices.push_back(mesh->mTextureCoords[0][i].x);
                vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
            else
            {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }

        submesh.BoundsMin = subMeshMin;
        submesh.BoundsMax = subMeshMax;

        // Extract indices
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        m_SubMeshes.push_back(submesh);
    }

    // Create simple buffer layout for Basic shader
    Aether::BufferLayout simpleLayout = {
        { "position", Aether::ShaderDataType::Float3 },
        { "texCoord", Aether::ShaderDataType::Float2 }
    };

    // Create mesh
    uint32_t vertexCount = (uint32_t)vertices.size() / 5;
    uint32_t indexCount = (uint32_t)indices.size();
    
    m_ModelMesh = Aether::CreateRef<Aether::Mesh>(
        vertices.data(), 
        vertexCount, 
        indices.data(), 
        indexCount, 
        simpleLayout,
        m_SubMeshes
    );

    m_ModelLoaded = true;
    
    AE_CORE_INFO("Model loaded: {0}", filepath);
    AE_CORE_INFO("Vertices: {0}, Indices: {1}, SubMeshes: {2}", 
        vertexCount, indexCount, m_SubMeshes.size());
    AE_CORE_INFO("Bounds: Min({0}, {1}, {2}) Max({3}, {4}, {5})",
        m_ModelBoundsMin.x, m_ModelBoundsMin.y, m_ModelBoundsMin.z,
        m_ModelBoundsMax.x, m_ModelBoundsMax.y, m_ModelBoundsMax.z);

    // Auto look at model on first load
    LookAtModel();
}

void ModelLoaderLayer::LookAtModel()
{
    if (!m_ModelLoaded) return;

    // Calculate model center and size
    glm::vec3 center = (m_ModelBoundsMin + m_ModelBoundsMax) * 0.5f;
    glm::vec3 extents = m_ModelBoundsMax - m_ModelBoundsMin;
    float maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));

    // Set camera to look at center from a distance
    float distance = maxExtent * 2.5f; // Adjust multiplier for framing
    m_EditorCamera.SetDistance(distance);
    
    AE_CORE_INFO("Camera focused on model. Distance: {0}", distance);
}

void ModelLoaderLayer::Update(Aether::Timestep ts)
{
    m_EditorCamera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    uint32_t width = window.GetFramebufferWidth();
    uint32_t height = window.GetFramebufferHeight();

    m_EditorCamera.SetViewportSize((float)width, (float)height);

    // Update Camera UBO
    glm::mat4 projection = m_EditorCamera.GetProjection();
    glm::mat4 view = m_EditorCamera.GetViewMatrix();
    glm::vec3 camPos = m_EditorCamera.GetPosition();

    m_CameraUBO->SetData(glm::value_ptr(projection), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(camPos), sizeof(glm::vec3), 2 * sizeof(glm::mat4));

    // Clear
    Aether::RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.25f, 1.0f });
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, width, height);

    // Render model
    if (m_ModelLoaded && m_ModelMesh)
    {
        m_Material->Bind(0);

        // Build model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_ModelPosition);
        model = glm::rotate(model, glm::radians(m_ModelRotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(m_ModelRotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(m_ModelRotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, m_ModelScale);

        glm::mat4 mvp = projection * view * model;
        m_Material->SetMat4("u_MVP", mvp);
        m_Material->UploadMaterial();

        auto vao = m_ModelMesh->GetVertexArray();

        // Draw each submesh
        for (const auto& submesh : m_SubMeshes)
        {
            void* indexOffset = (void*)(submesh.BaseIndex * sizeof(uint32_t));
            Aether::RenderCommand::DrawIndexedBaseVertex(
                vao, 
                submesh.IndexCount, 
                indexOffset, 
                submesh.BaseVertex
            );
        }
    }
}

void ModelLoaderLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled)
    {
        m_EditorCamera.OnEvent(event);
    }
}

void ModelLoaderLayer::OnImGuiRender()
{
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Model Loader", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Reload Model"))
            {
                LoadModel(m_ModelPath);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Performance
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
    ImGui::Text("FPS: %.1f (%.2fms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Model Info
    if (ImGui::CollapsingHeader("Model Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (m_ModelLoaded)
        {
            ImGui::Text("Path: %s", m_ModelPath.c_str());
            ImGui::Text("SubMeshes: %d", (int)m_SubMeshes.size());
            ImGui::Text("Vertices: %d", m_ModelMesh->GetVertexCount());
            ImGui::Text("Indices: %d", m_ModelMesh->GetIndexCount());
            
            ImGui::Spacing();
            ImGui::Text("Bounds:");
            ImGui::Text("  Min: (%.2f, %.2f, %.2f)", 
                m_ModelBoundsMin.x, m_ModelBoundsMin.y, m_ModelBoundsMin.z);
            ImGui::Text("  Max: (%.2f, %.2f, %.2f)", 
                m_ModelBoundsMax.x, m_ModelBoundsMax.y, m_ModelBoundsMax.z);

            if (ImGui::Button("Look At Model", ImVec2(-1, 30)))
            {
                LookAtModel();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No model loaded!");
        }
    }

    // Camera Controls
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
        ImGui::TextWrapped("Controls:");
        ImGui::PopStyleColor();
        
        ImGui::Indent(10.0f);
        ImGui::BulletText("Right Mouse: Rotate");
        ImGui::BulletText("Middle Mouse: Pan");
        ImGui::BulletText("Scroll: Zoom");
        ImGui::BulletText("WASD + Q/E: Move");
        ImGui::Unindent(10.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        
        glm::vec3 pos = m_EditorCamera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Distance: %.1f", m_EditorCamera.GetDistance());
    }

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Position", &m_ModelPosition.x, 0.1f);
        ImGui::DragFloat3("Rotation", &m_ModelRotation.x, 1.0f, -180.0f, 180.0f);
        ImGui::DragFloat3("Scale", &m_ModelScale.x, 0.05f, 0.01f, 10.0f);

        if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
        {
            m_ModelPosition = glm::vec3(0.0f);
            m_ModelRotation = glm::vec3(0.0f);
            m_ModelScale = glm::vec3(1.0f);
        }
    }

    // SubMeshes List
    if (ImGui::CollapsingHeader("SubMeshes"))
    {
        for (size_t i = 0; i < m_SubMeshes.size(); i++)
        {
            const auto& sm = m_SubMeshes[i];
            ImGui::PushID((int)i);
            
            if (ImGui::TreeNode("##submesh", "SubMesh %d: %s", (int)i, sm.NodeName.c_str()))
            {
                ImGui::Text("Vertices: %d", sm.VertexCount);
                ImGui::Text("Indices: %d", sm.IndexCount);
                ImGui::Text("Base Vertex: %d", sm.BaseVertex);
                ImGui::Text("Base Index: %d", sm.BaseIndex);
                ImGui::TreePop();
            }
            
            ImGui::PopID();
        }
    }

    ImGui::End();
}