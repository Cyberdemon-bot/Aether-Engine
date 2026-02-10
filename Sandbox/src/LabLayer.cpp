#include "LabLayer.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/AssetsRegister.h"
#include "Aether/Animation/AnimationManager.h"
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
    Aether::AnimationManager::Init();
    Aether::MaterialLibrary::Init();
    Aether::MeshLibrary::Init();
    Aether::Texture2DLibrary::Init();
    Aether::ShaderLibrary::Init();

    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);
    
    m_Shader = Aether::Shader::Create("assets/shaders/PBR.shader");
    Aether::ShaderLibrary::Add(m_Shader, id_ShaderPBR);
    m_CameraUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 2, 0);
    m_BoneUBO = Aether::UniformBuffer::Create(sizeof(glm::mat4) * 100, 1);
    m_Shader->SetUBOSlot("Camera", 0);
    m_Shader->SetUBOSlot("Bones", 1);

    Aether::ConsoleLayer::RegisterCommand("load", AE_BIND_CONSOLE_FN(LoadModelAsync));

    AE_CORE_INFO("Initialize done!");
}

void LabLayer::LoadModelAsync(const std::vector<std::string>& args)
{
    std::string path = args[0];
    Aether::JobSystem::SubmitJob([this, path]() {
        AE_CORE_INFO("Worker thread: Parsing {0}", path);
        auto modelData = Aether::Importer::Import(path);
        
        {
            std::lock_guard<std::mutex> lock(m_ParseMutex);
            m_CompletedParses.push(std::move(modelData));
        }
        
        AE_CORE_INFO("Worker thread: Parsing complete for {0}", path);
    });
}

void LabLayer::Detach()
{
    m_Shader.reset();
    m_CameraUBO.reset();
    m_BoneUBO.reset();
    m_Meshes.clear();
    m_Skeletons.clear();
    m_Animators.clear();
    m_Clips.clear();
    m_SkeletonToAnimator.clear();
    m_MeshToSkeleton.clear();

    Aether::AnimationManager::Shutdown();
    Aether::MaterialLibrary::Shutdown();
    Aether::MeshLibrary::Shutdown();
    Aether::Texture2DLibrary::Shutdown();
    Aether::ShaderLibrary::Shutdown();
}

void LabLayer::PrintSceneLog(const Aether::RegisteredScene& result)
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
    if (!result.skelIDs.empty())
    {
        std::string skelInfo = "Skeletons Loaded: " + std::to_string(result.skelIDs.size());
        Aether::ConsoleLayer::PushLog(skelInfo);
    }
    Aether::ConsoleLayer::PushLog("========================================");
}

void LabLayer::Update(Aether::Timestep ts)
{
    auto skelSystem = Aether::AnimationManager::GetSystem<Aether::SkeletalAnimationSystem>(Aether::AnimationType::Skeletal);
    if (!skelSystem)
    {
        AE_CORE_ERROR("LabLayer: Skeletal animation system not initialized!");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_ParseMutex);
        while (!m_CompletedParses.empty())
        {
            auto modelData = std::move(m_CompletedParses.front());
            m_CompletedParses.pop();
            
            AE_CORE_INFO("Main thread: Uploading to GPU...");

            auto result = Aether::Importer::Upload(modelData, id_ShaderPBR);

            m_Meshes.insert(m_Meshes.end(), result.meshIDs.begin(), result.meshIDs.end());
            m_Skeletons.insert(m_Skeletons.end(), result.skelIDs.begin(), result.skelIDs.end());
            m_Clips.insert(m_Clips.end(), result.clipIDs.begin(), result.clipIDs.end());

            for (size_t i = 0; i < result.skelIDs.size(); i++)
            {
                Aether::UUID skeletonID = result.skelIDs[i];
                Aether::UUID animatorID = Aether::UUID();
                skelSystem->CreateAnimator(animatorID, skeletonID);
                
                m_Animators.push_back(animatorID);
                m_SkeletonToAnimator[skeletonID] = animatorID;

                AE_CORE_INFO("Created animator {0} for skeleton {1}", 
                    (uint64_t)animatorID, (uint64_t)skeletonID);
            }

            for (size_t i = 0; i < result.meshIDs.size(); i++) m_Transforms.push_back(Transform());

            PrintSceneLog(result);
        }
    }

    Aether::AnimationManager::Update(ts);
    for (const auto& [skeletonID, animatorID] : m_SkeletonToAnimator)
    {
        skelSystem->RequestMatrices(animatorID);
    }
    skelSystem->ProcessRequests();
    
    if (!ImGui::GetIO().WantCaptureKeyboard) m_Camera.Update(ts);

    if (m_AutoRotate)
        for (int i = 0; i < m_Transforms.size(); i++)
            m_Transforms[i].m_ModelRot.y += ts * m_RotationSpeed;
    
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    
    glm::mat4 viewProj = m_Camera.GetProjection() * m_Camera.GetViewMatrix();
    glm::mat4 view = m_Camera.GetViewMatrix();
    
    m_CameraUBO->SetData(glm::value_ptr(viewProj), sizeof(glm::mat4), 0);
    m_CameraUBO->SetData(glm::value_ptr(view), sizeof(glm::mat4), sizeof(glm::mat4));
    
    Aether::RenderCommand::SetClearColor({0.2f, 0.2f, 0.25f, 1.0f});
    Aether::RenderCommand::Clear();
    Aether::RenderCommand::SetViewport(0, 0, window.GetFramebufferWidth(), window.GetFramebufferHeight());
    
    RenderScene();
}

void LabLayer::RenderScene()
{
    m_Shader->Bind();
    
    auto skelSystem = Aether::AnimationManager::GetSystem<Aether::SkeletalAnimationSystem>(Aether::AnimationType::Skeletal);
    
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
        
        bool hasAnimation = m_MeshToSkeleton.find(meshID) != m_MeshToSkeleton.end();
        
        if (hasAnimation && skelSystem)
        {
            auto meshIt = m_MeshToSkeleton.find(meshID);
            if (meshIt != m_MeshToSkeleton.end())
            {
                Aether::UUID skeletonID = meshIt->second;
                auto skelIt = m_SkeletonToAnimator.find(skeletonID);
                if (skelIt != m_SkeletonToAnimator.end())
                {
                    Aether::UUID animatorID = skelIt->second;
                    const auto& boneMatrices = skelSystem->GetMatrices(animatorID);
                    if (!boneMatrices.empty())
                    {
                        m_BoneUBO->SetData(boneMatrices.data(), 
                                          boneMatrices.size() * sizeof(glm::mat4), 0);
                    }
                }
            }
        }
        
        m_Shader->SetMat4("u_Model", transform);
        m_Shader->SetInt("u_HasAnimation", hasAnimation ? 1 : 0);
        
        for (const auto& submesh : submeshes)
        {
            if (submesh.MaterialID && Aether::MaterialLibrary::Exists(submesh.MaterialID))
            {
                auto material = Aether::MaterialLibrary::Get(submesh.MaterialID);
                material->Bind(0);
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
    auto skelSystem = Aether::AnimationManager::GetSystem<Aether::SkeletalAnimationSystem>(Aether::AnimationType::Skeletal);

    // Model Viewer Window
    ImGuiWindowFlags viewer_flags = ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Model Viewer", nullptr, viewer_flags))
    {
        ImGui::Text("Meshes: %d", (int)m_Meshes.size());
        ImGui::Text("Skeletons: %d", (int)m_Skeletons.size());
        ImGui::Text("Clips: %d", (int)m_Clips.size());
        
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Animation Control", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static Aether::UUID selectedMesh = Aether::UUID(0);
            static Aether::UUID selectedSkeleton = Aether::UUID(0);
            static Aether::UUID selectedClip = Aether::UUID(0);
            
            std::string meshName = uint64_t(selectedMesh) == 0 ? "Select Mesh" : Aether::AssetsRegister::Get(selectedMesh);
            if (ImGui::BeginCombo("Mesh", meshName.c_str()))
            {
                for (const auto& meshID : m_Meshes)
                {
                    ImGui::PushID((uint64_t)meshID);
                    
                    bool isSelected = (uint64_t(selectedMesh) == uint64_t(meshID));
                    std::string name = Aether::AssetsRegister::Get(meshID);
                    
                    if (ImGui::Selectable(name.c_str(), isSelected))
                        selectedMesh = meshID;
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                    
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            
            std::string skelName = uint64_t(selectedSkeleton) == 0 ? "Select Skeleton" : Aether::AssetsRegister::Get(selectedSkeleton);
            if (ImGui::BeginCombo("Skeleton", skelName.c_str()))
            {
                for (const auto& skeletonID : m_Skeletons)
                {
                    ImGui::PushID((uint64_t)skeletonID);
                    
                    bool isSelected = (uint64_t(selectedSkeleton) == uint64_t(skeletonID));
                    std::string name = Aether::AssetsRegister::Get(skeletonID);
                    
                    if (ImGui::Selectable(name.c_str(), isSelected))
                        selectedSkeleton = skeletonID;
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                    
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            
            std::string clipName = uint64_t(selectedClip) == 0 ? "Select Clip" : Aether::AssetsRegister::Get(selectedClip);
            if (ImGui::BeginCombo("Clip", clipName.c_str()))
            {
                for (const auto& clipID : m_Clips)
                {
                    ImGui::PushID((uint64_t)clipID);
                    
                    bool isSelected = (uint64_t(selectedClip) == uint64_t(clipID));
                    std::string name = Aether::AssetsRegister::Get(clipID);
                    
                    if (ImGui::Selectable(name.c_str(), isSelected))
                        selectedClip = clipID;
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                    
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            
            if (ImGui::Button("Bind & Play"))
            {
                if (uint64_t(selectedMesh) != 0 && uint64_t(selectedSkeleton) != 0 && uint64_t(selectedClip) != 0)
                {
                    m_MeshToSkeleton[selectedMesh] = selectedSkeleton;
                    
                    auto skelIt = m_SkeletonToAnimator.find(selectedSkeleton);
                    if (skelIt != m_SkeletonToAnimator.end())
                    {
                        Aether::UUID animatorID = skelIt->second;
                        skelSystem->BindClip(animatorID, selectedClip);
                        skelSystem->SetLoop(animatorID, true);
                        skelSystem->SetSpeed(animatorID, 1.0f);
                        skelSystem->Play(animatorID);
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::Text("Active Bindings:");
            ImGui::Separator();
            
            Aether::UUID toUnbind = Aether::UUID(0);
            
            for (const auto& [meshID, skeletonID] : m_MeshToSkeleton)
            {
                ImGui::PushID((uint64_t)meshID);
                
                std::string meshName = Aether::AssetsRegister::Get(meshID);
                std::string skelName = Aether::AssetsRegister::Get(skeletonID);
                
                ImGui::Text("%s -> %s", meshName.c_str(), skelName.c_str());
                
                auto skelIt = m_SkeletonToAnimator.find(skeletonID);
                if (skelIt != m_SkeletonToAnimator.end())
                {
                    Aether::UUID animatorID = skelIt->second;
                    
                    bool isPlaying = skelSystem->IsPlaying(animatorID);
                    if (ImGui::Checkbox("Playing", &isPlaying))
                    {
                        if (isPlaying) skelSystem->Play(animatorID);
                        else skelSystem->Pause(animatorID);
                    }
                    
                    float speed = skelSystem->GetSpeed(animatorID);
                    if (ImGui::SliderFloat("Speed", &speed, 0.0f, 2.0f))
                        skelSystem->SetSpeed(animatorID, speed);
                    
                    float currentTime = skelSystem->GetCurrentTime(animatorID);
                    float duration = skelSystem->GetDuration(animatorID);
                    ImGui::Text("Time: %.2f / %.2f", currentTime, duration);
                    if (duration > 0.0f)
                        ImGui::ProgressBar(currentTime / duration);
                    
                    if (ImGui::Button("Stop"))
                        skelSystem->Stop(animatorID);
                    
                    ImGui::SameLine();
                    if (ImGui::Button("Unbind"))
                    {
                        toUnbind = meshID;
                        skelSystem->Stop(animatorID);
                    }
                }
                
                ImGui::Separator();
                ImGui::PopID();
            }
            
            if (uint64_t(toUnbind) != 0)
                m_MeshToSkeleton.erase(toUnbind);
        }
        
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            glm::vec3 pos = m_Camera.GetPosition();
            ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
            ImGui::Text("Distance: %.1f", m_Camera.GetDistance());
            
            if (ImGui::Button("Reset Camera")) m_Camera.SetDistance(5.0f);
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
                    if (ImGui::Selectable(name.c_str(), isSelected)) m_SelectedMeshIndex = i; 
                    if (isSelected) ImGui::SetItemDefaultFocus();
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
                ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);
        }
    }
    ImGui::End();
}