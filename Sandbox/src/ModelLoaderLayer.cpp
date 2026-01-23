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

    // Register shader
    m_ShaderId = Aether::AssetsRegister::Register("Shader_Basic");
    Aether::ShaderLibrary::Load("assets/shaders/Basic.shader", m_ShaderId);

    // Camera UBO
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::UniformBuffer::Create(uboSize, 0);

    // Load initial model
    LoadModelFile(m_ModelPathBuffer);

    AE_CORE_INFO("ModelLoaderLayer initialized successfully!");
}

void ModelLoaderLayer::Detach()
{
    m_Model = ModelFile();
    m_CameraUBO.reset();
}

Aether::Ref<Aether::Material> ModelLoaderLayer::CreateMaterialFromTexture(const unsigned char* data, size_t size, int width, int height, const std::string& name)
{
    // Register texture
    Aether::UUID texId = Aether::AssetsRegister::Register(name);
    Aether::Texture2DLibrary::Load((void*)data, size, texId);
    
    // Create material
    auto material = Aether::CreateRef<Aether::Material>(m_ShaderId);
    material->SetTexture("u_Texture", texId);
    material->SetFloat4("u_Color", glm::vec4(1.0f));
    
    return material;
}

void ModelLoaderLayer::LoadModelFile(const std::string& filepath)
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

    // Reset model
    m_Model = ModelFile();
    m_Model.FilePath = filepath;
    m_Model.Name = filepath.substr(filepath.find_last_of("/\\") + 1);
    
    // Simple layout: Position (3) + TexCoord (2) = 5 floats per vertex
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    std::vector<Aether::SubMesh> submeshes;

    m_Model.BoundsMin = glm::vec3(FLT_MAX);
    m_Model.BoundsMax = glm::vec3(-FLT_MAX);

    // Load textures/materials for each mesh
    std::vector<Aether::Ref<Aether::Material>> materials;
    
    // First, load all materials from the file
    for (uint32_t matIdx = 0; matIdx < scene->mNumMaterials; matIdx++)
    {
        aiMaterial* aiMat = scene->mMaterials[matIdx];
        
        // Try to get texture from material
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texPath;
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
            
            // Check if it's an embedded texture
            const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texPath.C_Str());
            if (embeddedTex)
            {
                size_t size = (embeddedTex->mHeight == 0) ? embeddedTex->mWidth : embeddedTex->mHeight * embeddedTex->mWidth * 4;
                std::string matName = "Material_" + std::to_string(matIdx);
                materials.push_back(CreateMaterialFromTexture(
                    (unsigned char*)embeddedTex->pcData, 
                    size, 
                    embeddedTex->mWidth, 
                    embeddedTex->mHeight,
                    matName
                ));
                continue;
            }
        }
        
        // If no texture, check for embedded textures by index
        if (scene->HasTextures() && matIdx < scene->mNumTextures)
        {
            const aiTexture* texture = scene->mTextures[matIdx];
            size_t size = (texture->mHeight == 0) ? texture->mWidth : texture->mHeight * texture->mWidth * 4;
            std::string matName = "Material_" + std::to_string(matIdx);
            materials.push_back(CreateMaterialFromTexture(
                (unsigned char*)texture->pcData, 
                size, 
                texture->mWidth, 
                texture->mHeight,
                matName
            ));
        }
        else
        {
            // Create default material
            auto defaultMat = Aether::CreateRef<Aether::Material>(m_ShaderId);
            defaultMat->SetFloat4("u_Color", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
            materials.push_back(defaultMat);
        }
    }
    
    // If no materials, try to load first embedded texture
    if (materials.empty() && scene->HasTextures())
    {
        const aiTexture* texture = scene->mTextures[0];
        size_t size = (texture->mHeight == 0) ? texture->mWidth : texture->mHeight * texture->mWidth * 4;
        materials.push_back(CreateMaterialFromTexture(
            (unsigned char*)texture->pcData, 
            size, 
            texture->mWidth, 
            texture->mHeight,
            "DefaultMaterial"
        ));
    }
    
    // If still no materials, create a default one
    if (materials.empty())
    {
        auto defaultMat = Aether::CreateRef<Aether::Material>(m_ShaderId);
        defaultMat->SetFloat4("u_Color", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        materials.push_back(defaultMat);
    }

    // Process all meshes
    for (uint32_t m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];
        
        SubMeshInstance submeshInstance;
        submeshInstance.Data.BaseVertex = (uint32_t)vertices.size() / 5;
        submeshInstance.Data.BaseIndex = (uint32_t)indices.size();
        submeshInstance.Data.VertexCount = mesh->mNumVertices;
        submeshInstance.Data.IndexCount = mesh->mNumFaces * 3;
        submeshInstance.Data.NodeName = mesh->mName.C_Str();
        
        // Assign material (use material index or default to 0)
        uint32_t matIndex = (mesh->mMaterialIndex < materials.size()) ? mesh->mMaterialIndex : 0;
        submeshInstance.Material = materials[matIndex];

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
            m_Model.BoundsMin = glm::min(m_Model.BoundsMin, pos);
            m_Model.BoundsMax = glm::max(m_Model.BoundsMax, pos);

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

        submeshInstance.Data.BoundsMin = subMeshMin;
        submeshInstance.Data.BoundsMax = subMeshMax;

        // Extract indices
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        submeshes.push_back(submeshInstance.Data);
        m_Model.SubMeshes.push_back(submeshInstance);
    }

    // Create simple buffer layout for Basic shader
    Aether::BufferLayout simpleLayout = {
        { "position", Aether::ShaderDataType::Float3 },
        { "texCoord", Aether::ShaderDataType::Float2 }
    };

    // Create mesh
    uint32_t vertexCount = (uint32_t)vertices.size() / 5;
    uint32_t indexCount = (uint32_t)indices.size();
    
    m_Model.Mesh = Aether::CreateRef<Aether::Mesh>(
        vertices.data(), 
        vertexCount, 
        indices.data(), 
        indexCount, 
        simpleLayout,
        submeshes
    );

    m_Model.IsLoaded = true;
    
    AE_CORE_INFO("Model loaded: {0}", filepath);
    AE_CORE_INFO("Vertices: {0}, Indices: {1}, SubMeshes: {2}", 
        vertexCount, indexCount, m_Model.SubMeshes.size());

    // Auto look at model
    LookAtModel();
}

void ModelLoaderLayer::LookAtModel()
{
    if (!m_Model.IsLoaded) return;

    // Calculate model center and size
    glm::vec3 center = (m_Model.BoundsMin + m_Model.BoundsMax) * 0.5f;
    glm::vec3 extents = m_Model.BoundsMax - m_Model.BoundsMin;
    float maxExtent = glm::max(extents.x, glm::max(extents.y, extents.z));

    // Set camera to look at center from a distance
    float distance = maxExtent * 2.5f;
    m_EditorCamera.SetDistance(distance);
    
    AE_CORE_INFO("Camera focused on model. Distance: {0}", distance);
}

void ModelLoaderLayer::RenderSubMesh(ModelFile& model, SubMeshInstance& submesh)
{
    if (!submesh.Visible || !submesh.Material) return;

    submesh.Material->Bind(0);

    // Build transform matrix (model space -> submesh local transform -> world space)
    glm::mat4 submeshTransform = glm::mat4(1.0f);
    submeshTransform = glm::translate(submeshTransform, submesh.Position);
    submeshTransform = glm::rotate(submeshTransform, glm::radians(submesh.Rotation.x), glm::vec3(1, 0, 0));
    submeshTransform = glm::rotate(submeshTransform, glm::radians(submesh.Rotation.y), glm::vec3(0, 1, 0));
    submeshTransform = glm::rotate(submeshTransform, glm::radians(submesh.Rotation.z), glm::vec3(0, 0, 1));
    submeshTransform = glm::scale(submeshTransform, submesh.Scale);

    glm::mat4 projection = m_EditorCamera.GetProjection();
    glm::mat4 view = m_EditorCamera.GetViewMatrix();
    glm::mat4 mvp = projection * view * submeshTransform;
    
    submesh.Material->SetMat4("u_MVP", mvp);
    submesh.Material->UploadMaterial();

    auto vao = model.Mesh->GetVertexArray();

    void* indexOffset = (void*)(submesh.Data.BaseIndex * sizeof(uint32_t));
    Aether::RenderCommand::DrawIndexedBaseVertex(
        vao, 
        submesh.Data.IndexCount, 
        indexOffset, 
        submesh.Data.BaseVertex
    );
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

    // Render all submeshes
    if (m_Model.IsLoaded && m_Model.Mesh)
    {
        for (auto& submesh : m_Model.SubMeshes)
        {
            RenderSubMesh(m_Model, submesh);
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
    ImGui::SetNextWindowSize(ImVec2(450, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("Model Loader", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Reload Model"))
            {
                LoadModelFile(m_ModelPathBuffer);
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

    // Load Model Section
    if (ImGui::CollapsingHeader("Load Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Path", m_ModelPathBuffer, sizeof(m_ModelPathBuffer));
        
        if (ImGui::Button("Load Model", ImVec2(-1, 30)))
        {
            LoadModelFile(m_ModelPathBuffer);
        }
        
        if (ImGui::Button("Look At Model", ImVec2(-1, 30)))
        {
            LookAtModel();
        }
    }

    // Model Info
    if (ImGui::CollapsingHeader("Model Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (m_Model.IsLoaded)
        {
            ImGui::Text("File: %s", m_Model.Name.c_str());
            ImGui::Text("Objects: %d", (int)m_Model.SubMeshes.size());
            ImGui::Text("Total Vertices: %d", m_Model.Mesh->GetVertexCount());
            ImGui::Text("Total Indices: %d", m_Model.Mesh->GetIndexCount());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No model loaded!");
        }
    }

    // Camera Controls
    if (ImGui::CollapsingHeader("Camera"))
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

    // Objects List (SubMeshes)
    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < m_Model.SubMeshes.size(); i++)
        {
            auto& submesh = m_Model.SubMeshes[i];
            ImGui::PushID((int)i);
            
            // Object header with visibility checkbox
            bool isSelected = (int)i == m_SelectedSubMeshIndex;
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;
            
            std::string label = submesh.Data.NodeName.empty() ? 
                "Object " + std::to_string(i) : submesh.Data.NodeName;
            
            bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);
            
            // Select on click
            if (ImGui::IsItemClicked())
            {
                m_SelectedSubMeshIndex = (int)i;
            }
            
            ImGui::SameLine();
            ImGui::Checkbox("##visible", &submesh.Visible);
            
            if (nodeOpen)
            {
                ImGui::Text("Vertices: %d", submesh.Data.VertexCount);
                ImGui::Text("Indices: %d", submesh.Data.IndexCount);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Local Transform:");
                
                // Transform controls for individual object
                ImGui::DragFloat3("Position", &submesh.Position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &submesh.Rotation.x, 1.0f, -180.0f, 180.0f);
                ImGui::DragFloat3("Scale", &submesh.Scale.x, 0.05f, 0.01f, 10.0f);
                
                if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
                {
                    submesh.Position = glm::vec3(0.0f);
                    submesh.Rotation = glm::vec3(0.0f);
                    submesh.Scale = glm::vec3(1.0f);
                }
                
                // Show material info
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Material Info:");
                if (submesh.Material)
                {
                    auto tex = submesh.Material->GetTexture("u_Texture");
                    if (tex)
                    {
                        ImGui::Text("Texture: %d x %d", tex->GetWidth(), tex->GetHeight());
                        
                        float availWidth = ImGui::GetContentRegionAvail().x;
                        float aspect = (float)tex->GetHeight() / (float)tex->GetWidth();
                        ImGui::Image((void*)(uintptr_t)tex->GetRendererID(), 
                                   ImVec2(availWidth * 0.5f, availWidth * 0.5f * aspect));
                    }
                    else
                    {
                        ImGui::Text("No texture");
                    }
                }
                
                ImGui::TreePop();
            }
            
            ImGui::PopID();
        }
    }

    ImGui::End();
}