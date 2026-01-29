#include "LabLayer.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/AssetsRegister.h"
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

    auto shader = Aether::ShaderLibrary::Load("assets/shaders/PBR.shader", id_ShaderPBR);
    m_CameraUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 2 + sizeof(glm::vec4), 0);
    m_BoneUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 100, 1);
    shader->SetUBOSlot("Camera", 0);
    shader->SetUBOSlot("Bones", 1);
    
    LoadModelAsync("assets/models/human.glb");
    LoadModelAsync("assets/models/robot.glb");
}

void LabLayer::LoadModelAsync(const std::string& path)
{
    Aether::JobSystem::SubmitJob([this, path]() {
        AE_CORE_INFO("Worker thread: Parsing {0}", path);
        
        auto modelData = Aether::SceneLoader::Parsing(path);
        
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
    m_BoneUBO.reset();
    m_MeshNames.clear();
    m_AnimatorNames.clear();
    m_AnimationNames.clear();
    m_AnimationBindings.clear();
}

void LabLayer::Update(Aether::Timestep ts)
{
    {
        std::lock_guard<std::mutex> lock(m_ParseMutex);
        while (!m_CompletedParses.empty())
        {
            auto modelData = std::move(m_CompletedParses.front());
            m_CompletedParses.pop();
            
            AE_CORE_INFO("Main thread: Uploading to GPU...");

            std::string baseName = modelData.FilePath;
            auto meshIDs = Aether::SceneLoader::UploadModel(modelData, id_ShaderPBR);
            for (size_t i = 0; i < meshIDs.size(); i++)
            {
                std::string meshName = baseName + "_Mesh_" + std::to_string(i);
                Aether::AssetsRegister::Register(meshName, meshIDs[i]);
                m_MeshNames.push_back(meshName);
            }

            for (size_t i = 0; i < modelData.Skeletons.size(); i++)
            {
                const auto& skeleton = modelData.Skeletons[i];
                AE_CORE_INFO("Found skeleton with {0} bones", skeleton.parentIndices.size());
                
                std::string animatorName = "Animator_" + std::to_string(i);
                Aether::UUID animatorID = Aether::AssetsRegister::Register(animatorName);
                
                auto animator = Aether::AnimatorLibrary::Load(Aether::AnimatorSpec{
                    skeleton.parentIndices,
                    skeleton.inverseBindMatrices,
                    skeleton.localBindPose
                }, animatorID);
                
                m_AnimatorNames.push_back(animatorName);
            }

            for (const auto& animInfo : modelData.Animations)
            {
                AE_CORE_INFO("Found animation: {0} (duration: {1}s)", 
                    animInfo.clip.name, animInfo.clip.duration);
                
                std::string animationName = animInfo.clip.name;
                Aether::UUID animationID = Aether::AssetsRegister::Register(animationName);
                
                auto animation = Aether::AnimationLibrary::Load(animInfo.clip, animationID);
                
                m_AnimationNames.push_back(animationName);
            }
            
            AE_CORE_INFO("Main thread: Loaded {0} meshes, {1} animators, {2} animations", 
                m_MeshNames.size(), m_AnimatorNames.size(), m_AnimationNames.size());
        }
    }

    for (auto& binding : m_AnimationBindings)
    {
        if (!binding.IsActive) continue;
        if (binding.AnimatorName.empty()) continue;

        Aether::UUID animatorID = Aether::AssetsRegister::Get(binding.AnimatorName);
        auto animator = Aether::AnimatorLibrary::Get(animatorID);
        if (!animator) continue;

        animator->Update(ts);
        
        const auto& boneMatrices = animator->GetFinalMatrices();
        m_BoneUBO->SetData(boneMatrices.data(), 
                          boneMatrices.size() * sizeof(glm::mat4), 0);
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
    
    RenderScene();
}

void LabLayer::RenderScene()
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_ModelPos);
    transform = glm::rotate(transform, glm::radians(m_ModelRot.x), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRot.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(m_ModelRot.z), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, m_ModelScale);

    for (const auto& meshName : m_MeshNames)
    {
        Aether::UUID meshID = Aether::AssetsRegister::Get(meshName);
        auto mesh = Aether::MeshLibrary::Get(meshID);
        if (!mesh) continue;
        
        const auto& submeshes = mesh->GetSubMeshes();
        
        bool hasActiveBinding = false;
        for (const auto& binding : m_AnimationBindings)
        {
            if (binding.IsActive && binding.MeshName == meshName)
            {
                hasActiveBinding = true;
                break;
            }
        }
        
        for (const auto& submesh : submeshes)
        {
            if (submesh.MaterialID && Aether::MaterialLibrary::Exists(submesh.MaterialID))
            {
                auto material = Aether::MaterialLibrary::Get(submesh.MaterialID);
                material->Bind(0);
                material->SetMat4("u_Model", transform);

                if (hasActiveBinding)
                {
                    material->SetInt("u_HasAnimation", 1);
                }
                else
                {
                    material->SetInt("u_HasAnimation", 0);
                }

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
    
    ImGui::Text("Meshes: %d", (int)m_MeshNames.size());
    ImGui::Text("Animators: %d", (int)m_AnimatorNames.size());
    ImGui::Text("Animations: %d", (int)m_AnimationNames.size());
    
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Animation Bindings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Add Binding"))
        {
            AnimationBinding newBinding;
            newBinding.IsActive = false;
            m_AnimationBindings.push_back(newBinding);
        }
        
        ImGui::Separator();
        
        for (int i = 0; i < m_AnimationBindings.size(); i++)
        {
            ImGui::PushID(i);
            
            auto& binding = m_AnimationBindings[i];
            
            ImGui::Text("Binding %d", i);
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                m_AnimationBindings.erase(m_AnimationBindings.begin() + i);
                ImGui::PopID();
                break;
            }
            
            ImGui::Checkbox("Active", &binding.IsActive);
            
            std::string currentMeshName = binding.MeshName.empty() ? "None" : binding.MeshName;
            if (ImGui::BeginCombo("Mesh", currentMeshName.c_str()))
            {
                for (const auto& meshName : m_MeshNames)
                {
                    bool isSelected = (binding.MeshName == meshName);
                    
                    if (ImGui::Selectable(meshName.c_str(), isSelected))
                    {
                        binding.MeshName = meshName;
                    }
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            
            std::string currentAnimatorName = binding.AnimatorName.empty() ? "None" : binding.AnimatorName;
            if (ImGui::BeginCombo("Animator", currentAnimatorName.c_str()))
            {
                for (const auto& animatorName : m_AnimatorNames)
                {
                    bool isSelected = (binding.AnimatorName == animatorName);
                    
                    if (ImGui::Selectable(animatorName.c_str(), isSelected))
                    {
                        binding.AnimatorName = animatorName;
                    }
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            
            std::string currentAnimationName = binding.AnimationName.empty() ? "None" : binding.AnimationName;
            if (ImGui::BeginCombo("Animation", currentAnimationName.c_str()))
            {
                for (const auto& animationName : m_AnimationNames)
                {
                    bool isSelected = (binding.AnimationName == animationName);
                    
                    if (ImGui::Selectable(animationName.c_str(), isSelected))
                    {
                        binding.AnimationName = animationName;
                        
                        if (binding.IsActive && !binding.AnimatorName.empty())
                        {
                            Aether::UUID animatorID = Aether::AssetsRegister::Get(binding.AnimatorName);
                            auto animator = Aether::AnimatorLibrary::Get(animatorID);
                            
                            Aether::UUID animationID = Aether::AssetsRegister::Get(animationName);
                            auto animation = Aether::AnimationLibrary::Get(animationID);
                            
                            if (animator && animation)
                            {
                                animator->Play(&animation->Clip, true);
                            }
                        }
                    }
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            
            if (binding.IsActive && !binding.AnimatorName.empty())
            {
                Aether::UUID animatorID = Aether::AssetsRegister::Get(binding.AnimatorName);
                auto animator = Aether::AnimatorLibrary::Get(animatorID);
                
                if (animator)
                {
                    bool isPlaying = animator->IsPlaying();
                    if (ImGui::Checkbox("Playing", &isPlaying))
                    {
                        if (isPlaying)
                        {
                            if (!binding.AnimationName.empty())
                            {
                                Aether::UUID animationID = Aether::AssetsRegister::Get(binding.AnimationName);
                                auto animation = Aether::AnimationLibrary::Get(animationID);
                                if (animation)
                                {
                                    animator->Play(&animation->Clip, true);
                                }
                            }
                        }
                        else
                        {
                            animator->Pause();
                        }
                    }
                    
                    float speed = animator->GetPlaybackSpeed();
                    if (ImGui::SliderFloat("Speed", &speed, 0.0f, 2.0f))
                    {
                        animator->SetPlaybackSpeed(speed);
                    }
                    
                    if (!binding.AnimationName.empty())
                    {
                        Aether::UUID animationID = Aether::AssetsRegister::Get(binding.AnimationName);
                        auto animation = Aether::AnimationLibrary::Get(animationID);
                        if (animation)
                        {
                            float currentTime = animator->GetCurrentTime();
                            float duration = animation->Clip.duration;
                            ImGui::Text("Time: %.2f / %.2f", currentTime, duration);
                            ImGui::ProgressBar(currentTime / duration);
                        }
                    }
                    
                    if (ImGui::Button("Reset"))
                    {
                        animator->Stop();
                        if (!binding.AnimationName.empty())
                        {
                            Aether::UUID animationID = Aether::AssetsRegister::Get(binding.AnimationName);
                            auto animation = Aether::AnimationLibrary::Get(animationID);
                            if (animation)
                            {
                                animator->Play(&animation->Clip, true);
                            }
                        }
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    
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
    
    if (ImGui::CollapsingHeader("Auto Rotation"))
    {
        ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
        if (m_AutoRotate)
        {
            ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);
        }
    }
    
    ImGui::End();
}