#include "MainGameLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>
#include <imgui.h>

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(4.0f);
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
        { "u_ShadowMap",  shadowPass.TargetFBO->GetDepthAttachment() }
    };

    std::vector<Aether::RenderPass> pipeline = {shadowPass, mainPass, volPass};
    Aether::Renderer::SetPipeline(pipeline);

    // --- ÁNH SÁNG MẶT TRỜI ---
    m_SunLight = m_Scene.CreateEntity("Sun Light");
    auto& lightComp = m_Scene.AddComponent<Aether::LightComponent>(m_SunLight);
    lightComp.Config.type        = Aether::LightType::Directional;
    lightComp.Config.color       = glm::vec3(0.9f, 0.95f, 1.0f);
    lightComp.Config.intensity   = 1.5f;
    lightComp.Config.castShadows = true;
    lightComp.Config.direction   = glm::vec3(-0.5f, -1.0f, -0.5f);

    auto& sunTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
    sunTransform.Rotation    = glm::quat(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0.0f));
    sunTransform.Translation = glm::vec3(0.0f, 50.0f, 0.0f);
    sunTransform.Dirty       = true;

    // --- TẢI MAP ---
    auto uploadMap = Aether::Importer::Upload(Aether::Importer::Import("assets/models/map.glb"));
    if (!uploadMap.meshIDs.empty()) {
        m_BaseMapMesh     = Aether::AssetsManager::GetResource<Aether::Mesh>(uploadMap.meshIDs[0]);
        m_BaseMapMaterial = Aether::AssetsManager::GetResource<Aether::Material>(uploadMap.matIDs[0]);
        for (auto& id : uploadMap.meshIDs) m_LoadedMeshes.push_back(id);
    }

    // --- TẢI PLAYER ---
    m_Player = m_Scene.CreateEntity("Player");
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
    pTransform.Translation = {0.0f, -1.75f, 0.0f};
    pTransform.Scale       = {1.0f, 1.0f, 1.0f};
    pTransform.Dirty       = true;

    auto uploadPlayer = Aether::Importer::Upload(Aether::Importer::Import("assets/models/humanv2.glb"));
    m_Scene.LoadHierarchy(uploadPlayer, m_Player);

    if (!uploadPlayer.animatorIDS.empty())
        m_RunAnimation = uploadPlayer.animatorIDS[0];

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    {
        auto clips = rigSystem->GetClips(m_RunAnimation);
        if (!clips.empty()) rigSystem->BindClip(m_RunAnimation, clips[0]);
    }

    // --- PHYSICS: Player Kinematic Capsule ---
    // Kinematic vì ta tự điều khiển vị trí, physics chỉ lo va chạm
    m_PlayerBodyID = Aether::AssetsRegister::Register("Player_Body");
    {
        Aether::BodyConfig cfg;
        cfg.motionType  = Aether::MotionType::Kinematic;
        cfg.shape       = Aether::ColliderShape::Capsule;
        cfg.size        = glm::vec3(0.35f, 0.9f, 0.0f); // radius=0.35, halfHeight=0.9
        cfg.transform   = { pTransform.Translation, glm::quat(1,0,0,0) };
        cfg.friction    = 0.5f;
        cfg.restitution = 0.0f;
        Aether::PhysicsSystem::CreateBody(m_PlayerBodyID, cfg);
        m_Scene.AddComponent<Aether::ColliderComponent>(m_Player, m_PlayerBodyID, glm::vec3(0.0f));
    }

    // --- TẢI ZOMBIE (Lưu RegisteredScene để dùng lại khi spawn) ---
    // GPU data chỉ upload 1 lần, LoadHierarchy sau đó chỉ tốn CPU
    m_ZombieSceneData = Aether::Importer::Upload(Aether::Importer::Import("assets/models/zombie.glb"));

    if (!m_ZombieSceneData.animatorIDS.empty())
        m_ZombieRunAnimation = m_ZombieSceneData.animatorIDS[0];

    // --- TẢI SÚNG ---
    m_Gun = m_Scene.CreateEntity("Weapon_Gun");
    auto& gTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);
    gTransform.Translation = {0.0f, 0.0f, 0.0f};
    gTransform.Scale       = {1.0f, 1.0f, 1.0f};
    gTransform.Dirty       = true;

    auto uploadGun = Aether::Importer::Upload(Aether::Importer::Import("assets/models/gun.glb"));
    m_Scene.LoadHierarchy(uploadGun, m_Gun);

    if (!uploadGun.animatorIDS.empty()) {
        m_ShootAnimation = uploadGun.animatorIDS[0];
        auto clips = rigSystem->GetClips(m_ShootAnimation);
        AE_INFO("animator id: {0}, clips num: {1}", uint64_t(m_ShootAnimation), clips.size());
        if (!clips.empty()) rigSystem->BindClip(m_ShootAnimation, clips[0]);
        rigSystem->SetLoop(m_ShootAnimation, false);
    }

    AE_CORE_INFO("MainGameLayer Started! Infinite Cube Floor is ready.");

    Aether::PhysicsSystem::SetGravity({0.0f, 0.0f, 0.0f});
}

void MainGameLayer::Detach()
{
    // Dọn sạch animator và physics body của từng zombie
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    for (auto& [entity, animID] : m_ZombieAnimators)
    {
        if (rigSystem) rigSystem->DestroyAnimator(animID);
        if (m_Scene.IsValid(entity))
            m_Scene.DestroyEntity(entity);
    }
    for (auto& [entity, bodyID] : m_ZombieBodyIDs)
        Aether::PhysicsSystem::DestroyBody(bodyID);
    m_ZombieAnimators.clear();
    m_ZombieBodyIDs.clear();
    m_ActiveZombies.clear();

    // Dọn physics body của Player
    if (m_PlayerBodyID != 0)
        Aether::PhysicsSystem::DestroyBody(m_PlayerBodyID);

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

        glm::vec3 playerTopPos = pTransform.Translation + glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 playerEyePos = pTransform.Translation + glm::vec3(0.0f, 1.7f, 0.0f);

        // --- XỬ LÝ DI CHUYỂN ---
        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight   = m_Camera.GetRightDirection();
        camForward.y = 0.0f; camRight.y = 0.0f;
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight)   > 0.0f) camRight   = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);
        if (ImGui::IsKeyDown(ImGuiKey_W)) moveDir += camForward;
        if (ImGui::IsKeyDown(ImGuiKey_S)) moveDir -= camForward;
        if (ImGui::IsKeyDown(ImGuiKey_A)) moveDir -= camRight;
        if (ImGui::IsKeyDown(ImGuiKey_D)) moveDir += camRight;

        // --- HOẠT ẢNH PLAYER ---
        bool isMoving = glm::length(moveDir) > 0.0f;
        static bool wasMoving = false;
        if (isMoving != wasMoving)
        {
            if (isMoving) rigSystem->Play(m_RunAnimation);
            else          rigSystem->Pause(m_RunAnimation);
            wasMoving = isMoving;
        }

        // --- DI CHUYỂN CÓ QUÁN TÍNH ---
        glm::vec3 targetVelocity(0.0f);
        if (isMoving)
        {
            moveDir       = glm::normalize(moveDir);
            targetVelocity = moveDir * m_PlayerSpeed;

            if (!m_FirstPerson)
            {
                float targetAngle = glm::atan(moveDir.x, moveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(pTransform.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                float blend = 1.0f - glm::exp(-15.0f * (float)ts);
                pTransform.Rotation = glm::slerp(pTransform.Rotation, targetRot, blend);
            }
            pTransform.Dirty = true;
        }

        float friction = 12.0f;
        m_PlayerVelocity = glm::mix(m_PlayerVelocity, targetVelocity, 1.0f - glm::exp(-friction * (float)ts));

        if (glm::length(m_PlayerVelocity) > 0.01f) {
            pTransform.Translation += m_PlayerVelocity * (float)ts;
            pTransform.Dirty = true;
        }

        // --- XỬ LÝ CAMERA ---
        if (m_FirstPerson)
        {
            pTransform.Scale = {0.0f, 0.0f, 0.0f};
            m_Camera.SetDistance(0.5f);
            m_Camera.SetFocalPoint(playerEyePos);
            float camYaw = m_Camera.GetYaw();
            pTransform.Rotation = glm::quat(glm::vec3(0.0f, -camYaw, 0.0f));
            pTransform.Dirty    = true;
        }
        else
        {
            pTransform.Scale = {1.0f, 1.0f, 1.0f};
            m_Camera.SetFocalPoint(playerTopPos);
            if (m_LockCamera)
            {
                m_Camera.SetDistance(5.0f);
                if (m_Camera.GetPitch() < 0.2f) m_Camera.SetPitch(0.2f);
            }
        }

        // --- CẬP NHẬT ÁNH SÁNG ---
        if (m_SunLight != Aether::Null_Entity && m_Scene.IsValid(m_SunLight))
        {
            auto& lightTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
            lightTransform.Translation = playerTopPos + glm::vec3(0.0f, 50.0f, 0.0f);
            lightTransform.Dirty       = true;
        }

        UpdateMapChunks(pTransform.Translation);

        // --- FLOW FIELD & ZOMBIE AI ---
        m_FlowFieldTimer += (float)ts;
        if (m_FlowFieldTimer >= 0.2f)
        {
            UpdateFlowField(pTransform.Translation);
            m_FlowFieldTimer = 0.0f;
        }

        for (Aether::Entity zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;
            auto& zT = m_Scene.GetComponent<Aether::TransformComponent>(zombie);

            int zX = static_cast<int>(std::round(zT.Translation.x / m_PathGridSize));
            int zZ = static_cast<int>(std::round(zT.Translation.z / m_PathGridSize));
            auto zCoord = std::make_pair(zX, zZ);

            glm::vec3 zMoveDir(0.0f);
            if (m_FlowField.find(zCoord) != m_FlowField.end() && glm::length(m_FlowField[zCoord].direction) > 0.0f) {
                zMoveDir = m_FlowField[zCoord].direction;
            } else {
                zMoveDir = glm::normalize(pTransform.Translation - zT.Translation);
                zMoveDir.y = 0.0f;
            }

            glm::vec3 diff = pTransform.Translation - zT.Translation;
            diff.y = 0.0f;
            if (glm::length(diff) > 1.2f)
            {
                zT.Translation += zMoveDir * (m_ZombieSpeed * (float)ts);

                float targetAngle = glm::atan(zMoveDir.x, zMoveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(zT.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                zT.Rotation = glm::slerp(zT.Rotation, targetRot, 1.0f - glm::exp(-10.0f * (float)ts));
                zT.Dirty    = true;
            }
        }
    }

    // --- AUTO ROTATE (Debug) ---
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

    // --- SHADER UNIFORMS ---
    m_VolShader->Bind();
    m_VolShader->SetFloat("u_Density",    m_VolDensity);
    m_VolShader->SetFloat("u_Intensity",  m_VolIntensity);
    m_VolShader->SetInt  ("u_Steps",      m_VolSteps);
    m_VolShader->SetFloat("u_VolBias",    m_ShadowBias);
    m_VolShader->SetFloat("u_MaxDistance", 100.0f);

    m_MainShader->Bind();
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);

    m_Camera.Update(ts);

    // --- CẬP NHẬT SÚNG (NGAY SAU CAMERA) ---
    if (m_Scene.IsValid(m_Gun) && m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        auto& gTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);

        if (m_FirstPerson)
        {
            glm::vec3 camPos  = m_Camera.GetPosition();
            glm::vec3 forward = m_Camera.GetForwardDirection();
            glm::vec3 right   = m_Camera.GetRightDirection();
            glm::vec3 up      = m_Camera.GetUpDirection();

            gTransform.Translation = camPos + (right * m_GunPosFP.x) + (up * m_GunPosFP.y) + (forward * m_GunPosFP.z);

            glm::quat camQuat    = glm::quat(glm::vec3(-m_Camera.GetPitch(), -m_Camera.GetYaw(), 0.0f));
            glm::quat offsetQuat = glm::quat(glm::radians(m_GunRotFP));
            gTransform.Rotation  = camQuat * offsetQuat;
            gTransform.Scale     = m_GunScaleFP;
        }
        else
        {
            glm::vec3 pPos = pTransform.Translation;
            glm::quat pRot = pTransform.Rotation;

            glm::vec3 forward = pRot * glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 right   = pRot * glm::vec3(1.0f, 0.0f,  0.0f);
            glm::vec3 up      = pRot * glm::vec3(0.0f, 1.0f,  0.0f);

            gTransform.Translation = pPos + (right * m_GunPosTP.x) + (up * m_GunPosTP.y) + (forward * m_GunPosTP.z);

            glm::quat offsetQuat = glm::quat(glm::radians(m_GunRotTP));
            gTransform.Rotation  = pRot * offsetQuat;
            gTransform.Scale     = m_GunScaleTP;
        }
        gTransform.Dirty = true;
    }

    m_Scene.Update(ts, &m_Camera);
}

void MainGameLayer::UpdateMapChunks(const glm::vec3& playerPos)
{
    if (m_LoadedMeshes.empty()) return;

    float myScaleXZ      = 2.0f;
    float actualChunkSize = m_ChunkSize * myScaleXZ;

    int currentX = static_cast<int>(std::round(playerPos.x / actualChunkSize));
    int currentZ = static_cast<int>(std::round(playerPos.z / actualChunkSize));

    std::vector<std::pair<int, int>> chunksToKeep;

    for (int x = -m_CurrentRenderDistance; x <= m_CurrentRenderDistance; ++x)
    {
        for (int z = -m_CurrentRenderDistance; z <= m_CurrentRenderDistance; ++z)
        {
            int targetX = currentX + x;
            int targetZ = currentZ + z;
            auto coord  = std::make_pair(targetX, targetZ);
            chunksToKeep.push_back(coord);

            if (m_ActiveChunks.find(coord) == m_ActiveChunks.end())
            {
                Aether::Entity chunk = m_Scene.CreateEntity(
                    "MapGrid_" + std::to_string(targetX) + "_" + std::to_string(targetZ));

                auto& t = m_Scene.GetComponent<Aether::TransformComponent>(chunk);
                float yOffset = -(actualChunkSize / 2.0f);
                t.Translation = glm::vec3(targetX * actualChunkSize, yOffset, targetZ * actualChunkSize);
                t.Scale       = {myScaleXZ, 1.0f, myScaleXZ};
                t.Dirty       = true;

                auto& chunkcmp     = m_Scene.AddComponent<Aether::MeshComponent>(chunk);
                chunkcmp.MeshPtr   = m_BaseMapMesh;
                chunkcmp.Materials = {m_BaseMapMaterial};
                m_ActiveChunks[coord] = chunk;

                // 15% cơ hội spawn zombie, không sát Player
                if (std::rand() % 100 < 15 && (std::abs(x) > 2 || std::abs(z) > 2))
                {
                    glm::vec3 spawnPos = t.Translation;
                    spawnPos.y = -1.75f;
                    SpawnZombie(spawnPos);
                }
            }
        }
    }

    // Xóa chunk quá xa
    for (auto it = m_ActiveChunks.begin(); it != m_ActiveChunks.end(); )
    {
        if (std::find(chunksToKeep.begin(), chunksToKeep.end(), it->first) == chunksToKeep.end())
        {
            if (it->second != Aether::Null_Entity && m_Scene.IsValid(it->second))
                m_Scene.DestroyEntity(it->second);
            it = m_ActiveChunks.erase(it);
        }
        else {
            ++it;
        }
    }
}

// ==========================================
// HÀM ĐẺ ZOMBIE — Mỗi con có animator riêng
// ==========================================
void MainGameLayer::SpawnZombie(const glm::vec3& position)
{
    if (m_ActiveZombies.size() >= 50) return;

    // 1. Clone animator TRƯỚC khi LoadHierarchy để có UUID sẵn
    Aether::UUID newAnimID = Aether::AssetsRegister::Register(
        "ZombieAnim_" + std::to_string(m_ActiveZombies.size()));

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    if (rigSystem) {
        rigSystem->CloneAnimator(newAnimID, m_ZombieRunAnimation);
        rigSystem->BindClip(newAnimID, 4);
        rigSystem->Play(newAnimID);
    }

    // 2. Tạm thời đổi animatorID trong scene data
    // CreateNodeEntity đọc reg.animatorIDS[node.animatorIdx] -> tự stamp newAnimID vào AnimatorComponent
    Aether::UUID originalAnimID = m_ZombieSceneData.animatorIDS[0];
    m_ZombieSceneData.animatorIDS[0] = newAnimID;

    // 3. Tạo entity và load hierarchy
    Aether::Entity newZombie = m_Scene.CreateEntity("Zombie_Minion");
    auto& zTransform = m_Scene.GetComponent<Aether::TransformComponent>(newZombie);
    zTransform.Translation = position;
    zTransform.Scale       = {1.0f, 1.0f, 1.0f};
    zTransform.Dirty       = true;

    m_Scene.LoadHierarchy(m_ZombieSceneData, newZombie);

    // 4. Khôi phục ID gốc
    m_ZombieSceneData.animatorIDS[0] = originalAnimID;

    // 5. Physics: Kinematic Capsule
    Aether::UUID bodyID = Aether::AssetsRegister::Register(
        "ZombieBody_" + std::to_string(m_ActiveZombies.size()));
    {
        Aether::BodyConfig cfg;
        cfg.motionType  = Aether::MotionType::Dynamic;
        cfg.shape       = Aether::ColliderShape::Capsule;
        cfg.size        = glm::vec3(0.35f, 0.9f, 0.0f); // radius, halfHeight
        cfg.transform   = { position, glm::quat(1,0,0,0) };
        cfg.friction    = 0.5f;
        cfg.restitution = 0.0f;
        Aether::PhysicsSystem::CreateBody(bodyID, cfg);
        m_Scene.AddComponent<Aether::ColliderComponent>(newZombie, bodyID, glm::vec3(0.0f));
    }

    // 6. Lưu lại để cleanup sau
    m_ZombieAnimators[newZombie] = newAnimID;
    m_ZombieBodyIDs[newZombie]   = bodyID;
    m_ActiveZombies.push_back(newZombie);
}

void MainGameLayer::UpdateFlowField(const glm::vec3& targetPos)
{
    // 1. Reset integration field
    for (auto& pair : m_FlowField) {
        pair.second.bestCost  = 999999;
        pair.second.direction = glm::vec3(0.0f);
    }

    int targetX = static_cast<int>(std::round(targetPos.x / m_PathGridSize));
    int targetZ = static_cast<int>(std::round(targetPos.z / m_PathGridSize));
    auto targetCoord = std::make_pair(targetX, targetZ);

    m_FlowField[targetCoord].bestCost = 0;
    m_FlowField[targetCoord].cost     = 1;

    // 2. BFS / Dijkstra loang khoảng cách
    std::queue<std::pair<int, int>> openList;
    openList.push(targetCoord);

    std::vector<std::pair<int, int>> neighbors = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    int maxRadius = 40;

    while (!openList.empty())
    {
        auto current = openList.front();
        openList.pop();

        if (std::abs(current.first - targetX) > maxRadius ||
            std::abs(current.second - targetZ) > maxRadius) continue;

        int currCost = m_FlowField[current].bestCost;

        for (auto& offset : neighbors)
        {
            auto neighborCoord = std::make_pair(current.first + offset.first, current.second + offset.second);

            if (m_FlowField.find(neighborCoord) == m_FlowField.end()) {
                m_FlowField[neighborCoord].cost     = 1;
                m_FlowField[neighborCoord].bestCost = 999999;
            }

            if (m_FlowField[neighborCoord].cost >= 255) continue;

            int moveCost = (offset.first != 0 && offset.second != 0) ? 14 : 10;
            int newCost  = currCost + (moveCost * m_FlowField[neighborCoord].cost);

            if (newCost < m_FlowField[neighborCoord].bestCost)
            {
                m_FlowField[neighborCoord].bestCost = newCost;
                openList.push(neighborCoord);
            }
        }
    }

    // 3. Tạo vector hướng cho mỗi ô
    for (auto& pair : m_FlowField)
    {
        auto current = pair.first;
        if (pair.second.cost >= 255 || pair.second.bestCost == 999999) continue;

        int minCost = pair.second.bestCost;
        std::pair<int, int> bestNeighbor = current;

        for (auto& offset : neighbors)
        {
            auto neighborCoord = std::make_pair(current.first + offset.first, current.second + offset.second);
            if (m_FlowField.find(neighborCoord) != m_FlowField.end())
            {
                if (m_FlowField[neighborCoord].bestCost < minCost)
                {
                    minCost      = m_FlowField[neighborCoord].bestCost;
                    bestNeighbor = neighborCoord;
                }
            }
        }

        if (bestNeighbor != current) {
            glm::vec3 dir = glm::vec3(
                float(bestNeighbor.first  - current.first),
                0.0f,
                float(bestNeighbor.second - current.second));
            pair.second.direction = glm::normalize(dir);
        }
    }
}

void MainGameLayer::OnImGuiRender()
{
    ImGui::Begin("Game Controls");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Dieu khien: W A S D (Chay theo huong nhin Camera)");
    ImGui::Separator();

    if (ImGui::Checkbox("Lock Camera (Play Mode)", &m_LockCamera))
    {
        if (m_LockCamera) AE_CORE_INFO("Camera Locked: Mode Play");
        else              AE_CORE_INFO("Camera Unlocked: Mode Editor");
    }

    if (m_LockCamera)
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: Locked (No Zoom, No Under-ground)");
    else
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Free (Editor Mode)");

    if (ImGui::CollapsingHeader("Dynamic Map Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Toc do chay",           &m_PlayerSpeed,        5.0f,  50.0f);
        ImGui::Separator();
        ImGui::Text("--- Camera Zoom Logic ---");
        ImGui::SliderInt  ("Ban kinh toi thieu",    &m_BaseRenderDistance, 1,     30);
        ImGui::SliderFloat("Ti le Zoom -> Map",     &m_ZoomInfluence,      5.0f,  50.0f, "%.1f");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Zoom Camera hien tai: %.1f", m_Camera.GetDistance());
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "-> Ban kinh Render dang Load: %d", m_CurrentRenderDistance);
        ImGui::Text("So luong o dat: %d  |  Zombie: %d", (int)m_ActiveChunks.size(), (int)m_ActiveZombies.size());
    }
    ImGui::End();

    // --- BẢNG PLAYER ---
    if (ImGui::Begin("Player Setup"))
    {
        if (m_Player != Aether::Null_Entity && m_Scene.IsValid(m_Player))
        {
            auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
            ImGui::Text("Chinh cho chan cham dat:");
            if (ImGui::DragFloat3("Position", glm::value_ptr(pTransform.Translation), 0.01f)) pTransform.Dirty = true;
            ImGui::Text("Chinh kich thuoc to/nho:");
            if (ImGui::DragFloat3("Scale",    glm::value_ptr(pTransform.Scale),       0.01f)) pTransform.Dirty = true;
        }
    }
    ImGui::End();

    // --- BẢNG SÚNG ---
    if (ImGui::Begin("Weapon Setup (Gun)"))
    {
        ImGui::Text("Tim 'diem vang' cho tung goc nhin.");
        ImGui::Separator();

        if (m_FirstPerson)
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "--- DANG O: 1ST PERSON ---");
            ImGui::DragFloat3("FP Position", glm::value_ptr(m_GunPosFP),   0.01f);
            ImGui::DragFloat3("FP Rotation", glm::value_ptr(m_GunRotFP),   1.0f);
            ImGui::DragFloat3("FP Scale",    glm::value_ptr(m_GunScaleFP), 0.01f);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "--- DANG O: 3RD PERSON ---");
            ImGui::DragFloat3("TP Position", glm::value_ptr(m_GunPosTP),   0.01f);
            ImGui::DragFloat3("TP Rotation", glm::value_ptr(m_GunRotTP),   1.0f);
            ImGui::DragFloat3("TP Scale",    glm::value_ptr(m_GunScaleTP), 0.01f);
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Luu y: Ghi chep lai cac so nay sau khi tim xong!");
        ImGui::Text("Bam 'V' hoac cuon chuot de doi goc nhin.");
    }
    ImGui::End();

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawLightingPanel();
}

// --- Vẽ từng node trong hierarchy ---
void MainGameLayer::DrawEntityNode(Aether::Entity entity)
{
    auto& tag  = m_Scene.GetComponent<Aether::TagComponent>(entity);
    auto& hier = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (hier.firstChild == Aether::Null_Entity) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedEntity == entity)             flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", tag.Tag.c_str());
    if (ImGui::IsItemClicked()) m_SelectedEntity = entity;

    if (open)
    {
        Aether::Entity child = hier.firstChild;
        while (child != Aether::Null_Entity)
        {
            Aether::Entity next = m_Scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
            DrawEntityNode(child);
            child = next;
        }
        ImGui::TreePop();
    }
}

// --- Bảng Hierarchy ---
void MainGameLayer::DrawHierarchyPanel()
{
    if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }

    auto view = m_Scene.View<Aether::HierarchyComponent>();
    for (auto entity : view)
    {
        if (m_Scene.GetComponent<Aether::HierarchyComponent>(entity).parent == Aether::Null_Entity)
            DrawEntityNode(entity);
    }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        m_SelectedEntity = Aether::Null_Entity;

    ImGui::End();
}

// --- Bảng Inspector ---
void MainGameLayer::DrawScenePanel()
{
    if (!ImGui::Begin("Inspector")) { ImGui::End(); return; }

    if (m_SelectedEntity != Aether::Null_Entity && m_Scene.IsValid(m_SelectedEntity))
    {
        ImGui::Text("Entity ID: %d", (uint32_t)m_SelectedEntity);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& t = m_Scene.GetComponent<Aether::TransformComponent>(m_SelectedEntity);
            if (ImGui::DragFloat3("Position", glm::value_ptr(t.Translation), 0.1f))  t.Dirty = true;
            if (ImGui::DragFloat4("Rotation", glm::value_ptr(t.Rotation),    0.1f))  t.Dirty = true;
            if (ImGui::DragFloat3("Scale",    glm::value_ptr(t.Scale),       0.05f)) t.Dirty = true;
        }
    }
    else {
        ImGui::Text("Select an entity to see properties.");
    }

    ImGui::End();
}

// --- Bảng Lighting ---
void MainGameLayer::DrawLightingPanel()
{
    if (!ImGui::Begin("Lighting")) { ImGui::End(); return; }

    if (m_SunLight != Aether::Null_Entity && m_Scene.IsValid(m_SunLight))
    {
        if (ImGui::CollapsingHeader("Sun Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& light = m_Scene.GetComponent<Aether::LightComponent>(m_SunLight).Config;
            ImGui::ColorEdit3("Sun Color", glm::value_ptr(light.color));
            ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
            ImGui::DragFloat3("Direction",  glm::value_ptr(light.direction), 0.01f, -1.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Shadow Bias", &m_ShadowBias, 0.00001f, 0.005f, "%.5f");
    }

    ImGui::End();
}

void MainGameLayer::OnEvent(Aether::Event& event)
{
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

    // --- PHÍM V: CHUYỂN GÓC NHÌN ---
    if (event.GetEventType() == Aether::EventType::KeyPressed)
    {
        if (Aether::Input::IsKeyPressed(Aether::Key::V))
        {
            m_FirstPerson = !m_FirstPerson;
            if (m_FirstPerson) {
                pTransform.Scale = {0.0f, 0.0f, 0.0f};
                m_Camera.SetDistance(0.5f);
            } else {
                pTransform.Scale = {1.0f, 1.0f, 1.0f};
                m_Camera.SetDistance(5.0f);
            }
            pTransform.Dirty = true;
            event.Handled    = true;
            return;
        }
    }

    // --- CUỘN CHUỘT: VÀO/THOÁT 1ST PERSON ---
    if (event.GetEventType() == Aether::EventType::MouseScrolled)
    {
        auto& e = (Aether::MouseScrolledEvent&)event;
        float currentDist = m_Camera.GetDistance();

        if (!m_FirstPerson)
        {
            if (e.GetYOffset() > 0 && currentDist < 1.3f)
            {
                m_FirstPerson = true;
                m_Camera.SetDistance(0.5f);
                event.Handled = true;
                return;
            }
            if (m_LockCamera) { event.Handled = true; return; }
        }
        else
        {
            if (e.GetYOffset() < 0)
            {
                m_FirstPerson = false;
                m_Camera.SetDistance(5.0f);
                event.Handled = true;
                return;
            }
            event.Handled = true;
            return;
        }
    }

    // --- BẮN SÚNG ---
    if (event.GetEventType() == Aether::EventType::MouseButtonPressed)
    {
        if (Aether::Input::IsMouseButtonPressed(Aether::Mouse::Button0))
        {
            AE_INFO("shoot!");
            auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
            rigSystem->Play(m_ShootAnimation);
            event.Handled = true;
        }
    }

    if (!event.Handled)
        m_Camera.OnEvent(event);
}