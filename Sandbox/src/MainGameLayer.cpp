#include "MainGameLayer.h"
#include "Aether/Core/AssetsRegister.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm> // Để dùng std::clamp
#include <imgui.h>

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(4.0f); // Thiết lập khoảng cách khởi đầu ban đầu
}

void MainGameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);
    
    // --- 1. SHADOW PASS ---
    Aether::FramebufferSpec shadowFbSpec;
    shadowFbSpec.Width       = 2048;
    shadowFbSpec.Height      = 2048;
    shadowFbSpec.Attachments = { Aether::ImageFormat::DEPTH24STENCIL8 };

    m_ShadowShader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    m_ShadowShader->Bind();
    m_ShadowShader->SetUBOSlot("Bones",  1);
    m_ShadowShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass shadowPass;
    shadowPass.TargetFBO     = Aether::FrameBuffer::Create(shadowFbSpec);
    shadowPass.Shader        = m_ShadowShader;
    shadowPass.ClearDepth    = true;
    shadowPass.ClearColor    = false;
    shadowPass.OnScreen      = false;
    shadowPass.UsingMaterial = false;
    shadowPass.CullFace      = Aether::State::FRONT_CULL;
    shadowPass.attribList    = {{"u_LightIndex", 0}};

    // --- 2. MAIN PASS ---
    auto& window = Aether::Application::Get().GetWindow();
    Aether::FramebufferSpec sceneFbSpec;
    sceneFbSpec.Width       = window.GetWidth();
    sceneFbSpec.Height      = window.GetHeight();
    sceneFbSpec.Attachments = {
        Aether::ImageFormat::RGBA8,
        Aether::ImageFormat::DEPTH24STENCIL8
    };

    m_MainShader = Aether::Shader::Create("assets/shaders/Standard.shader");
    m_MainShader->Bind();
    m_MainShader->SetUBOSlot("Camera", 0);
    m_MainShader->SetUBOSlot("Bones",  1);
    m_MainShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass mainPass;
    mainPass.TargetFBO   = Aether::FrameBuffer::Create(sceneFbSpec);
    mainPass.Shader      = m_MainShader;
    mainPass.ClearColor  = true;
    mainPass.ClearDepth  = true;
    mainPass.UsingSkybox = true;
    mainPass.ClearValue  = glm::vec4(0.5f, 0.7f, 1.0f, 1.0f);
    mainPass.CullFace    = Aether::State::BACK_CULL;
    mainPass.OnScreen    = false; 
    mainPass.readList    = {{"u_DepthTex", shadowPass.TargetFBO->GetDepthAttachment()}};
    mainPass.attribList  = {{"u_LightIndex", 0}};

    // --- 3. VOLUMETRIC PASS ---
    Aether::FramebufferSpec volFbSpec;
    volFbSpec.Width       = sceneFbSpec.Width;
    volFbSpec.Height      = sceneFbSpec.Height;
    volFbSpec.Attachments = {
        Aether::ImageFormat::RGBA8,
        Aether::ImageFormat::DEPTH24STENCIL8
    };

    m_VolShader = Aether::Shader::Create("assets/shaders/Volumetric.shader");
    m_VolShader->Bind();
    m_VolShader->SetUBOSlot("Camera", 0);
    m_VolShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass volPass;
    volPass.TargetFBO     = Aether::FrameBuffer::Create(volFbSpec);
    volPass.Shader        = m_VolShader;
    volPass.ClearColor    = true;
    volPass.ClearDepth    = true;
    volPass.CullFace      = Aether::State::None;
    volPass.OnScreen      = true; 
    volPass.UsingGeometry = false;
    volPass.readList      = {
        { "u_SceneColor", mainPass.TargetFBO->GetColorAttachment() },
        { "u_SceneDepth", mainPass.TargetFBO->GetDepthAttachment() },
        { "u_ShadowMap",  shadowPass.TargetFBO->GetDepthAttachment()}
    };

    std::vector<Aether::RenderPass> pipeline = {shadowPass, mainPass, volPass};
    Aether::Renderer::SetPipeline(pipeline);

   
    m_SunLight = m_Scene.CreateEntity("Sun Light");
    auto& lightComp = m_Scene.AddComponent<Aether::LightComponent>(m_SunLight);
    
    
    lightComp.Config.type = Aether::LightType::Directional; 
    lightComp.Config.color = glm::vec3(0.9f, 0.95f, 1.0f); 
    lightComp.Config.intensity = 1.5f; 
    lightComp.Config.castShadows = true;
    lightComp.Config.direction = glm::vec3(-0.5f, -1.0f, -0.5f); 

    auto& sunTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
    sunTransform.Rotation = glm::quat(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0.0f));
    sunTransform.Translation = glm::vec3(0.0f, 50.0f, 0.0f); // Treo thật cao lên
    sunTransform.Dirty = true;

    
    auto parsedMap = Aether::Importer::Import("assets/models/map.glb"); 
    auto uploadResult = Aether::Importer::Upload(parsedMap);
    
    if (!uploadResult.meshIDs.empty()) {
        m_BaseMapMesh = uploadResult.meshIDs[0]; 
        for (auto& meshID : uploadResult.meshIDs) m_LoadedMeshes.push_back(meshID);
    }

    m_Player = m_Scene.CreateEntity("Player");
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
    pTransform.Translation = {0.0f, -0.7f, 0.0f}; 
    pTransform.Scale = {1.0f, 1.0f, 1.0f};
    pTransform.Dirty = true;
    
    auto parsedPlayer = Aether::Importer::Import("assets/models/humanv2.glb"); 
    auto uploadPlayer = Aether::Importer::Upload(parsedPlayer);
    
    m_Scene.LoadHierarchy(uploadPlayer, m_Player);

    if (!uploadPlayer.animatorIDS.empty()) m_RunAnimation = uploadPlayer.animatorIDS[0]; 

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    rigSystem->BindClip(m_RunAnimation, 0);

    AE_CORE_INFO("MainGameLayer Started! Infinite Cube Floor is ready.");
}

void MainGameLayer::Detach()
{
    m_ShadowShader.reset();
    m_MainShader.reset();
    m_VolShader.reset();
    m_ActiveChunks.clear();
}

void MainGameLayer::Update(Aether::Timestep ts)
{
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    float camDistance = m_Camera.GetDistance();
    
    m_CurrentRenderDistance = m_BaseRenderDistance + static_cast<int>(camDistance / m_ZoomInfluence);
    m_CurrentRenderDistance = std::clamp(m_CurrentRenderDistance, 1, 30);

    
    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        
        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight   = m_Camera.GetRightDirection();

        camForward.y = 0.0f;
        camRight.y   = 0.0f;
        
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight) > 0.0f)   camRight   = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);

        if (ImGui::IsKeyDown(ImGuiKey_W)) moveDir += camForward;
        if (ImGui::IsKeyDown(ImGuiKey_S)) moveDir -= camForward;
        if (ImGui::IsKeyDown(ImGuiKey_A)) moveDir -= camRight;
        if (ImGui::IsKeyDown(ImGuiKey_D)) moveDir += camRight;

        bool isMoving = glm::length(moveDir) > 0.0f;
        static bool wasMoving = false;
        if (isMoving != wasMoving)
        {
           
            auto meshView = m_Scene.View<Aether::MeshComponent>();
            for (auto entity : meshView)
            {
                if (m_Scene.HasComponent<Aether::AnimatorComponent>(entity)) {
                    auto& animator = m_Scene.GetComponent<Aether::AnimatorComponent>(entity);
                    if (isMoving) rigSystem->Play(m_RunAnimation);
                    else rigSystem->Pause(m_RunAnimation);
                }
            }
            wasMoving = isMoving;
        }

        if (isMoving)
        {
            moveDir = glm::normalize(moveDir); 
            pTransform.Translation += moveDir * (m_PlayerSpeed * ts);
            
            float targetAngle = glm::atan(moveDir.x, moveDir.z);
            pTransform.Rotation = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
            
            pTransform.Dirty = true;
        }

        glm::vec3 targetPos = pTransform.Translation + glm::vec3(0.0f, 1.0f, 0.0f);
        m_Camera.SetFocalPoint(targetPos);

        if (m_SunLight != Null_Entity && m_Scene.IsValid(m_SunLight) && 
            m_Player != Null_Entity && m_Scene.IsValid(m_Player))
        {
            auto& lightTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
            auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
            
            lightTransform.Translation = pTransform.Translation + glm::vec3(0.0f, 50.0f, 0.0f);
            lightTransform.Dirty = true;
        }

        UpdateMapChunks(pTransform.Translation);
    }

    if (m_AutoRotate)
    {
        auto meshView = m_Scene.View<Aether::MeshComponent, Aether::TransformComponent>();
        for (auto entity : meshView)
        {
            if (m_Scene.GetComponent<Aether::TagComponent>(entity).Tag.find("MapGrid") == std::string::npos) {
                auto& t = m_Scene.GetComponent<Aether::TransformComponent>(entity);
                t.Rotation.y += ts * m_RotationSpeed;
                t.Dirty = true;
            }
        }
    }

    m_VolShader->Bind();
    m_VolShader->SetFloat("u_Density",    m_VolDensity);
    m_VolShader->SetFloat("u_Intensity",  m_VolIntensity);
    m_VolShader->SetInt  ("u_Steps",      m_VolSteps);
    m_VolShader->SetFloat("u_VolBias",    m_ShadowBias);
    m_VolShader->SetFloat("u_MaxDistance", 100.0f);

    m_MainShader->Bind();
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);

    m_Camera.Update(ts);
    m_Scene.Update(ts, &m_Camera);
}

void MainGameLayer::UpdateMapChunks(const glm::vec3& playerPos)
{
    if (m_LoadedMeshes.empty()) return;

    // ĐỔI floor THÀNH round ĐỂ CANH GIỮA CHUẨN XÁC
    int currentX = static_cast<int>(std::round(playerPos.x / m_ChunkSize));
    int currentZ = static_cast<int>(std::round(playerPos.z / m_ChunkSize));

    std::vector<std::pair<int, int>> chunksToKeep;

    // Sinh map ra 4 phía xung quanh nhân vật (âm, dương, trái, phải, trước, sau)
    for (int x = -m_CurrentRenderDistance; x <= m_CurrentRenderDistance; ++x)
    {
        for (int z = -m_CurrentRenderDistance; z <= m_CurrentRenderDistance; ++z)
        {
            int targetX = currentX + x;
            int targetZ = currentZ + z;
            auto coord = std::make_pair(targetX, targetZ);
            chunksToKeep.push_back(coord);

            // Nếu ô đất này chưa tồn tại thì đẻ nó ra
            if (m_ActiveChunks.find(coord) == m_ActiveChunks.end())
            {
                Entity chunk = m_Scene.CreateEntity("MapGrid_" + std::to_string(targetX) + "_" + std::to_string(targetZ));
                
                auto& t = m_Scene.GetComponent<Aether::TransformComponent>(chunk);
                
                // CĂN CHỈNH SÁT NHAU: targetX * m_ChunkSize
                // Hạ y xuống -1.0f (nếu ChunkSize = 2.0f) để mặt trên của khối vuông nằm đúng tại y = 0
                float yOffset = -(m_ChunkSize / 2.0f); 
                t.Translation = glm::vec3(targetX * m_ChunkSize, yOffset, targetZ * m_ChunkSize);
                t.Dirty = true;

                m_Scene.AddComponent<Aether::MeshComponent>(chunk).MeshID = m_BaseMapMesh;
                m_ActiveChunks[coord] = chunk;
            }
        }
    }

    // Xóa các ô đất ở quá xa đằng sau lưng (để đỡ lag)
    for (auto it = m_ActiveChunks.begin(); it != m_ActiveChunks.end(); )
    {
        if (std::find(chunksToKeep.begin(), chunksToKeep.end(), it->first) == chunksToKeep.end())
        {
            if (it->second != Null_Entity && m_Scene.IsValid(it->second)) {
                m_Scene.DestroyEntity(it->second);
            }
            it = m_ActiveChunks.erase(it);
        }
        else {
            ++it;
        }
    }
}

void MainGameLayer::OnImGuiRender()
{
    ImGui::Begin("Game Controls");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Dieu khien: W A S D (Chay theo huong nhin Camera)");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Dynamic Map Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Toc do chay", &m_PlayerSpeed, 5.0f, 50.0f);
        // ImGui::SliderFloat("Kich thuoc 1 o Map", &m_ChunkSize, 1.0f, 100.0f);
        
        ImGui::Separator();
        ImGui::Text("--- Camera Zoom Logic ---");
        ImGui::SliderInt("Ban kinh toi thieu (Base)", &m_BaseRenderDistance, 1, 20);
        ImGui::SliderFloat("Ti le Zoom -> Map", &m_ZoomInfluence, 5.0f, 50.0f, "%.1f");
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Zoom Camera hien tai: %.1f", m_Camera.GetDistance());
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "-> Ban kinh Render dang Load: %d", m_CurrentRenderDistance);
        ImGui::Text("So luong o dat hien tai: %d", (int)m_ActiveChunks.size());
    }
    ImGui::End();

    // BẢNG CĂN CHỈNH NHÂN VẬT (Debug Player)
    if (ImGui::Begin("Player Setup"))
    {
        if (m_Player != Null_Entity && m_Scene.IsValid(m_Player))
        {
            auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
            
            ImGui::Text("Chinh cho chan cham dat:");
            if (ImGui::DragFloat3("Position", glm::value_ptr(pTransform.Translation), 0.01f)) pTransform.Dirty = true;
            
            ImGui::Text("Chinh kich thuoc to/nho:");
            // Kéo bước nhảy 0.01f để chỉnh chi tiết nếu model quá to
            if (ImGui::DragFloat3("Scale", glm::value_ptr(pTransform.Scale), 0.01f)) pTransform.Dirty = true;
        }
    }
    ImGui::End();

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawLightingPanel();
}

void MainGameLayer::DrawEntityNode(Entity entity)
{
    auto& tag  = m_Scene.GetComponent<Aether::TagComponent>(entity);
    auto& hier = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (hier.firstChild == Null_Entity) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedEntity == entity)     flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", tag.Tag.c_str());

    if (ImGui::IsItemClicked()) m_SelectedEntity = entity;

    if (open)
    {
        Entity child = hier.firstChild;
        while (child != Null_Entity)
        {
            Entity next = m_Scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
            DrawEntityNode(child);
            child = next;
        }
        ImGui::TreePop();
    }
}

void MainGameLayer::DrawHierarchyPanel()
{
    if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }
    auto view = m_Scene.View<Aether::HierarchyComponent>();
    for (auto entity : view)
    {
        if (m_Scene.GetComponent<Aether::HierarchyComponent>(entity).parent == Null_Entity)
            DrawEntityNode(entity);
    }
    ImGui::End();
}

void MainGameLayer::DrawScenePanel()
{
    if (!ImGui::Begin("Scene")) { ImGui::End(); return; }
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen) && m_SelectedEntity != Null_Entity && m_Scene.IsValid(m_SelectedEntity))
    {
        auto& t = m_Scene.GetComponent<Aether::TransformComponent>(m_SelectedEntity);
        if (ImGui::DragFloat3("Position", glm::value_ptr(t.Translation), 0.1f)) t.Dirty = true;
        if (ImGui::DragFloat4("Rotation", glm::value_ptr(t.Rotation),    0.1f)) t.Dirty = true;
        if (ImGui::DragFloat3("Scale",    glm::value_ptr(t.Scale),       0.05f)) t.Dirty = true;
    }
    ImGui::End();
}

void MainGameLayer::DrawLightingPanel()
{
    if (!ImGui::Begin("Lighting")) { ImGui::End(); return; }
    if (ImGui::CollapsingHeader("Sun Light", ImGuiTreeNodeFlags_DefaultOpen) && m_SunLight != Null_Entity && m_Scene.IsValid(m_SunLight))
    {
        auto& light = m_Scene.GetComponent<Aether::LightComponent>(m_SunLight).Config;
        ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);
        ImGui::DragFloat3("Direction", glm::value_ptr(light.direction), 0.01f, -1.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::SliderFloat("Bias", &m_ShadowBias, 0.00001f, 0.005f, "%.5f");
    ImGui::End();
}

void MainGameLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled) m_Camera.OnEvent(event);
}