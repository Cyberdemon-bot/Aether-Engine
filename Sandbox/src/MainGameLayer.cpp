#include "MainGameLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>
#include <set>
#include <imgui.h>

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(6.0f);
}

void MainGameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    Aether::Renderer::SetLutMap(Aether::Texture2D::Create("assets/textures/LUT.png", Aether::WrapMode::CLAMP_TO_EDGE, false));
    Aether::Renderer::SetSkyBox(Aether::TextureCube::Create("assets/textures/skybox.png"));

    // --- SHADOW PASS ---
    Aether::FramebufferSpec shadowFbSpec;
    shadowFbSpec.Width = 1024;
    shadowFbSpec.Height = 1024;
    shadowFbSpec.Attachments = { Aether::ImageFormat::DEPTH24STENCIL8 };

    m_ShadowShader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    m_ShadowShader->Bind();
    m_ShadowShader->SetUBOSlot("Bones", 1);
    m_ShadowShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass shadowPass;
    shadowPass.TargetFBO = Aether::FrameBuffer::Create(shadowFbSpec);
    shadowPass.Shader = m_ShadowShader;
    shadowPass.ClearDepth = true;
    shadowPass.ClearColor = false;
    shadowPass.OnScreen = false;
    shadowPass.UsingMaterial = false;
    shadowPass.CullFace = Aether::State::FRONT_CULL;
    shadowPass.attribList = { {"u_LightIndex", 0} };

    // --- MAIN PASS ---
    auto& window = Aether::Application::Get().GetWindow();
    Aether::FramebufferSpec sceneFbSpec;
    sceneFbSpec.Width = window.GetWidth();
    sceneFbSpec.Height = window.GetHeight();
    sceneFbSpec.Attachments = { Aether::ImageFormat::RGBA8, Aether::ImageFormat::DEPTH24STENCIL8 };

    m_MainShader = Aether::Shader::Create("assets/shaders/Standard.shader");
    m_MainShader->Bind();
    m_MainShader->SetUBOSlot("Camera", 0);
    m_MainShader->SetUBOSlot("Bones", 1);
    m_MainShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass mainPass;
    mainPass.TargetFBO = Aether::FrameBuffer::Create(sceneFbSpec);
    mainPass.Shader = m_MainShader;
    mainPass.ClearColor = true;
    mainPass.ClearDepth = true;
    mainPass.UsingSkybox = true;
    mainPass.ClearValue = glm::vec4(0.5f, 0.7f, 1.0f, 1.0f);
    mainPass.CullFace = Aether::State::BACK_CULL;
    mainPass.OnScreen = true;
    mainPass.readList = { {"u_DepthTex", shadowPass.TargetFBO->GetDepthAttachment()} };
    mainPass.attribList = { {"u_LightIndex", 0} };
    mainPass.LutIntensity = 0.1f;

    m_Pipeline = { shadowPass, mainPass };
    Aether::Renderer::SetPipeline(m_Pipeline);

    // --- SUN LIGHT ---
    m_SunLight = m_Scene.CreateEntity("Sun Light");
    auto& lightComp = m_Scene.AddComponent<Aether::LightComponent>(m_SunLight);
    lightComp.Config.type = Aether::LightType::Directional;
    lightComp.Config.color = glm::vec3(0.9f, 0.95f, 1.0f);
    lightComp.Config.intensity = 1.5f;
    lightComp.Config.castShadows = true;
    lightComp.Config.direction = glm::vec3(-0.5f, -1.0f, -0.5f);

    auto& sunTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
    sunTransform.Rotation = glm::quat(glm::vec3(glm::radians(-45.0f), glm::radians(30.0f), 0.0f));
    sunTransform.Translation = glm::vec3(0.0f, 50.0f, 0.0f);
    sunTransform.Dirty = true;

    // --- MAP ---
    auto uploadMap = Aether::Importer::Upload(Aether::Importer::Import("assets/models/map.glb"));
    if (!uploadMap.meshIDs.empty()) {
        m_BaseMapMesh = Aether::AssetsManager::GetResource<Aether::Mesh>(uploadMap.meshIDs[0]);
        if (uploadMap.matIDs.empty()) AE_ERROR("no material!");
        for (auto& matID : uploadMap.matIDs)
            m_BaseMapMaterials.push_back(Aether::AssetsManager::GetResource<Aether::Material>(matID));
    }

    // --- PLAYER ---
    m_Player = m_Scene.CreateEntity("Player");
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
    pTransform.Translation = { 0.0f, yFloor, 0.0f };
    pTransform.Scale = { 1.0f,  1.0f,  1.0f };
    pTransform.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    pTransform.Dirty = true;

    auto uploadPlayer = Aether::Importer::Upload(Aether::Importer::Import("assets/models/humanv2.glb"));
    m_Scene.LoadHierarchy(uploadPlayer, m_Player);

    if (!uploadPlayer.animatorIDS.empty())
        m_RunAnimation = uploadPlayer.animatorIDS[0];

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    {
        auto clips = rigSystem->GetClips(m_RunAnimation);
        if (!clips.empty()) rigSystem->BindClip(m_RunAnimation, clips[0]);
    }

    // --- PLAYER PHYSICS (Kinematic capsule) ---
    Aether::UUID bodyID = m_Scene.GetComponent<Aether::IDComponent>(m_Player).ID;
    {
        Aether::BodyConfig cfg;
        cfg.motionType = Aether::MotionType::Kinematic;
        cfg.shape = Aether::ColliderShape::Capsule;
        cfg.size = glm::vec3(0.35f, 2.0f, 0.0f); // radius, halfHeight
        cfg.transform = { pTransform.Translation, glm::quat(1,0,0,0) };
        cfg.offset = glm::vec3(0.0f, 1.0f, 0.0f);
        cfg.friction = 0.5f;
        cfg.restitution = 0.0f;
        Aether::PhysicsSystem::CreateBody(bodyID, cfg);
        m_PlayerBodyID = bodyID;
        m_Scene.AddComponent<Aether::ColliderComponent>(m_Player, bodyID, true);
    }

    m_ZombieSceneData = Aether::Importer::Upload(Aether::Importer::Import("assets/models/zombie.glb"));
    if (!m_ZombieSceneData.animatorIDS.empty())
        m_ZombieRunAnimation = m_ZombieSceneData.animatorIDS[0];

    // --- GUN ---
    m_Gun = m_Scene.CreateEntity("Weapon_Gun");
    auto& gTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);
    gTransform.Translation = { 0.0f, 0.0f, 0.0f };
    gTransform.Scale = { 1.0f, 1.0f, 1.0f };
    gTransform.Dirty = true;

    auto uploadGun = Aether::Importer::Upload(Aether::Importer::Import("assets/models/gun.glb"));
    m_Scene.LoadHierarchy(uploadGun, m_Gun);

    if (!uploadGun.animatorIDS.empty()) {
        m_ShootAnimation = uploadGun.animatorIDS[0];
        auto clips = rigSystem->GetClips(m_ShootAnimation);
        AE_INFO("animator id: {0}, clips num: {1}", uint64_t(m_ShootAnimation), clips.size());
        if (!clips.empty()) rigSystem->BindClip(m_ShootAnimation, clips[0]);
        rigSystem->SetLoop(m_ShootAnimation, false);
    }

    m_PathGridSize = (m_ChunkSize * 1.0f) / static_cast<float>(m_FlowFieldSubdivisions);

    m_MuzzleFlashTexture = Aether::Texture2D::Create("assets/models/tiadan.png");

    Aether::PhysicsSystem::SetGravity({ 0.0f, 0.0f, 0.0f });

    Aether::UUID bgmID;
    Aether::AudioSystem::CreateSource(bgmID, Aether::AudioType::Audio2D, "assets/audio/Hatsune Miku - Ievan Polkka.mp3");
    Aether::AudioSystem::Play(bgmID);

    AE_CORE_INFO("MainGameLayer started.");
}

void MainGameLayer::Detach()
{
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    for (auto& [entity, record] : m_ZombieRegistry) {
        if (rigSystem) rigSystem->DestroyAnimator(record.animatorID);
        Aether::PhysicsSystem::DestroyBody(record.bodyID);
        if (m_Scene.IsValid(entity)) DestroyHierarchy(entity);
    }
    m_ZombieRegistry.clear();
    m_ActiveZombies.clear();

    if (m_PlayerBodyID != 0)
        Aether::PhysicsSystem::DestroyBody(m_PlayerBodyID);

    m_ShadowShader.reset();
    m_MainShader.reset();
    m_ActiveChunks.clear();
}

void MainGameLayer::Update(Aether::Timestep ts)
{
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    // STEP 1: Update camera first to get latest mouse-driven orientation
    m_Camera.Update(ts);

    float camDistance = m_Camera.GetDistance();
    m_CurrentRenderDistance = m_BaseRenderDistance + static_cast<int>(camDistance / m_ZoomInfluence);
    m_CurrentRenderDistance = std::clamp(m_CurrentRenderDistance, 1, 30);

    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

        // STEP 2: Movement relative to camera orientation
        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight = m_Camera.GetRightDirection();
        static float s_HeadBobTimer = 0.0f;
        static float s_BobAmplitudeBlend = 0.0f;

        camForward.y = 0.0f; camRight.y = 0.0f;
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight) > 0.0f) camRight = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);
        if (Aether::Input::IsKeyPressed(Aether::Key::W)) moveDir += camForward;
        if (Aether::Input::IsKeyPressed(Aether::Key::S)) moveDir -= camForward;
        if (Aether::Input::IsKeyPressed(Aether::Key::A)) moveDir -= camRight;
        if (Aether::Input::IsKeyPressed(Aether::Key::D)) moveDir += camRight;

        bool isMoving = glm::length(moveDir) > 0.0f;
        if (isMoving != m_IsPlayerMoving) {
            if (isMoving) rigSystem->Play(m_RunAnimation);
            else          rigSystem->Pause(m_RunAnimation);
            m_IsPlayerMoving = isMoving;
        }

        if (isMoving)
        {
            moveDir = glm::normalize(moveDir);
            glm::vec3 newPlayerPos = pTransform.Translation + moveDir * (m_PlayerSpeed * (float)ts);
            Aether::PhysTransform playerTarget{ newPlayerPos, pTransform.Rotation };
            if (Aether::PhysicsSystem::CanMove(m_PlayerBodyID, playerTarget))
                pTransform.Translation = newPlayerPos;

            if (!m_FirstPerson) {
                float targetAngle = glm::atan(moveDir.x, moveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(pTransform.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                float blend = 1.0f - glm::exp(-15.0f * (float)ts);
                pTransform.Rotation = glm::normalize(glm::slerp(pTransform.Rotation, targetRot, blend));
            }
            pTransform.Dirty = true;

            s_HeadBobTimer += (float)ts * m_bobSpeed;
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 1.0f, (float)ts * 10.0f);
        }
        else
        {
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 0.0f, (float)ts * 10.0f);
            if (s_BobAmplitudeBlend < 0.01f) {
                s_BobAmplitudeBlend = 0.0f;
                s_HeadBobTimer = 0.0f;
            }
        }

        float targetAmplitude = m_FirstPerson ? m_bobStrength : m_bobStrength / 2.0f;
        float bobOffsetY = glm::abs(glm::sin(s_HeadBobTimer)) * targetAmplitude * s_BobAmplitudeBlend;

        // STEP 3: Sync camera with updated player position
        glm::vec3 playerTopPos = pTransform.Translation + glm::vec3(0.0f, 1.0f + bobOffsetY, 0.0f);
        glm::vec3 playerEyePos = pTransform.Translation + glm::vec3(0.0f, 1.7f + bobOffsetY, 0.0f);

        if (m_FirstPerson)
        {
            pTransform.Scale = { 0.001f, 0.001f, 0.001f };
            m_Camera.SetDistance(0.5f);
            m_Camera.SetFocalPoint(playerEyePos);
            pTransform.Rotation = glm::quat(glm::vec3(0.0f, -m_Camera.GetYaw(), 0.0f));
            pTransform.Dirty = true;
        }
        else
        {
            pTransform.Scale = { 1.0f, 1.0f, 1.0f };
            glm::vec3 shoulderOffset = m_Camera.GetRightDirection() * 0.5f;
            m_Camera.SetFocalPoint(playerTopPos + shoulderOffset);

            if (m_LockCamera) {
                m_Camera.SetDistance(5.0f);
                if (m_Camera.GetPitch() < 0.2f) m_Camera.SetPitch(0.2f);
            }
        }

        // --- RELOAD LOGIC ---
        if (Aether::Input::IsKeyPressed(Aether::Key::R) && !m_IsReloading && m_CurrentAmmo < m_MaxAmmo) {
            m_IsReloading = true;
            m_ReloadTimer = m_ReloadDuration;
            AE_INFO("Reloading...");
        }

        if (m_IsReloading) {
            m_ReloadTimer -= (float)ts;
            m_ReloadRotation += (float)ts * 7.0f;
            if (m_ReloadTimer <= 0.0f) {
                m_CurrentAmmo = m_MaxAmmo;
                m_IsReloading = false;
                AE_INFO("Reload complete.");
            }
        }

        // STEP 4: Keep sun light tracking player for stable shadows
        if (m_Scene.IsValid(m_SunLight)) {
            auto& lightTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
            lightTransform.Translation = playerTopPos + glm::vec3(0.0f, 50.0f, 0.0f);
            lightTransform.Dirty = true;
            m_Scene.GetComponent<Aether::LightComponent>(m_SunLight).Config.castShadows = true;
        }

        UpdateMapChunks(pTransform.Translation);

        m_FlowFieldTimer += (float)ts;
        if (m_FlowFieldTimer >= 0.2f) {
            UpdateFlowField(pTransform.Translation);
            m_FlowFieldTimer = 0.0f;
        }

        // =========================================================
        // ZOMBIE POPULATION MANAGEMENT
        // =========================================================
        static float s_TimeAccumulator = 0.0f;
        s_TimeAccumulator += (float)ts;

        const float actualChunkSize = m_ChunkSize * 1.0f;
        const float despawnRadius = (m_CurrentRenderDistance * actualChunkSize) + (actualChunkSize * 1.5f);
        const float despawnRadiusSq = despawnRadius * despawnRadius;

        // 1. Despawn out-of-range zombies
        for (auto it = m_ActiveZombies.begin(); it != m_ActiveZombies.end(); )
        {
            Aether::Entity zombie = *it;
            if (!m_Scene.IsValid(zombie)) { it = m_ActiveZombies.erase(it); continue; }

            auto& zT = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            glm::vec3 diff = pTransform.Translation - zT.Translation;
            diff.y = 0.0f;

            if (glm::dot(diff, diff) > despawnRadiusSq)
            {
                auto& rec = m_ZombieRegistry[zombie];
                rigSystem->DestroyAnimator(rec.animatorID);
                Aether::PhysicsSystem::DestroyBody(rec.bodyID);
                m_ZombieRegistry.erase(zombie);
                DestroyHierarchy(zombie);
                it = m_ActiveZombies.erase(it);
            }
            else { ++it; }
        }

        // 2. Ambient spawn at fog edge (once per second, cap at 50)
        static float s_SpawnTimer = 0.0f;
        s_SpawnTimer += (float)ts;
        if (s_SpawnTimer >= 1.0f) {
            s_SpawnTimer = 0.0f;
            if (m_ActiveZombies.size() < maxZombies) {
                float randomAngle = glm::radians((float)(std::rand() % 360));
                float spawnDist = (m_CurrentRenderDistance * actualChunkSize);
                glm::vec3 spawnPos = pTransform.Translation
                    + glm::vec3(glm::cos(randomAngle), 0.0f, glm::sin(randomAngle)) * spawnDist;
                spawnPos.y = yFloor;
                SpawnZombie(spawnPos);
            }
        }

        // 3. Staggered zombie AI update (alternating halves each frame)
        static uint32_t s_ZombieUpdateCounter = 0;
        s_ZombieUpdateCounter++;
        int zombieIndex = 0;

        for (Aether::Entity zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;
            zombieIndex++;
            if (zombieIndex % 2 != s_ZombieUpdateCounter % 2) continue;

            auto& zT = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            uint32_t zSeed = (uint32_t)zombie;

            int zX = static_cast<int>(std::floor(zT.Translation.x / m_PathGridSize));
            int zZ = static_cast<int>(std::floor(zT.Translation.z / m_PathGridSize));
            auto zCoord = std::make_pair(zX, zZ);

            // Base direction from flow field, fallback to direct line to player
            glm::vec3 baseDir(0.0f, 0.0f, 1.0f);
            auto flowIt = m_FlowField.find(zCoord);
            if (flowIt != m_FlowField.end() && glm::length(flowIt->second.direction) > 0.0000001f) {
                baseDir = flowIt->second.direction;
            }
            else {
                glm::vec3 diffBase = pTransform.Translation - zT.Translation;
                diffBase.y = 0.0f;
                if (glm::length(diffBase) > 0.001f) baseDir = glm::normalize(diffBase);
            }

            // Wobble perpendicular to movement
            glm::vec3 rightDir = glm::vec3(-baseDir.z, 0.0f, baseDir.x);
            float     wobble = glm::sin(s_TimeAccumulator * 2.5f + zSeed) * 0.35f;

            // Separation force to avoid stacking
            glm::vec3 separationForce(0.0f);
            const float sepRadiusSq = 0.64f; // 0.8^2
            int neighborCount = 0;
            for (Aether::Entity other : m_ActiveZombies) {
                if (other == zombie || !m_Scene.IsValid(other)) continue;
                auto& otherT = m_Scene.GetComponent<Aether::TransformComponent>(other);
                glm::vec3 diff = zT.Translation - otherT.Translation;
                diff.y = 0.0f;
                float distSq = glm::dot(diff, diff);
                if (distSq > 0.001f && distSq < sepRadiusSq) {
                    float dist = glm::sqrt(distSq);
                    separationForce += (diff / dist) * (0.8f - dist);
                    neighborCount++;
                }
            }
            if (neighborCount > 0) separationForce /= (float)neighborCount;

            // Combine forces into final move direction
            glm::vec3 totalForce = baseDir + rightDir * wobble + separationForce * 0.5f;
            glm::vec3 finalMoveDir = (glm::length(totalForce) > 0.001f) ? glm::normalize(totalForce) : baseDir;

            // Per-zombie speed variation (80%–120%)
            float speedMod = 0.8f + ((zSeed % 100) / 100.0f) * 0.4f;
            float actualSpeed = m_ZombieSpeed * speedMod;

            glm::vec3 diffToPlayer = pTransform.Translation - zT.Translation;
            diffToPlayer.y = 0.0f;

            if (glm::length(diffToPlayer) > 1.2f)
            {
                float     targetAngle = glm::atan(finalMoveDir.x, finalMoveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(zT.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                zT.Rotation = glm::normalize(glm::slerp(zT.Rotation, targetRot, 1.0f - glm::exp(-5.0f * (float)ts)));

                // Lock facing to ground plane to prevent vertical drift
                glm::vec3 facing = zT.Rotation * glm::vec3(0.0f, 0.0f, 1.0f);
                facing.y = 0.0f;
                if (glm::length(facing) > 0.001f) {
                    glm::vec3 newZombiePos = zT.Translation + glm::normalize(facing) * (actualSpeed * (float)ts);
                    newZombiePos.y = yFloor;
                    Aether::PhysTransform zombieTarget{ newZombiePos, zT.Rotation };
                    auto& zRec = m_ZombieRegistry[zombie];
                    if (Aether::PhysicsSystem::CanMove(zRec.bodyID, zombieTarget))
                        zT.Translation = newZombiePos;
                    else
                        zT.Translation.y = yFloor; // keep floor-locked even when blocked
                }
                zT.Dirty = true;
            }
        }
    }

    // --- SHADER UNIFORMS ---
    m_MainShader->Bind();
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);
    m_MainShader->SetInt("u_FogMode", m_FogMode);
    m_MainShader->SetFloat3("u_FogColor", m_FogColor);
    m_MainShader->SetFloat("u_FogDensity", m_FogDensity);
    m_MainShader->SetFloat("u_FogStart", m_FogStart);
    m_MainShader->SetFloat("u_FogEnd", m_FogEnd);

    // --- GUN POSITIONING ---
    if (m_Scene.IsValid(m_Gun) && m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        auto& gTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);

        if (m_FirstPerson)
        {
            glm::vec3 camPos = m_Camera.GetPosition();
            glm::vec3 forward = m_Camera.GetForwardDirection();
            glm::vec3 right = m_Camera.GetRightDirection();
            glm::vec3 up = m_Camera.GetUpDirection();

            gTransform.Translation = camPos + (right * m_GunPosFP.x) + (up * m_GunPosFP.y) + (forward * m_GunPosFP.z);
            glm::quat camQuat = glm::quat(glm::vec3(-m_Camera.GetPitch(), -m_Camera.GetYaw(), 0.0f));
            gTransform.Rotation = camQuat * glm::quat(glm::radians(m_GunRotFP));
            gTransform.Scale = m_GunScaleFP;
        }
        else
        {
            glm::quat pRot = pTransform.Rotation;
            glm::vec3 forward = pRot * glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 right = pRot * glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 up = pRot * glm::vec3(0.0f, 1.0f, 0.0f);

            gTransform.Translation = pTransform.Translation
                + (right * m_GunPosTP.x) + (up * m_GunPosTP.y) + (forward * m_GunPosTP.z);
            gTransform.Rotation = pRot * glm::quat(glm::radians(m_GunRotTP));
            gTransform.Scale = m_GunScaleTP;
        }
        gTransform.Dirty = true;
    }

    m_Scene.Update(ts, &m_Camera);
}

void MainGameLayer::UpdateMapChunks(const glm::vec3& playerPos)
{
    if (!m_BaseMapMesh) return;

    const float actualChunkSize = m_ChunkSize * 1.0f;
    int centerX = static_cast<int>(std::floor(playerPos.x / actualChunkSize));
    int centerZ = static_cast<int>(std::floor(playerPos.z / actualChunkSize));

    std::set<std::pair<int, int>> chunksToKeep;

    // Create new chunks in range
    for (int x = -m_CurrentRenderDistance; x <= m_CurrentRenderDistance; ++x) {
        for (int z = -m_CurrentRenderDistance; z <= m_CurrentRenderDistance; ++z) {
            auto coord = std::make_pair(centerX + x, centerZ + z);
            chunksToKeep.insert(coord);

            if (m_ActiveChunks.count(coord)) continue;

            Aether::Entity chunk = m_Scene.CreateEntity(
                "MapGrid_" + std::to_string(coord.first) + "_" + std::to_string(coord.second));
            auto& t = m_Scene.GetComponent<Aether::TransformComponent>(chunk);
            t.Translation = glm::vec3((coord.first + 0.5f) * actualChunkSize, -(actualChunkSize / 2.0f), (coord.second + 0.5f) * actualChunkSize);

            // Random 90-degree snap rotation on Y axis
            int randomRot = std::rand() % 4; // 0, 1, 2, 3
            float rotAngle = glm::radians(randomRot * 90.0f);
            t.Rotation = glm::quat(glm::vec3(0.0f, rotAngle, 0.0f));

            t.Dirty = true;

            auto& mesh = m_Scene.AddComponent<Aether::MeshComponent>(chunk);
            mesh.MeshPtr = m_BaseMapMesh;
            mesh.Materials = m_BaseMapMaterials;

            ChunkData newData;
            newData.landEntity = chunk;
            newData.rotation = randomRot;
            if (std::rand() % 100 < 80 && (std::abs(x) > 2 || std::abs(z) > 2)) {
                glm::vec3 spawnPos = t.Translation;
                spawnPos.y = yFloor;
                Aether::Entity zEnt = SpawnZombie(spawnPos);
                if (zEnt != Aether::Null_Entity) newData.zombies.push_back(zEnt);
            }

            m_ActiveChunks[coord] = newData;
        }
    }

    // Remove chunks that left range
    for (auto it = m_ActiveChunks.begin(); it != m_ActiveChunks.end(); ) {
        if (chunksToKeep.count(it->first)) { ++it; continue; }

        for (Aether::Entity zombie : it->second.zombies) {
            if (!m_Scene.IsValid(zombie)) continue;
            auto regIt = m_ZombieRegistry.find(zombie);
            if (regIt != m_ZombieRegistry.end()) {
                Aether::PhysicsSystem::DestroyBody(regIt->second.bodyID);
                m_ZombieRegistry.erase(regIt);
            }
            m_ActiveZombies.erase(std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), zombie), m_ActiveZombies.end());
            DestroyHierarchy(zombie);
        }

        if (m_Scene.IsValid(it->second.landEntity))
            m_Scene.DestroyEntity(it->second.landEntity);

        it = m_ActiveChunks.erase(it);
    }
}

Aether::Entity MainGameLayer::SpawnZombie(const glm::vec3& position)
{
    if (m_ActiveZombies.size() >= maxZombies) return Aether::Null_Entity;

    static uint32_t s_ZombieCounter = 0;
    s_ZombieCounter++;

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    // Clone animator for this zombie instance
    Aether::UUID newAnimID = Aether::AssetsRegister::Register("ZombieAnim_" + std::to_string(s_ZombieCounter));
    if (rigSystem) {
        rigSystem->CloneAnimator(newAnimID, m_ZombieRunAnimation);
        rigSystem->BindClip(newAnimID, 4);
        rigSystem->Play(newAnimID);
    }

    // Temporarily swap animator ID so LoadHierarchy binds the new clone
    Aether::UUID originalAnimID = m_ZombieSceneData.animatorIDS[0];
    m_ZombieSceneData.animatorIDS[0] = newAnimID;

    Aether::Entity newZombie = m_Scene.CreateEntity("Zombie_Minion");
    auto& zTransform = m_Scene.GetComponent<Aether::TransformComponent>(newZombie);
    zTransform.Translation = position;
    zTransform.Scale = { 1.0f, 1.0f, 1.0f };
    zTransform.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    zTransform.Dirty = true;

    m_Scene.LoadHierarchy(m_ZombieSceneData, newZombie);
    m_ZombieSceneData.animatorIDS[0] = originalAnimID;

    // Physics body
    Aether::UUID bodyID = m_Scene.GetComponent<Aether::IDComponent>(newZombie).ID;
    {
        Aether::BodyConfig cfg;
        cfg.motionType = Aether::MotionType::Kinematic;
        cfg.shape = Aether::ColliderShape::Capsule;
        cfg.size = glm::vec3(0.35f, 2.0f, 0.0f);
        cfg.transform = { position, glm::quat(1,0,0,0) };
        cfg.offset = glm::vec3(0.0f, 1.0f, 0.0f);
        Aether::PhysicsSystem::CreateBody(bodyID, cfg);
        m_Scene.AddComponent<Aether::ColliderComponent>(newZombie, bodyID, true);
    }

    m_ZombieRegistry[newZombie] = { newAnimID, bodyID };
    m_ActiveZombies.push_back(newZombie);
    return newZombie;
}

int MainGameLayer::GetObstacleCost(int coordX, int coordZ) const
{
    int s = k_ObstacleMapSize;

    // Which chunk does this cell belong to
    int chunkX = (int)std::floor((float)coordX / s);
    int chunkZ = (int)std::floor((float)coordZ / s);

    // Local index within the chunk [0, s)
    int localX = ((coordX % s) + s) % s;
    int localZ = ((coordZ % s) + s) % s;

    // Look up this chunk's rotation (0-3)
    int rot = 0;
    auto it = m_ActiveChunks.find({ chunkX, chunkZ });
    if (it != m_ActiveChunks.end())
        rot = it->second.rotation;

    // Rotate local coords to match chunk mesh rotation
    int rx = localX, rz = localZ;
    switch (rot) {
    case 1: rx = s - 1 - localZ; rz = localX;     break; // 90
    case 2: rx = s - 1 - localX; rz = s - 1 - localZ; break; // 180
    case 3: rx = localZ;     rz = s - 1 - localX; break; // 270
    default: break;                                    // 0 
    }

    return (m_ObstacleMap[rz][rx] == 1) ? 255 : 1;
}

void MainGameLayer::UpdateFlowField(const glm::vec3& targetPos)
{
    // Reset costs and directions
    for (auto& [coord, cell] : m_FlowField) {
        cell.bestCost = 999999;
        cell.direction = glm::vec3(0.0f);
        cell.cost = GetObstacleCost(coord.first, coord.second); // re-apply every update
    }

    int targetX = static_cast<int>(std::floor(targetPos.x / m_PathGridSize));
    int targetZ = static_cast<int>(std::floor(targetPos.z / m_PathGridSize));
    auto targetCoord = std::make_pair(targetX, targetZ);

    m_FlowField[targetCoord].bestCost = 0;
    m_FlowField[targetCoord].cost = 1;

    static const std::vector<std::pair<int, int>> neighbors = {
        {0, 1}, {0,-1}, {1, 0}, {-1, 0},
        {1, 1}, {1,-1}, {-1, 1}, {-1,-1}
    };

    const int maxRadius = 40;
    std::queue<std::pair<int, int>> openList;
    openList.push(targetCoord);

    // BFS Dijkstra flood fill
    while (!openList.empty())
    {
        auto current = openList.front(); openList.pop();
        if (std::abs(current.first - targetX) > maxRadius ||
            std::abs(current.second - targetZ) > maxRadius) continue;

        int currCost = m_FlowField[current].bestCost;

        for (auto& [dx, dz] : neighbors)
        {
            auto neighborCoord = std::make_pair(current.first + dx, current.second + dz);

            if (!m_FlowField.count(neighborCoord)) {
                int obsCost = GetObstacleCost(neighborCoord.first, neighborCoord.second);
                m_FlowField[neighborCoord] = { obsCost, 999999, glm::vec3(0.0f) };
            }

            auto& neighbor = m_FlowField[neighborCoord];
            if (neighbor.cost >= 255) continue;

            int moveCost = (dx != 0 && dz != 0) ? 14 : 10;
            int newCost = currCost + (moveCost * neighbor.cost);

            if (newCost < neighbor.bestCost) {
                neighbor.bestCost = newCost;
                openList.push(neighborCoord);
            }
        }
    }

    // Build smooth gradient direction vectors
    for (auto& [coord, cell] : m_FlowField)
    {
        if (cell.cost >= 255 || cell.bestCost == 999999) continue;

        glm::vec3 avgDir(0.0f);
        for (auto& [dx, dz] : neighbors)
        {
            auto neighborCoord = std::make_pair(coord.first + dx, coord.second + dz);
            auto it = m_FlowField.find(neighborCoord);
            if (it == m_FlowField.end()) continue;

            int neighborCost = it->second.bestCost;
            if (neighborCost < cell.bestCost) {
                float pullStrength = float(cell.bestCost - neighborCost);
                avgDir += glm::normalize(glm::vec3((float)dx, 0.0f, (float)dz)) * pullStrength;
            }
        }

        cell.direction = (glm::length(avgDir) > 0.01f) ? glm::normalize(avgDir) : glm::vec3(0.0f);
    }
}

void MainGameLayer::OnImGuiRender()
{
    ImGui::Begin("Game Controls");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Controls: W A S D (Move relative to camera)");
    ImGui::Separator();

    if (ImGui::Checkbox("Lock Camera (Play Mode)", &m_LockCamera)) {
        if (m_LockCamera) AE_CORE_INFO("Camera Locked: Play Mode");
        else              AE_CORE_INFO("Camera Unlocked: Editor Mode");
    }

    if (m_LockCamera)
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Status: Locked (No Zoom)");
    else
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Free (Editor Mode)");

    if (ImGui::CollapsingHeader("Dynamic Map Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Player Speed", &m_PlayerSpeed, 5.0f, 50.0f);
        ImGui::Separator();
        ImGui::Text("--- Camera Zoom Logic ---");
        ImGui::SliderInt("Base Render Distance", &m_BaseRenderDistance, 1, 30);
        ImGui::SliderFloat("Zoom -> Map Ratio", &m_ZoomInfluence, 5.0f, 50.0f, "%.1f");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Camera zoom: %.1f", m_Camera.GetDistance());
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "-> Render radius: %d", m_CurrentRenderDistance);
        ImGui::Text("Chunks: %d  |  Zombies: %d", (int)m_ActiveChunks.size(), (int)m_ActiveZombies.size());
        ImGui::SliderInt("Zombie cap", &maxZombies, 1, 200);
        ImGui::Separator();
        ImGui::Checkbox("Show Flow Field Debug", &m_ShowFlowFieldDebug);
    }
    ImGui::End();

    // --- PLAYER ---
    if (ImGui::Begin("Player Setup")) {
        if (m_Scene.IsValid(m_Player)) {
            auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
            if (ImGui::DragFloat3("Position", glm::value_ptr(pTransform.Translation), 0.01f)) pTransform.Dirty = true;
            if (ImGui::DragFloat3("Scale", glm::value_ptr(pTransform.Scale), 0.01f)) pTransform.Dirty = true;
        }
    }
    ImGui::End();

    // --- GUN ---
    if (ImGui::Begin("Weapon Setup (Gun)")) {
        ImGui::Text("Tune gun placement per perspective.");
        ImGui::Separator();
        if (m_FirstPerson) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "--- 1ST PERSON ---");
            ImGui::DragFloat3("FP Position", glm::value_ptr(m_GunPosFP), 0.01f);
            ImGui::DragFloat3("FP Rotation", glm::value_ptr(m_GunRotFP), 1.0f);
            ImGui::DragFloat3("FP Scale", glm::value_ptr(m_GunScaleFP), 0.01f);
        }
        else {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "--- 3RD PERSON ---");
            ImGui::DragFloat3("TP Position", glm::value_ptr(m_GunPosTP), 0.01f);
            ImGui::DragFloat3("TP Rotation", glm::value_ptr(m_GunRotTP), 1.0f);
            ImGui::DragFloat3("TP Scale", glm::value_ptr(m_GunScaleTP), 0.01f);
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Note: record these values when done tuning.");
        ImGui::Text("Press V or scroll to switch perspective.");
    }
    ImGui::End();

    ImGui::Begin("Weapon Adjustment");
    ImGui::DragFloat3("Muzzle Offset", glm::value_ptr(m_MuzzleOffset), 0.01f);
    ImGui::End();

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawLightingPanel();

    // --- AMMO HUD (bottom-right) ---
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 hudPos = ImVec2(viewport->Pos.x + viewport->Size.x - 180,
        viewport->Pos.y + viewport->Size.y - 100);
    ImGui::SetNextWindowPos(hudPos);
    ImGui::Begin("AmmoDisplay", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "WEAPON: AGENTIC PISTOL");
    if (m_IsReloading) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "RELOADING...");
    }
    else {
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("%d / INF", m_CurrentAmmo);
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();

    // --- CROSSHAIR ---
    ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f,
        viewport->Pos.y + viewport->Size.y * 0.5f);
    auto drawList = ImGui::GetForegroundDrawList();
    const float thickness = 2.0f;
    const ImU32 green = IM_COL32(0, 255, 0, 255);
    const ImU32 white = IM_COL32(255, 255, 255, 255);

    if (m_IsReloading)
    {
        const float radius = 15.0f;
        const int   numSegments = 8;
        for (int i = 0; i < numSegments; i++) {
            float angle = m_ReloadRotation + (i * (2.0f * glm::pi<float>() / numSegments));
            ImVec2 p1 = { center.x + glm::cos(angle) * (radius - 5), center.y + glm::sin(angle) * (radius - 5) };
            ImVec2 p2 = { center.x + glm::cos(angle) * radius,      center.y + glm::sin(angle) * radius };
            drawList->AddLine(p1, p2, white, thickness);
        }
        drawList->AddCircleFilled(center, 1.5f, IM_COL32(255, 0, 0, 150));
    }
    else
    {
        static float crosshairSpread = 0.0f;
        float target = m_IsPlayerMoving ? 12.0f : 0.0f;
        crosshairSpread = glm::mix(crosshairSpread, target, 0.1f);

        const float baseLength = 10.0f;
        const float offset = 5.0f + crosshairSpread;

        drawList->AddLine({ center.x - offset - baseLength, center.y }, { center.x - offset, center.y }, green, thickness);
        drawList->AddLine({ center.x + offset, center.y }, { center.x + offset + baseLength, center.y }, green, thickness);
        drawList->AddLine({ center.x, center.y - offset - baseLength }, { center.x, center.y - offset }, green, thickness);
        drawList->AddLine({ center.x, center.y + offset }, { center.x, center.y + offset + baseLength }, green, thickness);
        drawList->AddCircleFilled(center, 1.5f, white);
    }

    // --- FLOW FIELD DEBUG OVERLAY ---
    if (m_ShowFlowFieldDebug && !m_FlowField.empty())
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImDrawList* fgDraw = ImGui::GetForegroundDrawList();
        ImVec2         dispSize = vp->Size;
        ImVec2         dispPos = vp->Pos;

        glm::mat4 viewProj = m_Camera.GetViewProjection();

        const ImU32 colGrid = IM_COL32(0, 255, 0, 80);   // faint green cell borders
        const ImU32 colDir = IM_COL32(0, 255, 0, 200);   // bright green arrows
        const ImU32 colTarget = IM_COL32(255, 255, 0, 255);   // yellow for cost-0 cell
        const float half = m_PathGridSize * 0.5f;

        for (auto& [coord, cell] : m_FlowField)
        {
            if (cell.bestCost == 999999) continue;

            glm::vec3 worldCenter = glm::vec3(
                (coord.first + 0.5f) * m_PathGridSize,
                yFloor + 0.05f,
                (coord.second + 0.5f) * m_PathGridSize
            );

            // Project the four corners of this grid cell to screen space
            glm::vec3 corners[4] = {
                worldCenter + glm::vec3(-half, 0, -half),
                worldCenter + glm::vec3(half, 0, -half),
                worldCenter + glm::vec3(half, 0,  half),
                worldCenter + glm::vec3(-half, 0,  half),
            };

            ImVec2 screenCorners[4];
            bool   allVisible = true;
            for (int i = 0; i < 4; i++) {
                ImVec2 s;
                if (!WorldToScreen(corners[i], viewProj, dispSize, s)) { allVisible = false; break; }
                screenCorners[i] = ImVec2(dispPos.x + s.x, dispPos.y + s.y);
            }

            if (!allVisible) continue;

            // Draw cell quad
            ImU32 borderCol = (cell.bestCost == 0) ? colTarget : colGrid;
            fgDraw->AddQuad(screenCorners[0], screenCorners[1],
                screenCorners[2], screenCorners[3], borderCol, 1.0f);

            // Draw direction arrow (skip the target cell itself)
            if (cell.bestCost > 0 && glm::length(cell.direction) > 0.01f)
            {
                glm::vec3 arrowEnd = worldCenter + cell.direction * (m_PathGridSize * 0.4f);
                ImVec2    sCenter, sEnd;

                if (WorldToScreen(worldCenter, viewProj, dispSize, sCenter) &&
                    WorldToScreen(arrowEnd, viewProj, dispSize, sEnd))
                {
                    sCenter = ImVec2(dispPos.x + sCenter.x, dispPos.y + sCenter.y);
                    sEnd = ImVec2(dispPos.x + sEnd.x, dispPos.y + sEnd.y);

                    fgDraw->AddLine(sCenter, sEnd, colDir, 1.5f);

                    // Arrowhead
                    glm::vec2 lineDir = glm::normalize(glm::vec2(sEnd.x - sCenter.x, sEnd.y - sCenter.y));
                    glm::vec2 perp = glm::vec2(-lineDir.y, lineDir.x);
                    const float headLen = 4.0f;

                    ImVec2 tip = sEnd;
                    ImVec2 lWing = ImVec2(sEnd.x - lineDir.x * headLen + perp.x * headLen * 0.5f,
                        sEnd.y - lineDir.y * headLen + perp.y * headLen * 0.5f);
                    ImVec2 rWing = ImVec2(sEnd.x - lineDir.x * headLen - perp.x * headLen * 0.5f,
                        sEnd.y - lineDir.y * headLen - perp.y * headLen * 0.5f);

                    fgDraw->AddTriangleFilled(tip, lWing, rWing, colDir);
                }
            }
        }
    }

    if (m_ShowFlowFieldDebug && !m_ActiveChunks.empty())
    {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImDrawList* fgDraw = ImGui::GetForegroundDrawList();
        ImVec2       dispSize = vp->Size;
        ImVec2       dispPos = vp->Pos;

        glm::mat4 viewProj = m_Camera.GetViewProjection();
        const float actualChunkSize = m_ChunkSize * 1.0f;
        const float half = actualChunkSize * 0.5f;

        const ImU32 colChunk = IM_COL32(255, 0, 0, 160);  // red border
        const ImU32 colLabel = IM_COL32(255, 80, 80, 255);  // red label

        for (auto& [coord, chunkData] : m_ActiveChunks)
        {
            glm::vec3 worldCenter = glm::vec3(
                (coord.first + 0.5f) * actualChunkSize,
                yFloor + 0.05f,
                (coord.second + 0.5f) * actualChunkSize);

            glm::vec3 corners[4] = {
                worldCenter + glm::vec3(-half, 0.f, -half),
                worldCenter + glm::vec3(half, 0.f, -half),
                worldCenter + glm::vec3(half, 0.f,  half),
                worldCenter + glm::vec3(-half, 0.f,  half),
            };

            ImVec2 screenCorners[4];
            bool   allVisible = true;
            for (int i = 0; i < 4; i++) {
                ImVec2 s;
                if (!WorldToScreen(corners[i], viewProj, dispSize, s)) { allVisible = false; break; }
                screenCorners[i] = ImVec2(dispPos.x + s.x, dispPos.y + s.y);
            }

            if (!allVisible) continue;

            // Draw chunk border
            fgDraw->AddQuad(screenCorners[0], screenCorners[1],
                screenCorners[2], screenCorners[3], colChunk, 2.0f);

            // Draw coord label at chunk center
            ImVec2 sCenter;
            if (WorldToScreen(worldCenter, viewProj, dispSize, sCenter)) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d,%d", coord.first, coord.second);
                fgDraw->AddText(
                    ImVec2(dispPos.x + sCenter.x, dispPos.y + sCenter.y),
                    colLabel, buf
                );
            }
        }
    }
}

void MainGameLayer::DrawEntityNode(Aether::Entity entity)
{
    auto& tag = m_Scene.GetComponent<Aether::TagComponent>(entity);
    auto& hier = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (hier.firstChild == Aether::Null_Entity) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedEntity == entity)             flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", tag.Tag.c_str());
    if (ImGui::IsItemClicked()) m_SelectedEntity = entity;

    if (open) {
        Aether::Entity child = hier.firstChild;
        while (child != Aether::Null_Entity) {
            Aether::Entity next = m_Scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
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
    for (auto entity : view) {
        if (m_Scene.GetComponent<Aether::HierarchyComponent>(entity).parent == Aether::Null_Entity)
            DrawEntityNode(entity);
    }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        m_SelectedEntity = Aether::Null_Entity;

    ImGui::End();
}

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
            if (ImGui::DragFloat4("Rotation", glm::value_ptr(t.Rotation), 0.1f))  t.Dirty = true;
            if (ImGui::DragFloat3("Scale", glm::value_ptr(t.Scale), 0.05f)) t.Dirty = true;
        }
    }
    else {
        ImGui::Text("Select an entity to see properties.");
    }

    ImGui::End();
}

void MainGameLayer::DrawLightingPanel()
{
    if (!ImGui::Begin("Lighting")) { ImGui::End(); return; }

    if (m_Scene.IsValid(m_SunLight)) {
        if (ImGui::CollapsingHeader("Sun Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& light = m_Scene.GetComponent<Aether::LightComponent>(m_SunLight).Config;
            ImGui::ColorEdit3("Sun Color", glm::value_ptr(light.color));
            ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
            ImGui::DragFloat3("Direction", glm::value_ptr(light.direction), 0.01f, -1.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::SliderFloat("Shadow Bias", &m_ShadowBias, 0.00001f, 0.005f, "%.5f");

    if (ImGui::CollapsingHeader("Atmosphere & Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Fog Color", glm::value_ptr(m_FogColor));
        ImGui::SliderFloat("Fog Density", &m_FogDensity, 0.0f, 0.1f);
    }

    if (ImGui::CollapsingHeader("Camera Movement")) {
        ImGui::SliderFloat("Bob Speed", &m_bobSpeed, 5.0f, 20.0f);
        ImGui::SliderFloat("Bob Strength", &m_bobStrength, 0.01f, 0.2f);
    }

    ImGui::End();
}

void MainGameLayer::OnEvent(Aether::Event& event)
{
    m_Camera.OnEvent(event);
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

    // V: toggle perspective
    if (event.GetEventType() == Aether::EventType::KeyPressed &&
        Aether::Input::IsKeyPressed(Aether::Key::V))
    {
        m_FirstPerson = !m_FirstPerson;
        pTransform.Scale = m_FirstPerson ? glm::vec3(0.001f) : glm::vec3(1.0f);
        m_Camera.SetDistance(m_FirstPerson ? 0.5f : 6.0f);
        pTransform.Dirty = true;
        event.Handled = true;
        return;
    }

    // Scroll: transition into/out of first person
    if (event.GetEventType() == Aether::EventType::MouseScrolled)
    {
        auto& e = (Aether::MouseScrolledEvent&)event;
        if (!m_FirstPerson) {
            if (e.GetYOffset() > 0 && m_Camera.GetDistance() < 1.3f) {
                m_FirstPerson = true;
                m_Camera.SetDistance(0.5f);
                event.Handled = true;
                return;
            }
            if (m_LockCamera) { event.Handled = true; return; }
        }
        else {
            if (e.GetYOffset() < 0) {
                m_FirstPerson = false;
                m_Camera.SetDistance(6.0f);
            }
            event.Handled = true;
            return;
        }
    }

    // LMB: shoot
    if (event.GetEventType() == Aether::EventType::MouseButtonPressed &&
        Aether::Input::IsMouseButtonPressed(Aether::Mouse::Button0))
    {
        if (m_IsReloading) { AE_WARN("Cannot shoot while reloading!"); return; }
        if (m_CurrentAmmo <= 0) { AE_WARN("Out of ammo! Press R to reload."); return; }

        m_CurrentAmmo--;

        if (m_Scene.IsValid(m_Gun)) {
            auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
            rigSystem->Stop(m_ShootAnimation);
            rigSystem->Play(m_ShootAnimation);
        }

        glm::vec3 origin = m_Camera.GetPosition();
        glm::vec3 direction = glm::normalize(m_Camera.GetForwardDirection());
        Aether::RaycastHit hit = Aether::PhysicsSystem::CastRay(origin, direction, 100.0f);

        if (hit.Hit) {
            Aether::Entity target = m_Scene.FindEntity(hit.HitEntityID);
            if (target != Aether::Null_Entity && target != m_Player)
            {
                auto& rec = m_ZombieRegistry[target];
                Aether::PhysicsSystem::DestroyBody(rec.bodyID);
                DestroyHierarchy(target);
                m_ActiveZombies.erase(std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), target), m_ActiveZombies.end());
            }
        }

        event.Handled = true;
    }
}

void MainGameLayer::DestroyHierarchy(Aether::Entity entity)
{
    if (!m_Scene.IsValid(entity)) return;

    auto& hierarchy = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);
    Aether::Entity child = hierarchy.firstChild;
    while (child != Aether::Null_Entity) {
        Aether::Entity next = m_Scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
        DestroyHierarchy(child);
        child = next;
    }
    m_Scene.DestroyEntity(entity);
}

bool MainGameLayer::WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProj,
    ImVec2 displaySize, ImVec2& outScreen)
{
    glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.0f) return false;

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.f || ndc.x > 1.f || ndc.y < -1.f || ndc.y > 1.f) return false;

    outScreen.x = (ndc.x * 0.5f + 0.5f) * displaySize.x;
    outScreen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
    return true;
}