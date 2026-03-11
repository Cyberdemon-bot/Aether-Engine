#include "MainGameLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>
#include <set>

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(6.0f);
}

void MainGameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    Aether::Renderer::SetLutMap("assets/textures/LUT.png");
    Aether::Renderer::SetSkyBox("assets/textures/skybox.png");

    // --- SHADOW PASS ---
    Aether::FramebufferSpec shadowFbSpec;
    shadowFbSpec.Width = 1024;
    shadowFbSpec.Height = 1024;
    shadowFbSpec.Attachments = { Aether::ImageFormat::DEPTH24STENCIL8 };

    m_ShadowShader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    m_ShadowShader->Bind();
    m_ShadowShader->SetUBOSlot("Bones", 1);
    m_ShadowShader->SetUBOSlot("Lights", 2);
    m_ShadowFbo = Aether::FrameBuffer::Create(shadowFbSpec);

    Aether::RenderPass shadowPass;
    shadowPass.TargetFBO = m_ShadowFbo.get();
    shadowPass.Shader = m_ShadowShader.get();
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
    m_MainFbo = Aether::FrameBuffer::Create(sceneFbSpec);

    Aether::RenderPass mainPass;
    mainPass.TargetFBO = m_MainFbo.get();
    mainPass.Shader = m_MainShader.get();
    mainPass.ClearColor = true;
    mainPass.ClearDepth = true;
    mainPass.UsingSkybox = true;
    mainPass.ClearValue = glm::vec4(0.5f, 0.7f, 1.0f, 1.0f);
    mainPass.CullFace = Aether::State::BACK_CULL;
    mainPass.OnScreen = true;
    mainPass.readList = { {"u_DepthTex", shadowPass.TargetFBO->GetDepthAttachment()} };
    mainPass.attribList = { {"u_LightIndex", 0} };
    mainPass.LutIntensity = 0.2f;

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
        m_BaseMapMesh = Aether::AssetManager::GetHandle(uploadMap.meshIDs[0]);
        if (uploadMap.matIDs.empty()) AE_ERROR("no material!");
        for (auto& matID : uploadMap.matIDs)
            m_BaseMapMaterials.push_back(Aether::AssetManager::GetHandle(matID));
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
        cfg.size = glm::vec3(0.35f, 2.5f, 0.0f); // radius, halfHeight
        cfg.transform = { pTransform.Translation, glm::quat(1,0,0,0) };
        cfg.offset = glm::vec3(0.0f, 1.0f, 0.0f);
        cfg.friction = 0.5f;
        cfg.restitution = 0.0f;
        Aether::PhysicsSystem::CreateBody(bodyID, cfg);
        m_PlayerBodyID = bodyID;
        m_Scene.AddComponent<Aether::ColliderComponent>(m_Player, bodyID);
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
        if (!clips.empty()) rigSystem->BindClip(m_ShootAnimation, clips[0]);
        rigSystem->SetLoop(m_ShootAnimation, false);
    }

    m_PathGridSize = (m_ChunkSize * 1.0f) / static_cast<float>(m_FlowFieldSubdivisions);

    m_MuzzleFlashTexture = Aether::Texture2D::Create("assets/models/tiadan.png");

    Aether::PhysicsSystem::SetGravity({ 0.0f, 0.0f, 0.0f }); 

    Aether::AssetManager::CreateAsset<Aether::Sound>(m_BgmSoundID,   "assets/audio/Hatsune Miku - Ievan Polkka.mp3");
    Aether::AssetManager::CreateAsset<Aether::Sound>(m_GunSoundID,   "assets/audio/pistol.mp3");
    Aether::AssetManager::CreateAsset<Aether::Sound>(m_GunReloadID,  "assets/audio/pistol_reload.mp3");
    Aether::AssetManager::CreateAsset<Aether::Sound>(m_ZombieBiteID, "assets/audio/zombie_bite.mp3");

    Aether::UUID bgmSrcID;
    Aether::AudioSystem::CreateSource(bgmSrcID, m_BgmSoundID, Aether::AudioType::Audio2D);
    Aether::AudioSystem::SetLooping(bgmSrcID, true);
    Aether::AudioSystem::Play(bgmSrcID);

    AE_INFO("MainGameLayer started.");
}

void MainGameLayer::Detach()
{
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    for (auto& [entity, record] : m_ZombieRegistry) {
        if (rigSystem) rigSystem->DestroyAnimator(record.animatorID);
        Aether::PhysicsSystem::DestroyBody(record.bodyID);
        if (m_Scene.IsValid(entity)) m_Scene.DestroyHierarchy(entity);
    }
    m_ZombieRegistry.clear();
    m_ActiveZombies.clear();

    if (m_PlayerBodyID != 0)
        Aether::PhysicsSystem::DestroyBody(m_PlayerBodyID);

    m_ShadowShader.reset();
    m_MainShader.reset();
    m_ActiveChunks.clear();

    Aether::AssetManager::Unload(m_BgmSoundID);
    Aether::AssetManager::Unload(m_GunSoundID);
    Aether::AssetManager::Unload(m_GunReloadID);
    Aether::AssetManager::Unload(m_ZombieBiteID);
}

void MainGameLayer::Update(Aether::Timestep ts)
{
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();

    // Update camera first to get latest mouse-driven orientation
    m_Camera.Update(ts);

    float camDistance = m_Camera.GetDistance();
    m_CurrentRenderDistance = m_BaseRenderDistance + static_cast<int>(camDistance / m_ZoomInfluence);
    m_CurrentRenderDistance = std::clamp(m_CurrentRenderDistance, 1, 30);

    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

        // Movement relative to camera orientation
        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight = m_Camera.GetRightDirection();
        static float s_HeadBobTimer = 0.0f;
        static float s_BobAmplitudeBlend = 0.0f;

        camForward.y = 0.0f; camRight.y = 0.0f;
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight) > 0.0f) camRight = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);
        if (m_PlayerHealth > 0.0f) // Thêm điều kiện này bao bọc logic di chuyển
        {
            if (Aether::Input::IsKeyPressed(Aether::Key::W)) moveDir += camForward;
            if (Aether::Input::IsKeyPressed(Aether::Key::S)) moveDir -= camForward;
            if (Aether::Input::IsKeyPressed(Aether::Key::A)) moveDir -= camRight;
            if (Aether::Input::IsKeyPressed(Aether::Key::D)) moveDir += camRight;
        }
        else 
        {
            pTransform.Translation.y = yFloor;
        }

        bool isMoving = glm::length(moveDir) > 0.0f;

        if (isMoving)
        {
            moveDir = glm::normalize(moveDir);
            float speedMult  = GetSpeedMultiplier(pTransform.Translation);
            float stepLen    = m_PlayerSpeed * speedMult * (float)ts;

            // Try full move, then fall back to axis-separated slides so the
            // player never gets stuck against a wall when pressing diagonally.
            auto tryMove = [&](glm::vec3 delta) -> bool {
                glm::vec3 candidate = pTransform.Translation + delta;
                if (IsObstacleWithRadius(candidate)) return false;
                Aether::PhysTransform pt{ candidate, pTransform.Rotation };
                if (!m_FirstPerson && !Aether::PhysicsSystem::CanMove(m_PlayerBodyID, pt)) return false;
                pTransform.Translation = candidate;
                return true;
            };

            bool didMove = tryMove(moveDir * stepLen);
            if (!didMove) didMove = tryMove(glm::vec3(moveDir.x, 0, 0) * stepLen); // slide along Z wall
            if (!didMove) didMove = tryMove(glm::vec3(0, 0, moveDir.z) * stepLen); // slide along X wall

            if (!m_FirstPerson) {
                float targetAngle = glm::atan(moveDir.x, moveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(pTransform.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                float blend = 1.0f - glm::exp(-15.0f * (float)ts);
                pTransform.Rotation = glm::normalize(glm::slerp(pTransform.Rotation, targetRot, blend));
                m_Camera.Update(ts);
            }

            pTransform.Dirty = true;

            // Pause run animation when keys are held but all moves are blocked
            if (didMove != m_IsPlayerMoving) {
                if (didMove) rigSystem->Play(m_RunAnimation);
                else         rigSystem->Pause(m_RunAnimation);
                m_IsPlayerMoving = didMove;
            }

            if (didMove) {
                s_HeadBobTimer      += (float)ts * m_bobSpeed;
                s_BobAmplitudeBlend  = glm::mix(s_BobAmplitudeBlend, 1.0f, (float)ts * 10.0f);
            } else {
                s_BobAmplitudeBlend  = glm::mix(s_BobAmplitudeBlend, 0.0f, (float)ts * 10.0f);
            }
        }
        else
        {
            if (m_IsPlayerMoving) {
                rigSystem->Pause(m_RunAnimation);
                m_IsPlayerMoving = false;
            }
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 0.0f, (float)ts * 10.0f);
            if (s_BobAmplitudeBlend < 0.01f) {
                s_BobAmplitudeBlend = 0.0f;
                s_HeadBobTimer      = 0.0f;
            }
        }

        float targetAmplitude = m_FirstPerson ? m_bobStrength : m_bobStrength / 2.0f;
        float bobOffsetY = glm::abs(glm::sin(s_HeadBobTimer)) * targetAmplitude * s_BobAmplitudeBlend;

        // Sync camera with updated player position
        glm::vec3 playerTopPos = pTransform.Translation + glm::vec3(0.0f, 1.0f + bobOffsetY, 0.0f);
        glm::vec3 playerEyePos = pTransform.Translation + glm::vec3(0.0f, 1.7f + bobOffsetY, 0.0f);

        if (m_FirstPerson)
        {
            pTransform.Scale = { 0.001f, 0.001f, 0.001f };
            m_Camera.SetDistance(0.0f);
            m_Camera.SetFocalPoint(playerEyePos);
            pTransform.Rotation = glm::quat(glm::vec3(0.0f, -m_Camera.GetYaw(), 0.0f));
            pTransform.Dirty = true;
        }
        else
        {
            pTransform.Scale = { 1.0f, 1.0f, 1.0f };
            glm::vec3 shoulderOffset = m_Camera.GetRightDirection() * 0.5f;
            glm::vec3 stablePlayerPos = pTransform.Translation + glm::vec3(0.0f, 1.5f, 0.0f); // 1.5f là chiều cao cố định
            m_Camera.SetFocalPoint(stablePlayerPos + shoulderOffset);

            if (m_LockCamera) {
                m_Camera.SetDistance(5.0f);
                if (m_Camera.GetPitch() < 0.2f) m_Camera.SetPitch(0.2f);
            }
        }

        if (m_ShootTimer > 0.0f)
        {
            m_ShootTimer -= (float)ts;
            if (m_ShootTimer < 0.0f) m_ShootTimer = 0.0f;
        }

        // --- RELOAD LOGIC ---
        if (Aether::Input::IsKeyPressed(Aether::Key::R) && !m_IsReloading && m_CurrentAmmo < m_MaxAmmo) 
        {
            m_IsReloading = true;
            m_ReloadTimer = m_ReloadDuration;
            Aether::UUID src; 
            Aether::AudioSystem::CreateSource(src, m_GunReloadID, Aether::AudioType::Audio2D);
            Aether::AudioSystem::Play(src);
            sources.push_back(src);
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

        if (m_AmmoEmptyTimer > 0.0f)
            m_AmmoEmptyTimer -= (float)ts;

        // Keep sun light tracking player for stable shadows
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

        // --- ZOMBIE MANAGEMENT ---
        static float s_TimeAccumulator = 0.0f;
        s_TimeAccumulator += (float)ts;

        const float actualChunkSize = m_ChunkSize;
        const float despawnRadius = (m_CurrentRenderDistance * actualChunkSize) + (actualChunkSize * 1.5f);
        const float despawnRadiusSq = despawnRadius * despawnRadius;

        // Despawn out-of-range zombies
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
                m_Scene.DestroyHierarchy(zombie);
                it = m_ActiveZombies.erase(it);
            }
            else { ++it; }
        }

        // Spawn one zombie per second at the fog edge, up to cap
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

        // Staggered AI: alternate halves of the zombie list each frame
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

                // Re-project rotation onto ground plane to prevent vertical drift
                glm::vec3 facing = zT.Rotation * glm::vec3(0.0f, 0.0f, 1.0f);
                facing.y = 0.0f;
                if (glm::length(facing) > 0.001f) {
                    float zSpeedMult = GetSpeedMultiplier(zT.Translation);
                    glm::vec3 newZombiePos = zT.Translation + glm::normalize(facing) * (actualSpeed * zSpeedMult * (float)ts);
                    newZombiePos.y = yFloor;
                    Aether::PhysTransform zombieTarget{ newZombiePos, zT.Rotation };
                    auto& zRec = m_ZombieRegistry[zombie];
                    bool zombieBlocked = IsObstacleWithRadius(newZombiePos);
                    if (!zombieBlocked && Aether::PhysicsSystem::CanMove(zRec.bodyID, zombieTarget)) {
                        zT.Translation = newZombiePos;
                        if (rigSystem) rigSystem->Play(zRec.animatorID);
                    } else {
                        zT.Translation.y = yFloor;
                        if (rigSystem) rigSystem->Pause(zRec.animatorID);
                    }
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

    for (size_t i = 0; i < sources.size(); )
    {
        if (!Aether::AudioSystem::IsActive(sources[i]))
        {
            sources[i] = sources.back(); 
            sources.pop_back();   
        }
        else i++;
    }

    // --- Logic Máu Player ---
    if (m_DamageCooldown > 0.0f)
        m_DamageCooldown -= ts; // Giảm thời gian chờ theo thời gian thực

    if (m_Scene.IsValid(m_Player) && m_PlayerHealth > 0.0f && m_DamageCooldown <= 0.0f)
    {
        auto& pPos = m_Scene.GetComponent<Aether::TransformComponent>(m_Player).Translation;

        for (auto zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;

            auto& zPos = m_Scene.GetComponent<Aether::TransformComponent>(zombie).Translation;
            float dist = glm::distance(pPos, zPos);

            // Nếu zombie cách player dưới 1.5 mét thì cắn
            if (dist < 1.5f) 
            {
                m_PlayerHealth -= 10.0f; // Mỗi lần cắn mất 10 máu
                m_DamageCooldown = 1.0f; // 1 giây sau mới cho cắn tiếp (cooldown)

                Aether::UUID src; 
                Aether::AudioSystem::CreateSource(src, m_ZombieBiteID, Aether::AudioType::Audio2D);
                Aether::AudioSystem::Play(src); sources.push_back(src);
                
                AE_WARN("Player bi can! Mau con: {0}", m_PlayerHealth);
                break; // Thoát vòng lặp để 1 frame chỉ bị 1 con cắn
            }
        }
    }

    // Kiểm tra nếu hết máu
    if (m_PlayerHealth <= 0.0f)
    {
        // Hiển thị thông báo hoặc chờ bấm nút
        if (Aether::Input::IsKeyPressed(Aether::Key::R))
        {
            // 1. Reset chỉ số
            m_PlayerHealth = 100.0f;
            
            // 2. Reset vị trí về tọa độ gốc (hoặc điểm spawn)
            auto& pTrans = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
            pTrans.Translation = glm::vec3(0.0f, yFloor, 0.0f);
            
            // 3. Reset Camera (nếu cần)
            m_Camera.SetDistance(6.0f);
            
            AE_INFO("Player Resurrected!");
        }
    }

    m_Scene.Update(ts, &m_Camera);
}

void MainGameLayer::UpdateMapChunks(const glm::vec3& playerPos)
{
    const float actualChunkSize = m_ChunkSize;
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

            int randomRot = std::rand() % 4;
            float rotAngle = glm::radians(randomRot * 90.0f);
            t.Rotation = glm::quat(glm::vec3(0.0f, rotAngle, 0.0f));

            t.Dirty = true;

            auto& mesh = m_Scene.AddComponent<Aether::MeshComponent>(chunk);
            mesh.Mesh = m_BaseMapMesh;
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
            m_Scene.DestroyHierarchy(zombie);
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
    // Aether::Entity child = m_Scene.GetComponent<Aether::HierarchyComponent>(newZombie).firstChild;
    // child = m_Scene.GetComponent<Aether::HierarchyComponent>(child).firstChild;
    // m_Scene.GetComponent<Aether::MeshComponent>(child).ShowBounds = true;

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
        m_Scene.AddComponent<Aether::ColliderComponent>(newZombie, bodyID);
    }

    m_ZombieRegistry[newZombie] = { newAnimID, bodyID };
    m_ActiveZombies.push_back(newZombie);
    return newZombie;
}

float MainGameLayer::GetCellValue(int coordX, int coordZ) const
{
    int s = k_ObstacleMapSize;

    int chunkX = (int)std::floor((float)coordX / s);
    int chunkZ = (int)std::floor((float)coordZ / s);
    int localX = ((coordX % s) + s) % s;
    int localZ = ((coordZ % s) + s) % s;

    int rot = 0;
    auto it = m_ActiveChunks.find({ chunkX, chunkZ });
    if (it != m_ActiveChunks.end())
        rot = it->second.rotation;

    int rx = localX, rz = localZ;
    switch (rot) {
    case 1: rx = s - 1 - localZ; rz = localX;          break; // 90
    case 2: rx = s - 1 - localX; rz = s - 1 - localZ;  break; // 180
    case 3: rx = localZ;         rz = s - 1 - localX;  break; // 270
    default: break;
    }

    return m_ObstacleMap[rz][rx];
}

int MainGameLayer::GetObstacleCost(int coordX, int coordZ) const
{
    // Any non-zero cell (slow zone or wall) is treated as impassable by the flow field
    return (GetCellValue(coordX, coordZ) > 0.0f) ? 255 : 1;
}

bool MainGameLayer::IsObstacle(const glm::vec3& worldPos) const
{
    int cx = static_cast<int>(std::floor(worldPos.x / m_PathGridSize));
    int cz = static_cast<int>(std::floor(worldPos.z / m_PathGridSize));
    return GetCellValue(cx, cz) >= 1.0f;
}

bool MainGameLayer::IsObstacleWithRadius(const glm::vec3& worldPos) const
{
    // Probe center + 4 cardinal points at (capsule radius + skin) so the block
    // triggers before the player's visual edge actually touches the wall cell.
    const float r = k_CapsuleRadius + k_CollisionSkin;
    if (IsObstacle(worldPos))                              return true;
    if (IsObstacle(worldPos + glm::vec3( r, 0,  0)))      return true;
    if (IsObstacle(worldPos + glm::vec3(-r, 0,  0)))      return true;
    if (IsObstacle(worldPos + glm::vec3( 0, 0,  r)))      return true;
    if (IsObstacle(worldPos + glm::vec3( 0, 0, -r)))      return true;
    return false;
}

float MainGameLayer::GetSpeedMultiplier(const glm::vec3& worldPos) const
{
    const float r = k_CapsuleRadius + k_CollisionSkin;
    const glm::vec3 probes[] = {
        worldPos,
        worldPos + glm::vec3( r, 0,  0),
        worldPos + glm::vec3(-r, 0,  0),
        worldPos + glm::vec3( 0, 0,  r),
        worldPos + glm::vec3( 0, 0, -r),
    };

    float minMult = 1.0f;
    for (auto& p : probes) {
        int cx = static_cast<int>(std::floor(p.x / m_PathGridSize));
        int cz = static_cast<int>(std::floor(p.z / m_PathGridSize));
        // val 0 → 1.0x speed, val 0.5 → 0.5x, val 1 → already hard-blocked before this runs
        float mult = 1.0f - glm::clamp(GetCellValue(cx, cz), 0.0f, 1.0f);
        if (mult < minMult) minMult = mult;
    }
    return minMult;
}

void MainGameLayer::UpdateFlowField(const glm::vec3& targetPos)
{
    // Reset costs and re-sample obstacle map
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

    // BFS flood-fill from the target outward
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

    // Build gradient direction vectors from cost differences
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
    using namespace Aether;

    // --- AMMO HUD (bottom-right) ---
    {
        ImVec2 pos = { UI::ViewportPos().x + UI::ViewportSize().x - 180.f,
                       UI::ViewportPos().y + UI::ViewportSize().y - 100.f };

        if (auto w = UI::Overlay("AmmoDisplay", pos))
        {
            auto fs = UI::FontScale(1.5f);
            UI::TextColored(UI::Color::Green(), "WEAPON: PISTOL");

            if (m_IsReloading)
            {
                UI::TextColored(UI::Color::Red(), "RELOADING...");
            }
            else
            {
                ImVec4 color   = UI::Color::White();
                float  offsetY = 0.0f;

                if (m_CurrentAmmo == 0) {
                    color = UI::Color::Red();
                    if (m_AmmoEmptyTimer > 0.0f)
                        offsetY = -glm::abs(glm::sin(m_AmmoEmptyTimer * 20.0f)) * 15.0f * m_AmmoEmptyTimer;
                }

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);
                UI::TextColored(color, "%d / INF", m_CurrentAmmo);

                // Shoot cooldown bar
                if (m_ShootTimer > 0.0f)
                    UI::ProgressBar(1.0f - (m_ShootTimer / m_ShootDuration),
                                    {120.0f, 5.0f}, "", UI::Color::Orange());
            }
        }
    }

    // --- PERF OVERLAY ---
    {
        ImVec2 pos = { UI::ViewportPos().x + 30.0f, UI::ViewportPos().y + 60.0f };
        if (auto w = UI::Overlay("##perf", pos, 0.6f, ImGuiWindowFlags_NoMove))
        {
            UI::Text("FPS        %.1f",    ImGui::GetIO().Framerate);
            UI::Text("Frame time %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
        }
    }

    // --- CROSSHAIR ---
    {
        ImVec2 c   = UI::ScreenCenter();
        auto*  dl  = UI::FgDraw();

        if (m_IsReloading)
        {
            const float radius = 15.0f;
            const int   segs   = 8;
            for (int i = 0; i < segs; i++) {
                float  angle = m_ReloadRotation + i * (2.0f * 3.14159f / segs);
                ImVec2 p1 = { c.x + cosf(angle) * (radius - 5), c.y + sinf(angle) * (radius - 5) };
                ImVec2 p2 = { c.x + cosf(angle) *  radius,      c.y + sinf(angle) *  radius };
                dl->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 2.0f);
            }
            dl->AddCircleFilled(c, 1.5f, IM_COL32(255, 0, 0, 150));
        }
        else
        {
            static float crosshairSpread = 0.0f;
            crosshairSpread = m_IsPlayerMoving
                ? glm::mix(crosshairSpread, 12.0f, 0.1f)
                : glm::mix(crosshairSpread,  0.0f, 0.1f);

            float base   = 10.0f;
            float offset = 5.0f + crosshairSpread;
            ImU32 green  = IM_COL32(0, 255, 0, 255);

            dl->AddLine({c.x - offset - base, c.y}, {c.x - offset,        c.y}, green, 2.0f);
            dl->AddLine({c.x + offset,        c.y}, {c.x + offset + base, c.y}, green, 2.0f);
            dl->AddLine({c.x, c.y - offset - base}, {c.x, c.y - offset       }, green, 2.0f);
            dl->AddLine({c.x, c.y + offset       }, {c.x, c.y + offset + base}, green, 2.0f);
            dl->AddCircleFilled(c, 1.5f, IM_COL32(255, 255, 255, 255));
        }
    }

    // --- FLOW FIELD DEBUG OVERLAY ---
    if (m_ShowFlowFieldDebug && !m_FlowField.empty())
    {
        auto*       dl        = UI::FgDraw();
        ImVec2      dispPos   = UI::ViewportPos();
        ImVec2      dispSize  = UI::ViewportSize();
        glm::mat4   viewProj  = m_Camera.GetViewProjection();

        const ImU32 colGrid   = IM_COL32(  0, 255,   0,  80);
        const ImU32 colDir    = IM_COL32(  0, 255,   0, 200);
        const ImU32 colTarget = IM_COL32(255, 255,   0, 255);
        const float half      = m_PathGridSize * 0.5f;

        for (auto& [coord, cell] : m_FlowField)
        {
            if (cell.bestCost == 999999) continue;

            glm::vec3 worldCenter = {
                (coord.first  + 0.5f) * m_PathGridSize,
                yFloor + 0.05f,
                (coord.second + 0.5f) * m_PathGridSize
            };

            glm::vec3 corners[4] = {
                worldCenter + glm::vec3(-half, 0, -half),
                worldCenter + glm::vec3( half, 0, -half),
                worldCenter + glm::vec3( half, 0,  half),
                worldCenter + glm::vec3(-half, 0,  half),
            };

            ImVec2 sc[4]; bool allVisible = true;
            for (int i = 0; i < 4; i++) {
                ImVec2 s;
                if (!WorldToScreen(corners[i], viewProj, dispSize, s)) { allVisible = false; break; }
                sc[i] = { dispPos.x + s.x, dispPos.y + s.y };
            }
            if (!allVisible) continue;

            dl->AddQuad(sc[0], sc[1], sc[2], sc[3],
                        (cell.bestCost == 0) ? colTarget : colGrid, 1.0f);

            if (cell.bestCost > 0 && glm::length(cell.direction) > 0.01f)
            {
                glm::vec3 arrowEnd = worldCenter + cell.direction * (m_PathGridSize * 0.4f);
                ImVec2    sCenter, sEnd;
                if (WorldToScreen(worldCenter, viewProj, dispSize, sCenter) &&
                    WorldToScreen(arrowEnd,    viewProj, dispSize, sEnd))
                {
                    sCenter = { dispPos.x + sCenter.x, dispPos.y + sCenter.y };
                    sEnd    = { dispPos.x + sEnd.x,    dispPos.y + sEnd.y    };
                    dl->AddLine(sCenter, sEnd, colDir, 1.5f);

                    glm::vec2 dir  = glm::normalize(glm::vec2(sEnd.x - sCenter.x, sEnd.y - sCenter.y));
                    glm::vec2 perp = { -dir.y, dir.x };
                    const float h  = 4.0f;
                    dl->AddTriangleFilled(
                        sEnd,
                        { sEnd.x - dir.x*h + perp.x*h*0.5f, sEnd.y - dir.y*h + perp.y*h*0.5f },
                        { sEnd.x - dir.x*h - perp.x*h*0.5f, sEnd.y - dir.y*h - perp.y*h*0.5f },
                        colDir);
                }
            }
        }
    }

    if (m_ShowFlowFieldDebug && !m_ActiveChunks.empty())
    {
        auto*     dl       = UI::FgDraw();
        ImVec2    dispPos  = UI::ViewportPos();
        ImVec2    dispSize = UI::ViewportSize();
        glm::mat4 viewProj = m_Camera.GetViewProjection();

        const float half     = m_ChunkSize * 0.5f;
        const ImU32 colChunk = IM_COL32(255,  0,  0, 160);
        const ImU32 colLabel = IM_COL32(255, 80, 80, 255);

        for (auto& [coord, chunkData] : m_ActiveChunks)
        {
            glm::vec3 worldCenter = {
                (coord.first  + 0.5f) * m_ChunkSize,
                yFloor + 0.05f,
                (coord.second + 0.5f) * m_ChunkSize
            };

            glm::vec3 corners[4] = {
                worldCenter + glm::vec3(-half, 0.f, -half),
                worldCenter + glm::vec3( half, 0.f, -half),
                worldCenter + glm::vec3( half, 0.f,  half),
                worldCenter + glm::vec3(-half, 0.f,  half),
            };

            ImVec2 sc[4]; bool allVisible = true;
            for (int i = 0; i < 4; i++) {
                ImVec2 s;
                if (!WorldToScreen(corners[i], viewProj, dispSize, s)) { allVisible = false; break; }
                sc[i] = { dispPos.x + s.x, dispPos.y + s.y };
            }
            if (!allVisible) continue;

            dl->AddQuad(sc[0], sc[1], sc[2], sc[3], colChunk, 2.0f);

            ImVec2 sCenter;
            if (WorldToScreen(worldCenter, viewProj, dispSize, sCenter)) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d,%d", coord.first, coord.second);
                dl->AddText({ dispPos.x + sCenter.x, dispPos.y + sCenter.y }, colLabel, buf);
            }
        }
    }

    // --- HEALTH BAR ---
    {
        auto*  dl      = UI::FgDraw();
        ImVec2 vp      = UI::ViewportPos();
        float  barX    = vp.x + 30.0f;
        float  barY    = vp.y + 40.0f;
        float  barW    = 200.0f;
        float  barH    = 18.0f;
        float  radius  = 4.0f;
        float  frac    = glm::clamp(m_PlayerHealth / m_MaxHealth, 0.0f, 1.0f);

        dl->AddRectFilled({barX, barY}, {barX + barW, barY + barH},
                          IM_COL32(30, 30, 30, 180), radius);

        if (frac > 0.0f) {
            ImU32 fill = IM_COL32((ImU8)((1.f - frac) * 255), (ImU8)(frac * 220), 0, 220);
            dl->AddRectFilled({barX, barY}, {barX + barW * frac, barY + barH}, fill, radius);
        }

        dl->AddRect({barX, barY}, {barX + barW, barY + barH},
                    IM_COL32(180, 180, 180, 200), radius, 0, 1.5f);

        dl->AddText(ImGui::GetFont(), 20.0f,
                    {barX, barY - 22.0f}, IM_COL32(220, 220, 220, 230), "PLAYER HP");
    }

    // --- GAME OVER OVERLAY ---
    if (m_PlayerHealth <= 0.0f)
    {
        auto*  dl = UI::FgDraw();
        ImVec2 c  = UI::ScreenCenter();

        dl->AddRectFilled({c.x - 200, c.y - 70}, {c.x + 200, c.y + 70}, IM_COL32(0, 0, 0, 180), 10.0f);
        dl->AddRect      ({c.x - 200, c.y - 70}, {c.x + 200, c.y + 70}, IM_COL32(200, 0, 0, 200), 10.0f, 0, 2.0f);

        const char* diedText    = "YOU DIED!";
        const char* respawnText = "Press 'R' to Respawn";
        ImVec2 sz1 = ImGui::CalcTextSize(diedText);
        ImVec2 sz2 = ImGui::CalcTextSize(respawnText);

        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 2.0f,
                    {c.x - sz1.x, c.y - 40.0f}, IM_COL32(220, 0, 0, 255), diedText);
        dl->AddText({c.x - sz2.x * 0.5f, c.y + 20.0f},
                    IM_COL32(200, 200, 200, 220), respawnText);
    }

    DrawRadar();
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
        Aether::Input::IsMouseButtonPressed(Aether::Mouse::Button0) && m_PlayerHealth > 0.0f)
    {
        if (m_IsReloading) { 
            AE_WARN("Cant shoot while reloading!");
            return; 
        }
        if (m_CurrentAmmo <= 0) {
            m_AmmoEmptyTimer = 1.0f;
            AE_WARN("Out of ammo! Press R");
            return;
        }
        if (m_ShootTimer > 0.0f) return;

        m_CurrentAmmo--;
        m_ShootTimer  = m_ShootDuration;
        if (m_CurrentAmmo == 0)
            m_AmmoEmptyTimer = 1.0f;

        if (m_Scene.IsValid(m_Gun)) {
            auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
            rigSystem->Stop(m_ShootAnimation);
            rigSystem->Play(m_ShootAnimation);
        }

        Aether::UUID src;
        Aether::AudioSystem::CreateSource(src, m_GunSoundID, Aether::AudioType::Audio2D);
        Aether::AudioSystem::SetVolume(src, 0.3f);
        sources.push_back(src);
        Aether::AudioSystem::Play(src);

        glm::vec3 origin = m_Camera.GetPosition();
        glm::vec3 direction = glm::normalize(m_Camera.GetForwardDirection());
        Aether::RaycastHit hit = Aether::PhysicsSystem::CastRay(origin, direction, 100.0f);

        if (hit.Hit) {
            Aether::Entity target = m_Scene.FindEntity(hit.HitEntityID);
            if (target != Aether::Null_Entity && target != m_Player)
            {
                auto& rec = m_ZombieRegistry[target];
                Aether::PhysicsSystem::DestroyBody(rec.bodyID);
                m_Scene.DestroyHierarchy(target);
                m_ActiveZombies.erase(std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), target), m_ActiveZombies.end());
            }
        }

        event.Handled = true;
    }
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

void MainGameLayer::DrawRadar()
{
    using namespace Aether;

    const float radarRadius      = 100.0f;
    const float maxTrackDistance = 50.0f;

    auto*  dl = UI::FgDraw();
    ImVec2 vp = UI::ViewportPos();

    float  cx = vp.x + 20.0f + radarRadius;
    float  cy = vp.y + UI::ViewportSize().y - 20.0f - radarRadius;
    ImVec2 center(cx, cy);

    dl->AddCircleFilled(center, radarRadius, IM_COL32(10, 30, 10, 200));
    dl->AddCircle      (center, radarRadius, IM_COL32( 0, 255, 0, 255), 64, 2.0f);
    dl->AddCircle      (center, radarRadius * 0.5f, IM_COL32(0, 180, 0, 80), 64, 1.0f);
    dl->AddLine({center.x - radarRadius, center.y}, {center.x + radarRadius, center.y}, IM_COL32(0, 180, 0, 60), 1.0f);
    dl->AddLine({center.x, center.y - radarRadius}, {center.x, center.y + radarRadius}, IM_COL32(0, 180, 0, 60), 1.0f);

    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        glm::vec3 pPos   = pTransform.Translation;

        float cosA = cosf(-m_Camera.GetYaw());
        float sinA = sinf(-m_Camera.GetYaw());

        for (auto zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;

            auto& zTransform = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            glm::vec3 zPos   = zTransform.Translation;

            float relX = zPos.x - pPos.x;
            float relZ = zPos.z - pPos.z;
            float dist = sqrtf(relX * relX + relZ * relZ);

            if (dist <= maxTrackDistance)
            {
                float rx = relX * cosA - relZ * sinA;
                float ry = relX * sinA + relZ * cosA;
                float ox = (rx / maxTrackDistance) * radarRadius;
                float oy = (ry / maxTrackDistance) * radarRadius;

                float d = sqrtf(ox * ox + oy * oy);
                if (d > radarRadius - 3.0f) { float s = (radarRadius - 3.0f) / d; ox *= s; oy *= s; }

                dl->AddCircleFilled({center.x + ox, center.y + oy}, 3.5f, IM_COL32(255, 50, 50, 255));
            }
        }

        dl->AddCircleFilled(center, 5.0f, IM_COL32(255, 255, 255, 255));
        dl->AddTriangleFilled(
            {center.x,        center.y - 9.0f},
            {center.x - 4.0f, center.y + 4.0f},
            {center.x + 4.0f, center.y + 4.0f},
            IM_COL32(100, 220, 255, 220));
    }

    const char* label   = "RADAR";
    const float fontSize = 24.0f;
    ImVec2      labelSz  = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.f, label);
    dl->AddText(ImGui::GetFont(), fontSize,
                {center.x - labelSz.x * 0.5f, cy - radarRadius - fontSize - 5.0f},
                IM_COL32(0, 220, 0, 200), label);
}