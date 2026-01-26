#include "LabLayer.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

Aether::UUID id_ShaderGLTF = Aether::AssetsRegister::Register("Shader_GLTF");

LabLayer::LabLayer()
    : Layer("GLTF Loader Lab")
{
    m_Camera = Aether::EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
    m_Camera.SetDistance(5.0f);
}

void LabLayer::Attach()
{
    ImGuiContext* IGContext = Aether::ImGuiLayer::GetContext();
    if (IGContext) ImGui::SetCurrentContext(IGContext);

    // Load GLTF shader
    Aether::ShaderLibrary::Load("assets/shaders/GLTF.shader", id_ShaderGLTF);

    // Camera UBO
    uint32_t uboSize = sizeof(glm::mat4) * 2 + sizeof(glm::vec4);
    m_CameraUBO = Aether::UniformBuffer::Create(uboSize, 0);

    // Load GLB file
    LoadGLBFile("assets/models/robot.glb");

    AE_CORE_INFO("LabLayer initialized successfully!");
}

void LabLayer::Detach()
{
    m_CameraUBO.reset();
    m_LoadedMeshes.clear();
}

void LabLayer::LoadGLBFile(const std::string& filepath)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
    if (result != cgltf_result_success)
    {
        AE_CORE_ERROR("Failed to parse GLB file: {0}", filepath);
        return;
    }

    result = cgltf_load_buffers(&options, data, filepath.c_str());
    if (result != cgltf_result_success)
    {
        AE_CORE_ERROR("Failed to load GLB buffers: {0}", filepath);
        cgltf_free(data);
        return;
    }

    AE_CORE_INFO("Loaded GLB file: {0}", filepath);
    AE_CORE_INFO("  Meshes: {0}", data->meshes_count);
    AE_CORE_INFO("  Materials: {0}", data->materials_count);
    AE_CORE_INFO("  Textures: {0}", data->textures_count);
    AE_CORE_INFO("  Images: {0}", data->images_count);

    // Load textures first
    std::vector<Aether::UUID> textureIDs;
    for (size_t i = 0; i < data->images_count; i++)
    {
        cgltf_image* image = &data->images[i];
        Aether::UUID texID = Aether::AssetsRegister::Register(
            std::string("GLTFTex_") + (image->name ? image->name : std::to_string(i))
        );

        if (image->buffer_view)
        {
            cgltf_buffer_view* view = image->buffer_view;
            const uint8_t* imageData = (const uint8_t*)view->buffer->data + view->offset;
            
            Aether::Texture2DLibrary::Load((void*)imageData, view->size, texID);
            AE_CORE_INFO("  Loaded embedded texture [{0}]: {1}", i, image->name ? image->name : "unnamed");
        }
        else if (image->uri)
        {
            std::string imagePath = "assets/models/" + std::string(image->uri);
            Aether::Texture2DLibrary::Load(imagePath, texID);
            AE_CORE_INFO("  Loaded texture from URI [{0}]: {1}", i, image->uri);
        }
        
        textureIDs.push_back(texID);
    }

    // Load materials
    std::vector<Aether::UUID> materialIDs;
    for (size_t i = 0; i < data->materials_count; i++)
    {
        cgltf_material* mat = &data->materials[i];
        Aether::UUID matID = Aether::AssetsRegister::Register(
            std::string("GLTFMat_") + (mat->name ? mat->name : std::to_string(i))
        );

        auto material = Aether::MaterialLibrary::Load(id_ShaderGLTF, matID);

        // Base color texture
        if (mat->pbr_metallic_roughness.base_color_texture.texture)
        {
            cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
            size_t texIndex = tex->image - data->images;
            if (texIndex < textureIDs.size())
            {
                material->SetTexture("u_AlbedoMap", textureIDs[texIndex]);
            }
        }

        // Base color factor
        glm::vec4 baseColor(
            mat->pbr_metallic_roughness.base_color_factor[0],
            mat->pbr_metallic_roughness.base_color_factor[1],
            mat->pbr_metallic_roughness.base_color_factor[2],
            mat->pbr_metallic_roughness.base_color_factor[3]
        );
        material->SetFloat4("u_AlbedoColor", baseColor);

        // Metallic-Roughness texture
        if (mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            cgltf_texture* tex = mat->pbr_metallic_roughness.metallic_roughness_texture.texture;
            size_t texIndex = tex->image - data->images;
            if (texIndex < textureIDs.size())
            {
                material->SetTexture("u_MetallicRoughnessMap", textureIDs[texIndex]);
            }
        }

        material->SetFloat("u_Metallic", mat->pbr_metallic_roughness.metallic_factor);
        material->SetFloat("u_Roughness", mat->pbr_metallic_roughness.roughness_factor);

        // Normal map
        if (mat->normal_texture.texture)
        {
            cgltf_texture* tex = mat->normal_texture.texture;
            size_t texIndex = tex->image - data->images;
            if (texIndex < textureIDs.size())
            {
                material->SetTexture("u_NormalMap", textureIDs[texIndex]);
                material->SetInt("u_HasNormalMap", 1);
            }
        }
        else
        {
            material->SetInt("u_HasNormalMap", 0);
        }

        materialIDs.push_back(matID);
        AE_CORE_INFO("  Created material [{0}]: {1}", i, mat->name ? mat->name : "unnamed");
    }

    // Load meshes
    for (size_t meshIdx = 0; meshIdx < data->meshes_count; meshIdx++)
    {
        cgltf_mesh* mesh = &data->meshes[meshIdx];
        
        GLTFMeshData meshData;
        meshData.Name = mesh->name ? mesh->name : ("Mesh_" + std::to_string(meshIdx));
        meshData.MeshID = Aether::AssetsRegister::Register(std::string("GLTFMesh_") + meshData.Name);

        AE_CORE_INFO("Processing mesh [{0}]: {1} with {2} primitives", 
            meshIdx, meshData.Name, mesh->primitives_count);

        // Collect all vertex data streams
        std::vector<std::vector<float>> positionData;
        std::vector<std::vector<float>> normalData;
        std::vector<std::vector<float>> tangentData;
        std::vector<std::vector<float>> texCoordData;
        std::vector<std::vector<uint32_t>> indexData;
        std::vector<Aether::SubMesh> submeshes;

        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;

        for (size_t primIdx = 0; primIdx < mesh->primitives_count; primIdx++)
        {
            cgltf_primitive* prim = &mesh->primitives[primIdx];
            
            Aether::SubMesh submesh;
            submesh.BaseVertex = totalVertices;
            submesh.BaseIndex = totalIndices;
            submesh.NodeName = meshData.Name + "_Prim" + std::to_string(primIdx);
            
            // Get material
            if (prim->material)
            {
                size_t matIndex = prim->material - data->materials;
                if (matIndex < materialIDs.size())
                {
                    submesh.MaterialID = materialIDs[matIndex];
                    meshData.SubmeshMaterialIDs.push_back(materialIDs[matIndex]);
                }
            }

            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<float> tangents;
            std::vector<float> texCoords;

            // Extract attributes
            for (size_t attrIdx = 0; attrIdx < prim->attributes_count; attrIdx++)
            {
                cgltf_attribute* attr = &prim->attributes[attrIdx];
                cgltf_accessor* accessor = attr->data;
                
                if (attr->type == cgltf_attribute_type_position)
                {
                    submesh.VertexCount = (uint32_t)accessor->count;
                    positions.resize(accessor->count * 3);
                    
                    for (size_t v = 0; v < accessor->count; v++)
                    {
                        float pos[3];
                        cgltf_accessor_read_float(accessor, v, pos, 3);
                        positions[v * 3 + 0] = pos[0];
                        positions[v * 3 + 1] = pos[1];
                        positions[v * 3 + 2] = pos[2];

                        // Update bounds
                        if (v == 0 && primIdx == 0)
                        {
                            submesh.BoundsMin = glm::vec3(pos[0], pos[1], pos[2]);
                            submesh.BoundsMax = submesh.BoundsMin;
                        }
                        else
                        {
                            submesh.BoundsMin = glm::min(submesh.BoundsMin, glm::vec3(pos[0], pos[1], pos[2]));
                            submesh.BoundsMax = glm::max(submesh.BoundsMax, glm::vec3(pos[0], pos[1], pos[2]));
                        }
                    }
                }
                else if (attr->type == cgltf_attribute_type_normal)
                {
                    normals.resize(accessor->count * 3);
                    for (size_t v = 0; v < accessor->count; v++)
                    {
                        float norm[3];
                        cgltf_accessor_read_float(accessor, v, norm, 3);
                        normals[v * 3 + 0] = norm[0];
                        normals[v * 3 + 1] = norm[1];
                        normals[v * 3 + 2] = norm[2];
                    }
                }
                else if (attr->type == cgltf_attribute_type_tangent)
                {
                    tangents.resize(accessor->count * 4);
                    for (size_t v = 0; v < accessor->count; v++)
                    {
                        float tan[4];
                        cgltf_accessor_read_float(accessor, v, tan, 4);
                        tangents[v * 4 + 0] = tan[0];
                        tangents[v * 4 + 1] = tan[1];
                        tangents[v * 4 + 2] = tan[2];
                        tangents[v * 4 + 3] = tan[3];
                    }
                }
                else if (attr->type == cgltf_attribute_type_texcoord)
                {
                    texCoords.resize(accessor->count * 2);
                    for (size_t v = 0; v < accessor->count; v++)
                    {
                        float uv[2];
                        cgltf_accessor_read_float(accessor, v, uv, 2);
                        texCoords[v * 2 + 0] = uv[0];
                        texCoords[v * 2 + 1] = uv[1];
                    }
                }
            }

            // Generate missing data if needed
            if (normals.empty() && !positions.empty())
            {
                normals.resize(positions.size(), 0.0f);
                for (size_t i = 0; i < positions.size() / 3; i++)
                {
                    normals[i * 3 + 1] = 1.0f; // Default up
                }
            }

            if (tangents.empty() && !positions.empty())
            {
                tangents.resize((positions.size() / 3) * 4, 0.0f);
                for (size_t i = 0; i < positions.size() / 3; i++)
                {
                    tangents[i * 4 + 0] = 1.0f; // Default right
                    tangents[i * 4 + 3] = 1.0f; // Handedness
                }
            }

            if (texCoords.empty() && !positions.empty())
            {
                texCoords.resize((positions.size() / 3) * 2, 0.0f);
            }

            // Extract indices
            std::vector<uint32_t> indices;
            if (prim->indices)
            {
                cgltf_accessor* accessor = prim->indices;
                submesh.IndexCount = (uint32_t)accessor->count;
                indices.resize(accessor->count);
                
                for (size_t i = 0; i < accessor->count; i++)
                {
                    indices[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
                }
            }

            positionData.push_back(std::move(positions));
            normalData.push_back(std::move(normals));
            tangentData.push_back(std::move(tangents));
            texCoordData.push_back(std::move(texCoords));
            indexData.push_back(std::move(indices));
            
            totalVertices += submesh.VertexCount;
            totalIndices += submesh.IndexCount;
            submeshes.push_back(submesh);
        }

        // Merge all streams into continuous buffers
        std::vector<float> mergedPositions;
        std::vector<float> mergedNormals;
        std::vector<float> mergedTangents;
        std::vector<float> mergedTexCoords;
        std::vector<uint32_t> mergedIndices;

        for (size_t i = 0; i < positionData.size(); i++)
        {
            mergedPositions.insert(mergedPositions.end(), positionData[i].begin(), positionData[i].end());
            mergedNormals.insert(mergedNormals.end(), normalData[i].begin(), normalData[i].end());
            mergedTangents.insert(mergedTangents.end(), tangentData[i].begin(), tangentData[i].end());
            mergedTexCoords.insert(mergedTexCoords.end(), texCoordData[i].begin(), texCoordData[i].end());
            mergedIndices.insert(mergedIndices.end(), indexData[i].begin(), indexData[i].end());
        }

        // Create vertex streams (SoA format)
        std::vector<Aether::VertexStream> streams;
        
        streams.push_back(Aether::VertexStream{
            mergedPositions.data(),
            totalVertices,
            Aether::BufferLayout{{ "a_Position", Aether::ShaderDataType::Float3 }}
        });
        
        streams.push_back(Aether::VertexStream{
            mergedNormals.data(),
            totalVertices,
            Aether::BufferLayout{{ "a_Normal", Aether::ShaderDataType::Float3 }}
        });
        
        streams.push_back(Aether::VertexStream{
            mergedTangents.data(),
            totalVertices,
            Aether::BufferLayout{{ "a_Tangent", Aether::ShaderDataType::Float4 }}
        });
        
        streams.push_back(Aether::VertexStream{
            mergedTexCoords.data(),
            totalVertices,
            Aether::BufferLayout{{ "a_TexCoord", Aether::ShaderDataType::Float2 }}
        });

        // Create mesh
        Aether::MeshSpec meshSpec;
        meshSpec.Streams = streams;
        meshSpec.IndexData = mergedIndices.data();
        meshSpec.IndexCount = totalIndices;
        meshSpec.Submeshes = submeshes;

        Aether::MeshLibrary::Load(meshSpec, meshData.MeshID);
        m_LoadedMeshes.push_back(meshData);

        AE_CORE_INFO("  Created mesh with {0} vertices, {1} indices, {2} submeshes", 
            totalVertices, totalIndices, submeshes.size());
    }

    cgltf_free(data);
    AE_CORE_INFO("GLB loading complete! Total meshes: {0}", m_LoadedMeshes.size());
}

void LabLayer::Update(Aether::Timestep ts)
{
    if (m_AutoRotate) m_ModelRotation.y += ts * m_RotationSpeed;

    m_Camera.Update(ts);

    // Render
    auto& window = Aether::Application::Get().GetWindow();
    Aether::RenderCommand::SetClearColor({ 0.2f, 0.2f, 0.25f, 1.0f });
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());

    // Set viewport size THEN get matrices
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    glm::mat4 projection = m_Camera.GetProjection();
    glm::mat4 view = m_Camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;
    glm::vec3 camPos = m_Camera.GetPosition();

    m_CameraUBO->SetData(glm::value_ptr(viewProjection), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    m_CameraUBO->SetData(glm::value_ptr(camPos), sizeof(glm::vec3), 2 * sizeof(glm::mat4));

    RenderScene();
}

void LabLayer::RenderScene()
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_ModelPosition);
    transform = glm::rotate(transform, glm::radians(m_ModelRotation.x), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRotation.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRotation.z), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, m_ModelScale);

    for (const auto& meshData : m_LoadedMeshes)
    {
        auto mesh = Aether::MeshLibrary::Get(meshData.MeshID);
        const auto& submeshes = mesh->GetSubMeshes();

        for (size_t i = 0; i < submeshes.size(); i++)
        {
            const auto& submesh = submeshes[i];
            
            // Get material for this submesh
            Aether::UUID matID = (i < meshData.SubmeshMaterialIDs.size()) 
                ? meshData.SubmeshMaterialIDs[i] 
                : Aether::UUID(0);

            if (matID != Aether::UUID(0) && Aether::MaterialLibrary::Exists(matID))
            {
                auto material = Aether::MaterialLibrary::Get(matID);
                material->Bind(0);
                material->SetMat4("u_Model", transform);
                material->UploadMaterial();

                // Draw submesh using base vertex
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
    ImGui::Begin("GLTF Model Viewer");

    ImGui::Text("Loaded Meshes: %d", (int)m_LoadedMeshes.size());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Right Mouse: Rotate (orbit)");
        ImGui::TextWrapped("Middle Mouse: Pan view");
        ImGui::TextWrapped("Scroll Wheel: Zoom in/out");
        ImGui::TextWrapped("WASD: Move horizontally");
        ImGui::TextWrapped("Q/E: Move down/up");
        ImGui::Spacing();
        
        glm::vec3 pos = m_Camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Distance: %.1f", m_Camera.GetDistance());
        
        if (ImGui::Button("Reset Camera"))
        {
            m_Camera.SetDistance(5.0f);
        }
        ImGui::Separator();
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat3("Position", &m_ModelPosition.x, 0.1f);
        ImGui::DragFloat3("Rotation", &m_ModelRotation.x, 1.0f);
        ImGui::DragFloat3("Scale", &m_ModelScale.x, 0.1f, 0.01f, 10.0f);
        
        if (ImGui::Button("Reset Transform"))
        {
            m_ModelPosition = glm::vec3(0.0f);
            m_ModelRotation = glm::vec3(0.0f);
            m_ModelScale = glm::vec3(1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Animation"))
    {
        ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
        if (m_AutoRotate)
        {
            ImGui::SliderFloat("Rotation Speed", &m_RotationSpeed, -5.0f, 5.0f);
        }
    }

    if (ImGui::CollapsingHeader("Mesh Info"))
    {
        for (size_t i = 0; i < m_LoadedMeshes.size(); i++)
        {
            const auto& meshData = m_LoadedMeshes[i];
            if (ImGui::TreeNode((void*)i, "Mesh: %s", meshData.Name.c_str()))
            {
                auto mesh = Aether::MeshLibrary::Get(meshData.MeshID);
                ImGui::Text("Vertices: %u", mesh->GetVertexCount());
                ImGui::Text("Indices: %u", mesh->GetIndexCount());
                ImGui::Text("Submeshes: %zu", mesh->GetSubMeshes().size());
                
                const auto& bounds = mesh->GetBoundsCenter();
                ImGui::Text("Center: (%.2f, %.2f, %.2f)", bounds.x, bounds.y, bounds.z);
                
                // Show submeshes with materials
                ImGui::Separator();
                const auto& submeshes = mesh->GetSubMeshes();
                for (size_t j = 0; j < submeshes.size(); j++)
                {
                    if (ImGui::TreeNode((void*)(i * 10000 + j), "Submesh %zu", j))
                    {
                        const auto& submesh = submeshes[j];
                        ImGui::Text("Vertices: %u", submesh.VertexCount);
                        ImGui::Text("Indices: %u", submesh.IndexCount);
                        
                        if (j < meshData.SubmeshMaterialIDs.size())
                        {
                            Aether::UUID matID = meshData.SubmeshMaterialIDs[j];
                            if (Aether::MaterialLibrary::Exists(matID))
                            {
                                auto material = Aether::MaterialLibrary::Get(matID);
                                ImGui::Text("Material ID: %llu", (uint64_t)matID);
                                
                                ImGui::Spacing();
                                
                                // Albedo texture
                                auto albedoTex = material->GetTexture("u_AlbedoMap");
                                if (albedoTex)
                                {
                                    ImGui::Text("Albedo:");
                                    ImGui::Image(
                                        (void*)(intptr_t)albedoTex->GetRendererID(),
                                        ImVec2(128, 128),
                                        ImVec2(0, 1), ImVec2(1, 0)
                                    );
                                }
                                
                                // Metallic-Roughness texture
                                auto mrTex = material->GetTexture("u_MetallicRoughnessMap");
                                if (mrTex)
                                {
                                    ImGui::Text("Metallic-Roughness:");
                                    ImGui::Image(
                                        (void*)(intptr_t)mrTex->GetRendererID(),
                                        ImVec2(128, 128),
                                        ImVec2(0, 1), ImVec2(1, 0)
                                    );
                                }
                                
                                // Normal map
                                auto normalTex = material->GetTexture("u_NormalMap");
                                if (normalTex)
                                {
                                    ImGui::Text("Normal Map:");
                                    ImGui::Image(
                                        (void*)(intptr_t)normalTex->GetRendererID(),
                                        ImVec2(128, 128),
                                        ImVec2(0, 1), ImVec2(1, 0)
                                    );
                                }
                            }
                        }
                        
                        ImGui::TreePop();
                    }
                }
                
                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
}