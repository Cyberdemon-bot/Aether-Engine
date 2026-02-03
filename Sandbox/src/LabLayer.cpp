#include "LabLayer.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/AssetsRegister.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

    auto shader = Aether::Shader::Create("assets/shaders/PBR.shader");
    Aether::ShaderLibrary::Add(shader, id_ShaderPBR);
    m_CameraUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 2 + sizeof(glm::vec4), 0);
    m_BoneUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 100, 1);
    shader->SetUBOSlot("Camera", 0);
    shader->SetUBOSlot("Bones", 1);

    Aether::ConsoleLayer::RegisterCommand("load", AE_BIND_CONSOLE_FN(LoadModelAsync));
}

void LabLayer::LoadModelAsync(const std::vector<std::string>& args)
{
    std::string path = args[0];
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
    m_Meshes.clear();
    m_SkeletalAnimator.clear();
    m_SkeletalAnim.clear();
    m_AnimationBindings.clear();
}

void LabLayer::PrintSceneLog(const Aether::SceneResult& result)
{
    Aether::ConsoleLayer::PushLog("========================================");
    std::string meshInfo = "Meshes Loaded: " + std::to_string(result.meshIDs.size());
    Aether::ConsoleLayer::PushLog(meshInfo);
    for (size_t i = 0; i < result.meshIDs.size(); i++)
    {
        Aether::UUID meshID = result.meshIDs[i];
        std::string name = Aether::AssetsRegister::Get(meshID);
        std::string line = "   -> [" + std::to_string(i) + "] " + name;
        Aether::ConsoleLayer::PushLog(line);
    }
    if (!result.clipIDs.empty())
    {
        std::string animInfo = "Animations Loaded: " + std::to_string(result.clipIDs.size());
        Aether::ConsoleLayer::PushLog(animInfo);
    }
    if (!result.skeletonIDs.empty())
    {
        std::string skelInfo = "Skeletons Loaded: " + std::to_string(result.skeletonIDs.size());
        Aether::ConsoleLayer::PushLog(skelInfo);
    }
    Aether::ConsoleLayer::PushLog("========================================");
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

            auto result = Aether::SceneLoader::UploadScene(modelData, id_ShaderPBR);

            m_Meshes.insert(m_Meshes.end(), result.meshIDs.begin(), result.meshIDs.end());
            m_SkeletalAnimator.insert(m_SkeletalAnimator.end(), result.skeletonIDs.begin(), result.skeletonIDs.end());
            m_SkeletalAnim.insert(m_SkeletalAnim.end(), result.clipIDs.begin(), result.clipIDs.end());
            for (size_t i = 0; i < result.meshIDs.size(); i++)  m_Transforms.push_back(Transform()); 
            PrintSceneLog(result);
        }
    }

    for (auto& binding : m_AnimationBindings)
    {
        if (!binding.IsActive) continue;
        if (uint64_t(binding.skeleton) == 0) continue;

        auto animator = Aether::SkeletalAnimatorLibrary::Get(binding.skeleton);
        if (!animator) continue;

        animator->Update(ts);
        
        const auto& boneMatrices = animator->GetFinalMatrices();
        m_BoneUBO->SetData(boneMatrices.data(), 
                          boneMatrices.size() * sizeof(glm::mat4), 0);
    }
    
    if (!ImGui::GetIO().WantCaptureKeyboard) m_Camera.Update(ts);

    if (m_AutoRotate)
    {
        for (int i = 0; i < m_Transforms.size(); i++)
        {
            m_Transforms[i].m_ModelRot.y += ts * m_RotationSpeed;
        }
    }
    
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
    for (int i = 0; i < m_Meshes.size(); i++)
    {
        Aether::UUID meshID = m_Meshes[i];
        glm::vec3 pos = m_Transforms[i].m_ModelPos;
        glm::vec3 rot = m_Transforms[i].m_ModelRot;
        glm::vec3 scale = m_Transforms[i].m_ModelScale;
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), pos);
        glm::quat rotationQuat = glm::quat(glm::radians(rot)); 
        glm::mat4 rotationMatrix = glm::toMat4(rotationQuat);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 transform = translationMatrix * rotationMatrix * scaleMatrix;

        auto mesh = Aether::MeshLibrary::Get(meshID);
        if (!mesh) continue;
        
        const auto& submeshes = mesh->GetSubMeshes();
        
        bool hasActiveBinding = false;
        for (const auto& binding : m_AnimationBindings)
        {
            if (binding.IsActive && uint64_t(binding.mesh) == meshID)
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
    // Model Viewer Window
    ImGuiWindowFlags viewer_flags = ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Model Viewer", nullptr, viewer_flags))
    {
        ImGui::Text("Meshes: %d", (int)m_Meshes.size());
        ImGui::Text("Animators: %d", (int)m_SkeletalAnimator.size());
        ImGui::Text("Animations: %d", (int)m_SkeletalAnim.size());
        
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
                
                std::string currentMeshName = uint64_t(binding.mesh) == 0 ? "None" : Aether::AssetsRegister::Get(binding.mesh);
                if (ImGui::BeginCombo("Mesh", currentMeshName.c_str()))
                {
                    for (const auto& mesh : m_Meshes)
                    {
                        bool isSelected = (uint64_t(binding.mesh) == uint64_t(mesh));
                        
                        if (ImGui::Selectable(Aether::AssetsRegister::Get(mesh).c_str(), isSelected))
                        {
                            binding.mesh = mesh;
                        }
                        
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                std::string currentAnimatorName = uint64_t(binding.skeleton) == 0 ? "None" : Aether::AssetsRegister::Get(binding.skeleton);
                if (ImGui::BeginCombo("Animator", currentAnimatorName.c_str()))
                {
                    for (const auto& skeleton : m_SkeletalAnimator)
                    {
                        bool isSelected = (uint64_t(binding.skeleton) == skeleton);
                        
                        if (ImGui::Selectable(Aether::AssetsRegister::Get(skeleton).c_str(), isSelected))
                        {
                            binding.skeleton = skeleton;
                        }
                        
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                std::string currentAnimationName = uint64_t(binding.anim) == 0 ? "None" : Aether::AssetsRegister::Get(binding.anim);
                if (ImGui::BeginCombo("Animation", currentAnimationName.c_str()))
                {
                    for (const auto& anim : m_SkeletalAnim)
                    {
                        bool isSelected = (uint64_t(binding.anim) == anim);
                        
                        if (ImGui::Selectable(Aether::AssetsRegister::Get(anim).c_str(), isSelected))
                        {
                            binding.anim = anim;
                            
                            if (binding.IsActive && uint64_t(binding.anim))
                            {
                                auto animator = Aether::SkeletalAnimatorLibrary::Get(binding.skeleton);
                                auto animation = Aether::ClipLibrary::Get(binding.anim);
                                
                                if (animator && animation)
                                {
                                    animator->Play(animation.get(), true);
                                }
                            }
                        }
                        
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                if (binding.IsActive && uint64_t(binding.skeleton))
                {
                    auto animator = Aether::SkeletalAnimatorLibrary::Get(binding.skeleton);
                    
                    if (animator)
                    {
                        bool isPlaying = animator->IsPlaying();
                        if (ImGui::Checkbox("Playing", &isPlaying))
                        {
                            if (isPlaying)
                            {
                                if (uint64_t(binding.anim))
                                {
                                    auto animation = Aether::ClipLibrary::Get(binding.anim);
                                    if (animation)
                                    {
                                        animator->Play(animation.get(), true);
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
                        
                        if (uint64_t(binding.anim))
                        {
                            auto animation = Aether::ClipLibrary::Get(binding.anim);
                            if (animation)
                            {
                                float currentTime = animator->GetCurrentTime();
                                float duration = animation->Durations;
                                ImGui::Text("Time: %.2f / %.2f", currentTime, duration);
                                ImGui::ProgressBar(currentTime / duration);
                            }
                        }
                        
                        if (ImGui::Button("Reset"))
                        {
                            animator->Stop();
                            if (uint64_t(binding.anim))
                            {
                                auto animation = Aether::ClipLibrary::Get(binding.anim);
                                if (animation)
                                {
                                    animator->Play(animation.get(), true);
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
            std::string previewName = "None";
            if (m_SelectedMeshIndex >= 0 && m_SelectedMeshIndex < m_Meshes.size())
                previewName = Aether::AssetsRegister::Get(m_Meshes[m_SelectedMeshIndex]);

            if (ImGui::BeginCombo("Select Mesh", previewName.c_str()))
            {
                for (int i = 0; i < m_Meshes.size(); i++)
                {
                    bool isSelected = (m_SelectedMeshIndex == i);
                    std::string name = std::to_string(i) + ": " + Aether::AssetsRegister::Get(m_Meshes[i]);

                    if (ImGui::Selectable(name.c_str(), isSelected))
                    {
                        m_SelectedMeshIndex = i; 
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Separator();
            if (m_SelectedMeshIndex >= 0 && m_SelectedMeshIndex < m_Transforms.size())
            {
                auto& transform = m_Transforms[m_SelectedMeshIndex]; 
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Editing: %s", previewName.c_str());
                ImGui::DragFloat3("Position", glm::value_ptr(transform.m_ModelPos), 0.1f);
                ImGui::DragFloat3("Rotation", glm::value_ptr(transform.m_ModelRot), 1.0f);
                ImGui::DragFloat3("Scale",    glm::value_ptr(transform.m_ModelScale), 0.05f, 0.01f, 100.0f);

                if (ImGui::Button("Reset Transform"))
                {
                    transform.m_ModelPos = glm::vec3(0.0f);
                    transform.m_ModelRot = glm::vec3(0.0f);
                    transform.m_ModelScale = glm::vec3(1.0f);
                }
            }
            else ImGui::TextDisabled("Please select a mesh above to edit transform.");
        }
        
        if (ImGui::CollapsingHeader("Auto Rotation"))
        {
            ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
            if (m_AutoRotate)
            {
                ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);
            }
        }
    }
    ImGui::End();
}