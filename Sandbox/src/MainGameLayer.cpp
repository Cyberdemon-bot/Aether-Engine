#include "MainGameLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>
#include <set>

static Aether::Entity FindAnimatorEntity(Aether::Scene& scene, Aether::Entity root)
{
    if (!scene.IsValid(root)) return Aether::Null_Entity;
    if (scene.HasComponent<Aether::AnimatorComponent>(root)) return root;
    auto& hier = scene.GetComponent<Aether::HierarchyComponent>(root);
    Aether::Entity child = hier.firstChild;
    while (child != Aether::Null_Entity)
    {
        Aether::Entity found = FindAnimatorEntity(scene, child);
        if (found != Aether::Null_Entity) return found;
        child = scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
    }
    return Aether::Null_Entity;
}

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(6.0f);
}

void MainGameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    auto* renderer = Aether::ServiceManager::GetService<Aether::Renderer>();
    auto* asset_manager = Aether::ServiceManager::GetService<Aether::AssetManager>();

    renderer->SetLutMap("Assets/textures/LUT.png");
    renderer->SetSkyBox("Assets/textures/skybox.png");

    auto& window = Aether::Application::Get().GetWindow();
    Aether::FramebufferSpec sceneFbSpec;
    sceneFbSpec.Width       = window.GetWidth();
    sceneFbSpec.Height      = window.GetHeight();
    sceneFbSpec.Attachments = { Aether::ImageFormat::RGBA8, Aether::ImageFormat::DEPTH24STENCIL8 };

    m_MainShader = Aether::Shader::Create("Assets/shaders/Standard.shader");
    m_MainShader->Bind();
    m_MainShader->SetUBOSlot("Camera", 0);
    m_MainShader->SetUBOSlot("Lights", 2);
    m_MainFbo = Aether::FrameBuffer::Create(sceneFbSpec);

    Aether::RenderPass mainPass;
    mainPass.TargetFBO    = m_MainFbo.get();
    mainPass.Shader       = m_MainShader.get();
    mainPass.ClearColor   = true;
    mainPass.ClearDepth   = true;
    mainPass.UsingSkybox  = true;
    mainPass.ClearValue   = glm::vec4(0.5f, 0.7f, 1.0f, 1.0f);
    mainPass.CullFace     = Aether::State::BACK_CULL;
    mainPass.OnScreen     = true;
    mainPass.LutIntensity = 0.2f;
    mainPass.UsingShadowmap = true;
    m_Pipeline.push_back(mainPass);

    renderer->SetPipeline(m_Pipeline.data(), m_Pipeline.size());
    m_Scene.Init();

    // =========================================================================
    // SUN LIGHT
    // =========================================================================
    m_SunLight = m_Scene.CreateEntity("Sun Light");
    auto& lightComp              = m_Scene.AddComponent<Aether::LightComponent>(m_SunLight);
    lightComp.Config.type        = Aether::LightType::Directional;
    lightComp.Config.color       = glm::vec3(0.9f, 0.95f, 1.0f);
    lightComp.Config.intensity   = 1.5f;
    lightComp.Config.castShadows = true;
    lightComp.Config.direction   = glm::vec3(-0.5f, -1.0f, -0.5f);

    auto& sunTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
    sunTransform.Translation = glm::vec3(0.0f, 50.0f, 0.0f);
    sunTransform.Dirty       = true;

    renderer->ActivatePass(0);

    // =========================================================================
    // MAP
    // =========================================================================
    auto uploadMap = Aether::Importer::Upload(Aether::Importer::Import("assets/models/map.glb"));
    if (!uploadMap.meshIDs.empty()) {
        m_BaseMapMesh = asset_manager->GetHandle(uploadMap.meshIDs[0]);
        if (uploadMap.matIDs.empty()) AE_ERROR("no material!");
        for (auto& matID : uploadMap.matIDs)
            m_BaseMapMaterials.push_back(asset_manager->GetHandle(matID));
    }

    // =========================================================================
    // PLAYER
    // =========================================================================
    m_Player = m_Scene.CreateEntity("Player");
    auto& pTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
    pTransform.Translation = { 0.0f, yFloor, 0.0f };
    pTransform.Scale       = { 1.0f, 1.0f,   1.0f };
    pTransform.Rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    pTransform.Dirty       = true;

    auto uploadPlayer = Aether::Importer::Upload(Aether::Importer::Import("assets/models/humanv2.glb"));
    m_Scene.LoadHierarchy(uploadPlayer, m_Player);

    // =========================================================================
    // PLAYER PHYSICS
    // =========================================================================
    {
        Aether::BodyConfig cfg;
        cfg.motionType  = Aether::MotionType::Kinematic;
        cfg.shape       = Aether::ColliderShape::Capsule;
        cfg.size        = glm::vec3(0.35f, 2.5f, 0.0f);
        cfg.transform   = { pTransform.Translation, glm::quat(1,0,0,0) };
        cfg.offset      = glm::vec3(0.0f, 1.0f, 0.0f);
        cfg.friction    = 0.5f;
        cfg.restitution = 0.0f;
        m_PlayerBodyHandle = Aether::PhysicsSystem::CreateBody(m_Scene.GetPhysicsInstance(), cfg);
        m_Scene.AddComponent<Aether::ColliderComponent>(m_Player, m_Scene.GetPhysicsInstance(), m_PlayerBodyHandle);

        Aether::UUID playerID = m_Scene.GetComponent<Aether::IDComponent>(m_Player).ID;
        Aether::PhysicsSystem::SetUUID(m_Scene.GetPhysicsInstance(), m_PlayerBodyHandle, playerID);
    }
    m_Scene.GetComponent<Aether::AnimatorComponent>(FindAnimatorEntity(m_Scene, m_Player)).IsPlaying = false;

    // =========================================================================
    // ZOMBIES
    // =========================================================================
    m_ZombieSceneData = Aether::Importer::Upload(Aether::Importer::Import("assets/models/zombie.glb"));

    // =========================================================================
    // GUN
    // =========================================================================
    m_Gun = m_Scene.CreateEntity("Weapon_Gun");
    auto& gTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);
    gTransform.Translation = { 0.0f, 0.0f, 0.0f };
    gTransform.Scale       = { 1.0f, 1.0f, 1.0f };
    gTransform.Dirty       = true;

    auto uploadGun = Aether::Importer::Upload(Aether::Importer::Import("assets/models/gun.glb"));
    m_Scene.LoadHierarchy(uploadGun, m_Gun);

    if (!uploadGun.animators.empty())
    {
        Aether::Entity gunAnimEnt = FindAnimatorEntity(m_Scene, m_Gun);
        if (gunAnimEnt != Aether::Null_Entity)
        {
            auto& animComp = m_Scene.GetComponent<Aether::AnimatorComponent>(gunAnimEnt);
            animComp.Loop      = false;
            animComp.IsPlaying = false;
        }
    }

    // =========================================================================
    // MISC
    // =========================================================================
    m_PathGridSize = m_ChunkSize / static_cast<float>(m_FlowFieldSubdivisions);

    Aether::PhysicsSystem::SetGravity(m_Scene.GetPhysicsInstance(), { 0.0f, 0.0f, 0.0f });

    // =========================================================================
    // AUDIO
    // =========================================================================
    m_BgmSource    = Aether::AudioSystem::CreateSource("assets/audios/Hatsune Miku - Ievan Polkka.mp3", Aether::AudioType::Audio2D);
    m_GunSource    = Aether::AudioSystem::CreateSource("assets/audios/pistol.mp3",        Aether::AudioType::Audio2D);
    m_ReloadSource = Aether::AudioSystem::CreateSource("assets/audios/pistol_reload.mp3", Aether::AudioType::Audio2D);
    m_BiteSource   = Aether::AudioSystem::CreateSource("assets/audios/zombie_bite.mp3",   Aether::AudioType::Audio2D);

    Aether::AudioSystem::SetLooping(m_BgmSource, true);
    Aether::AudioSystem::Play(m_BgmSource);

    m_SheetHandle = asset_manager->CreateAsset<Aether::Sheet>(Aether::UUID());
    m_MapSheet = asset_manager->GetAsset<Aether::Sheet>(m_SheetHandle);
    m_MapSheet->Resize(m_BaseMapMaterials.size());
    m_MapSheet->CopyDefaultList(m_BaseMapMaterials);

    AE_INFO("MainGameLayer started.");
}

void MainGameLayer::Detach()
{
    m_ActiveZombies.clear();
    m_MainShader.reset();
    m_ActiveChunks.clear();
    m_Scene.Shutdown();
}

void MainGameLayer::Update(Aether::Timestep ts)
{
    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    m_Camera.Update(ts);

    float camDistance       = m_Camera.GetDistance();
    m_CurrentRenderDistance = m_BaseRenderDistance + static_cast<int>(camDistance / m_ZoomInfluence);
    m_CurrentRenderDistance = std::clamp(m_CurrentRenderDistance, 1, 30);

    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight   = m_Camera.GetRightDirection();
        static float s_HeadBobTimer      = 0.0f;
        static float s_BobAmplitudeBlend = 0.0f;

        camForward.y = 0.0f; camRight.y = 0.0f;
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight)   > 0.0f) camRight   = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);
        if (m_PlayerHealth > 0.0f)
        {
            if (Aether::Input::IsKeyPressed(Aether::Key::KeyCode::W)) moveDir += camForward;
            if (Aether::Input::IsKeyPressed(Aether::Key::KeyCode::S)) moveDir -= camForward;
            if (Aether::Input::IsKeyPressed(Aether::Key::KeyCode::A)) moveDir -= camRight;
            if (Aether::Input::IsKeyPressed(Aether::Key::KeyCode::D)) moveDir += camRight;
        }
        else
        {
            pTransform.Translation.y = yFloor;
        }

        bool isMoving = glm::length(moveDir) > 0.0f;

        Aether::Entity playerAnimEnt = FindAnimatorEntity(m_Scene, m_Player);

        if (isMoving)
        {
            moveDir         = glm::normalize(moveDir);
            float speedMult = GetSpeedMultiplier(pTransform.Translation);
            float stepLen   = m_PlayerSpeed * speedMult * (float)ts;

            auto tryMove = [&](glm::vec3 delta) -> bool {
                glm::vec3 candidate = pTransform.Translation + delta;
                if (IsObstacleWithRadius(candidate)) return false;
                pTransform.Translation = candidate;
                return true;
            };

            bool didMove = tryMove(moveDir * stepLen);
            if (!didMove) didMove = tryMove(glm::vec3(moveDir.x, 0, 0) * stepLen);
            if (!didMove) didMove = tryMove(glm::vec3(0, 0, moveDir.z) * stepLen);

            {
                float     targetAngle = glm::atan(moveDir.x, moveDir.z);
                glm::quat targetRot   = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(pTransform.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                float blend = 1.0f - glm::exp(-15.0f * (float)ts);
                pTransform.Rotation = glm::normalize(glm::slerp(pTransform.Rotation, targetRot, blend));
                m_Camera.Update(ts);
            }

            pTransform.Dirty = true;

            if (didMove != m_IsPlayerMoving) {
                if (playerAnimEnt != Aether::Null_Entity)
                    m_Scene.GetComponent<Aether::AnimatorComponent>(playerAnimEnt).IsPlaying = didMove;
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
                if (playerAnimEnt != Aether::Null_Entity)
                    m_Scene.GetComponent<Aether::AnimatorComponent>(playerAnimEnt).IsPlaying = false;
                m_IsPlayerMoving = false;
            }
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 0.0f, (float)ts * 10.0f);
            if (s_BobAmplitudeBlend < 0.01f) {
                s_BobAmplitudeBlend = 0.0f;
                s_HeadBobTimer      = 0.0f;
            }
        }

        float bobOffsetY    = glm::abs(glm::sin(s_HeadBobTimer)) * (m_bobStrength / 2.0f) * s_BobAmplitudeBlend;
        glm::vec3 playerTopPos = pTransform.Translation + glm::vec3(0.0f, 1.0f + bobOffsetY, 0.0f);

        {
            pTransform.Scale = { 1.0f, 1.0f, 1.0f };
            glm::vec3 shoulderOffset  = m_Camera.GetRightDirection() * 0.5f;
            glm::vec3 stablePlayerPos = pTransform.Translation + glm::vec3(0.0f, 1.5f, 0.0f);
            pTransform.Dirty = true;
            m_Camera.SetFocalPoint(stablePlayerPos + shoulderOffset);

            if (m_LockCamera) {
                m_Camera.SetDistance(5.0f);
                if (m_Camera.GetPitch() < 0.2f) m_Camera.SetPitch(0.2f);
            }
        }

        if (m_ShootTimer > 0.0f) {
            m_ShootTimer -= (float)ts;
            if (m_ShootTimer < 0.0f) m_ShootTimer = 0.0f;
        }

        // --- RELOAD LOGIC ---
        if (Aether::Input::IsKeyPressed(Aether::Key::KeyCode::R) && !m_IsReloading && m_CurrentAmmo < m_MaxAmmo)
        {
            m_IsReloading = true;
            m_ReloadTimer = m_ReloadDuration;
            Aether::AudioSystem::Stop(m_ReloadSource);
            Aether::AudioSystem::Play(m_ReloadSource);
            AE_INFO("Reloading...");
        }

        if (m_IsReloading) {
            m_ReloadTimer    -= (float)ts;
            m_ReloadRotation += (float)ts * 7.0f;
            if (m_ReloadTimer <= 0.0f) {
                m_CurrentAmmo = m_MaxAmmo;
                m_IsReloading = false;
                AE_INFO("Reload complete.");
            }
        }

        if (m_AmmoEmptyTimer > 0.0f)
            m_AmmoEmptyTimer -= (float)ts;

        if (m_Scene.IsValid(m_SunLight)) {
            auto& lightTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
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
        const float despawnRadius   = (m_CurrentRenderDistance * actualChunkSize) + (actualChunkSize * 1.5f);
        const float despawnRadiusSq = despawnRadius * despawnRadius;

        for (auto it = m_ActiveZombies.begin(); it != m_ActiveZombies.end(); )
        {
            Aether::Entity zombie = *it;
            if (!m_Scene.IsValid(zombie)) { it = m_ActiveZombies.erase(it); continue; }

            auto&     zT   = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            glm::vec3 diff = pTransform.Translation - zT.Translation;
            diff.y = 0.0f;

            if (glm::dot(diff, diff) > despawnRadiusSq)
            {
                m_Scene.DestroyHierarchy(zombie);
                it = m_ActiveZombies.erase(it);
            }
            else { ++it; }
        }

        static float s_SpawnTimer = 0.0f;
        s_SpawnTimer += (float)ts;
        if (s_SpawnTimer >= 1.0f) {
            s_SpawnTimer = 0.0f;
            if (m_ActiveZombies.size() < maxZombies) {
                float     randomAngle = glm::radians((float)(std::rand() % 360));
                float     spawnDist   = (m_CurrentRenderDistance * actualChunkSize);
                glm::vec3 spawnPos    = pTransform.Translation
                    + glm::vec3(glm::cos(randomAngle), 0.0f, glm::sin(randomAngle)) * spawnDist;
                spawnPos.y = yFloor;
                SpawnZombie(spawnPos);
            }
        }

        static uint32_t s_ZombieUpdateCounter = 0;
        s_ZombieUpdateCounter++;
        int zombieIndex = 0;

        for (Aether::Entity zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;
            zombieIndex++;
            if (zombieIndex % 2 != s_ZombieUpdateCounter % 2) continue;

            auto&    zT    = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            uint32_t zSeed = (uint32_t)zombie;

            int  zX     = static_cast<int>(std::floor(zT.Translation.x / m_PathGridSize));
            int  zZ     = static_cast<int>(std::floor(zT.Translation.z / m_PathGridSize));
            auto zCoord = std::make_pair(zX, zZ);

            glm::vec3 baseDir(0.0f, 0.0f, 1.0f);
            auto flowIt = m_FlowField.find(zCoord);
            if (flowIt != m_FlowField.end() && glm::length(flowIt->second.direction) > 0.0000001f)
                baseDir = flowIt->second.direction;
            else {
                glm::vec3 diffBase = pTransform.Translation - zT.Translation;
                diffBase.y = 0.0f;
                if (glm::length(diffBase) > 0.001f) baseDir = glm::normalize(diffBase);
            }

            glm::vec3 rightDir = glm::vec3(-baseDir.z, 0.0f, baseDir.x);
            float     wobble   = glm::sin(s_TimeAccumulator * 2.5f + zSeed) * 0.35f;

            glm::vec3 separationForce(0.0f);
            const float sepRadiusSq = 0.64f;
            int neighborCount = 0;
            for (Aether::Entity other : m_ActiveZombies) {
                if (other == zombie || !m_Scene.IsValid(other)) continue;
                auto&     otherT = m_Scene.GetComponent<Aether::TransformComponent>(other);
                glm::vec3 d      = zT.Translation - otherT.Translation;
                d.y = 0.0f;
                float distSq = glm::dot(d, d);
                if (distSq > 0.001f && distSq < sepRadiusSq) {
                    float dist = glm::sqrt(distSq);
                    separationForce += (d / dist) * (0.8f - dist);
                    neighborCount++;
                }
            }
            if (neighborCount > 0) separationForce /= (float)neighborCount;

            glm::vec3 totalForce   = baseDir + rightDir * wobble + separationForce * 0.5f;
            glm::vec3 finalMoveDir = (glm::length(totalForce) > 0.001f) ? glm::normalize(totalForce) : baseDir;

            float speedMod    = 0.8f + ((zSeed % 100) / 100.0f) * 0.4f;
            float actualSpeed = m_ZombieSpeed * speedMod;

            glm::vec3 diffToPlayer = pTransform.Translation - zT.Translation;
            diffToPlayer.y = 0.0f;

            if (glm::length(diffToPlayer) > 1.2f)
            {
                float     targetAngle = glm::atan(finalMoveDir.x, finalMoveDir.z);
                glm::quat targetRot   = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(zT.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                zT.Rotation = glm::normalize(
                    glm::slerp(zT.Rotation, targetRot, 1.0f - glm::exp(-5.0f * (float)ts)));

                glm::vec3 facing = zT.Rotation * glm::vec3(0.0f, 0.0f, 1.0f);
                facing.y = 0.0f;
                if (glm::length(facing) > 0.001f) {
                    float     zSpeedMult   = GetSpeedMultiplier(zT.Translation);
                    glm::vec3 newZombiePos = zT.Translation
                        + glm::normalize(facing) * (actualSpeed * zSpeedMult * (float)ts);
                    newZombiePos.y = yFloor;

                    Aether::Entity zAnimEnt = FindAnimatorEntity(m_Scene, zombie);
                    if (!IsObstacleWithRadius(newZombiePos)) {
                        zT.Translation = newZombiePos;
                        if (zAnimEnt != Aether::Null_Entity)
                            m_Scene.GetComponent<Aether::AnimatorComponent>(zAnimEnt).IsPlaying = true;
                    } else {
                        zT.Translation.y = yFloor;
                        if (zAnimEnt != Aether::Null_Entity)
                            m_Scene.GetComponent<Aether::AnimatorComponent>(zAnimEnt).IsPlaying = false;
                    }
                }
                zT.Dirty = true;
            }
        }
    }

    // --- SHADER UNIFORMS ---
    m_MainShader->Bind();
    m_MainShader->SetFloat ("u_Bias",       m_ShadowBias);
    m_MainShader->SetInt   ("u_FogMode",    m_FogMode);
    m_MainShader->SetFloat3("u_FogColor",   m_FogColor);
    m_MainShader->SetFloat ("u_FogDensity", m_FogDensity);
    m_MainShader->SetFloat ("u_FogStart",   m_FogStart);
    m_MainShader->SetFloat ("u_FogEnd",     m_FogEnd);

    // --- GUN POSITIONING ---
    if (m_Scene.IsValid(m_Gun) && m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        auto& gTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Gun);

        glm::quat pRot    = pTransform.Rotation;
        glm::vec3 forward = pRot * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 right   = pRot * glm::vec3(1.0f, 0.0f,  0.0f);
        glm::vec3 up      = pRot * glm::vec3(0.0f, 1.0f,  0.0f);

        gTransform.Translation = pTransform.Translation
            + (right * m_GunPosTP.x) + (up * m_GunPosTP.y) + (forward * m_GunPosTP.z);
        gTransform.Rotation    = pRot * glm::quat(glm::radians(m_GunRotTP));
        gTransform.Scale       = m_GunScaleTP;
        gTransform.Dirty       = true;
    }

    // --- PLAYER HEALTH ---
    if (m_DamageCooldown > 0.0f)
        m_DamageCooldown -= ts;

    if (m_Scene.IsValid(m_Player) && m_PlayerHealth > 0.0f && m_DamageCooldown <= 0.0f)
    {
        auto& pPos = m_Scene.GetComponent<Aether::TransformComponent>(m_Player).Translation;
        for (auto zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;
            auto& zPos = m_Scene.GetComponent<Aether::TransformComponent>(zombie).Translation;
            if (glm::distance(pPos, zPos) < 1.5f)
            {
                m_PlayerHealth   -= 10.0f;
                m_DamageCooldown  = 1.0f;
                Aether::UUID src;
                Aether::AudioSystem::Stop(m_BiteSource);
                Aether::AudioSystem::Play(m_BiteSource);
                AE_WARN("Player bit! HP remaining: {0}", m_PlayerHealth);
                break;
            }
        }
    }

    if (m_PlayerHealth <= 0.0f && Aether::Input::IsKeyPressed(Aether::Key::KeyCode::R))
    {
        m_PlayerHealth = 100.0f;
        auto& pTrans = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);
        pTrans.Translation = glm::vec3(0.0f, yFloor, 0.0f);
        m_Camera.SetDistance(6.0f);
        AE_INFO("Player Resurrected!");
    }

    m_Scene.Update(ts, &m_Camera);
}

void MainGameLayer::UpdateMapChunks(const glm::vec3& playerPos)
{
    const float actualChunkSize = m_ChunkSize;
    int centerX = static_cast<int>(std::floor(playerPos.x / actualChunkSize));
    int centerZ = static_cast<int>(std::floor(playerPos.z / actualChunkSize));

    std::set<std::pair<int, int>> chunksToKeep;

    for (int x = -m_CurrentRenderDistance; x <= m_CurrentRenderDistance; ++x) {
        for (int z = -m_CurrentRenderDistance; z <= m_CurrentRenderDistance; ++z) {
            auto coord = std::make_pair(centerX + x, centerZ + z);
            chunksToKeep.insert(coord);
            if (m_ActiveChunks.count(coord)) continue;

            Aether::Entity chunk = m_Scene.CreateEntity(
                "MapGrid_" + std::to_string(coord.first) + "_" + std::to_string(coord.second));
            auto& t = m_Scene.GetComponent<Aether::TransformComponent>(chunk);
            t.Translation = glm::vec3(
                (coord.first  + 0.5f) * actualChunkSize, -(actualChunkSize / 2.0f),
                (coord.second + 0.5f) * actualChunkSize);

            int   randomRot = std::rand() % 4;
            float rotAngle  = glm::radians(randomRot * 90.0f);
            t.Rotation = glm::quat(glm::vec3(0.0f, rotAngle, 0.0f));
            t.Dirty = true;

            auto& mesh     = m_Scene.AddComponent<Aether::MeshComponent>(chunk);
            mesh.Mesh      = m_BaseMapMesh;
            mesh.Sheet     = m_SheetHandle;

            ChunkData newData;
            newData.landEntity = chunk;
            newData.rotation   = randomRot;
            if (std::rand() % 100 < 80 && (std::abs(x) > 2 || std::abs(z) > 2)) {
                glm::vec3 spawnPos = t.Translation;
                spawnPos.y = yFloor;
                Aether::Entity zEnt = SpawnZombie(spawnPos);
                if (zEnt != Aether::Null_Entity) newData.zombies.push_back(zEnt);
            }
            m_ActiveChunks[coord] = newData;
        }
    }

    for (auto it = m_ActiveChunks.begin(); it != m_ActiveChunks.end(); ) {
        if (chunksToKeep.count(it->first)) { ++it; continue; }

        for (Aether::Entity zombie : it->second.zombies) {
            if (!m_Scene.IsValid(zombie)) continue;
            m_ActiveZombies.erase(
                std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), zombie),
                m_ActiveZombies.end());
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

    Aether::Entity newZombie     = m_Scene.CreateEntity("Zombie_Minion");
    auto& zTransform             = m_Scene.GetComponent<Aether::TransformComponent>(newZombie);
    zTransform.Translation       = position;
    zTransform.Scale             = { 1.0f, 1.0f, 1.0f };
    zTransform.Rotation          = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    zTransform.Dirty             = true;

    m_Scene.LoadHierarchy(m_ZombieSceneData, newZombie);

    Aether::Entity zAnimEnt = FindAnimatorEntity(m_Scene, newZombie);
    if (zAnimEnt != Aether::Null_Entity)
    {
        m_Scene.GetComponent<Aether::AnimatorComponent>(zAnimEnt).IsPlaying    = true;
        m_Scene.GetComponent<Aether::AnimatorComponent>(zAnimEnt).ActiveClipIdx = 4;
    }

    m_Scene.AddComponent<Aether::ColliderComponent>(newZombie);
    auto& cmp = m_Scene.GetComponent<Aether::ColliderComponent>(newZombie);

    cmp.Type = Aether::MotionType::Kinematic;
    cmp.Shape = Aether::ColliderShape::Capsule;
    cmp.Size = glm::vec3(0.35f, 2.0f, 0.0f);
    cmp.ColliderOffset = glm::vec3(0.0f, 1.0f, 0.0f);

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
    case 1: rx = s - 1 - localZ; rz = localX;          break;
    case 2: rx = s - 1 - localX; rz = s - 1 - localZ;  break;
    case 3: rx = localZ;         rz = s - 1 - localX;  break;
    default: break;
    }

    return m_ObstacleMap[rz][rx];
}

int MainGameLayer::GetObstacleCost(int coordX, int coordZ) const
{
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
    const float r = k_CapsuleRadius + k_CollisionSkin;
    if (IsObstacle(worldPos))                         return true;
    if (IsObstacle(worldPos + glm::vec3( r, 0,  0))) return true;
    if (IsObstacle(worldPos + glm::vec3(-r, 0,  0))) return true;
    if (IsObstacle(worldPos + glm::vec3( 0, 0,  r))) return true;
    if (IsObstacle(worldPos + glm::vec3( 0, 0, -r))) return true;
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
        int   cx   = static_cast<int>(std::floor(p.x / m_PathGridSize));
        int   cz   = static_cast<int>(std::floor(p.z / m_PathGridSize));
        float mult = 1.0f - glm::clamp(GetCellValue(cx, cz), 0.0f, 1.0f);
        if (mult < minMult) minMult = mult;
    }
    return minMult;
}

void MainGameLayer::UpdateFlowField(const glm::vec3& targetPos)
{
    for (auto& [coord, cell] : m_FlowField) {
        cell.bestCost  = 999999;
        cell.direction = glm::vec3(0.0f);
        cell.cost      = GetObstacleCost(coord.first, coord.second);
    }

    int  targetX     = static_cast<int>(std::floor(targetPos.x / m_PathGridSize));
    int  targetZ     = static_cast<int>(std::floor(targetPos.z / m_PathGridSize));
    auto targetCoord = std::make_pair(targetX, targetZ);

    m_FlowField[targetCoord].bestCost = 0;
    m_FlowField[targetCoord].cost     = 1;

    static const std::vector<std::pair<int, int>> neighbors = {
        {0, 1}, {0,-1}, {1, 0}, {-1, 0},
        {1, 1}, {1,-1}, {-1, 1}, {-1,-1}
    };

    const int maxRadius = 40;
    std::queue<std::pair<int, int>> openList;
    openList.push(targetCoord);

    while (!openList.empty())
    {
        auto current = openList.front(); openList.pop();
        if (std::abs(current.first  - targetX) > maxRadius ||
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
            int newCost  = currCost + (moveCost * neighbor.cost);

            if (newCost < neighbor.bestCost) {
                neighbor.bestCost = newCost;
                openList.push(neighborCoord);
            }
        }
    }

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
        glm::vec2 pos = UI::Screen::Anchor(1.f, 1.f) + glm::vec2(-180.f, -100.f);
        if (auto w = UI::Overlay("AmmoDisplay", {pos.x, pos.y}))
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

                if (m_ShootTimer > 0.0f)
                    UI::ProgressBar(1.0f - (m_ShootTimer / m_ShootDuration),
                                    {120.0f, 5.0f}, "", UI::Color::Orange());
            }
        }
    }

    // --- PERF OVERLAY (top-left) ---
    UI::PerformanceOverlay(0, 30, 60);

    // --- CROSSHAIR ---
    {
        glm::vec2 c  = UI::Screen::Center();
        auto      cv = UI::Foreground();

        if (m_IsReloading)
        {
            const float radius = 15.0f;
            const int   segs   = 8;
            for (int i = 0; i < segs; i++) {
                float     angle = m_ReloadRotation + i * (2.0f * 3.14159f / segs);
                glm::vec2 p1    = c + glm::vec2(cosf(angle), sinf(angle)) * (radius - 5.f);
                glm::vec2 p2    = c + glm::vec2(cosf(angle), sinf(angle)) * radius;
                cv.Line(p1, p2, UI::Col32(255, 255, 255, 255), 2.f);
            }
            cv.CircleFill(c, 1.5f, UI::Col32(255, 0, 0, 150));
        }
        else
        {
            static float s_CrosshairSpread = 0.0f;
            s_CrosshairSpread = m_IsPlayerMoving
                ? glm::mix(s_CrosshairSpread, 12.0f, 0.1f)
                : glm::mix(s_CrosshairSpread,  0.0f, 0.1f);

            float base   = 10.0f;
            float offset = 5.0f + s_CrosshairSpread;
            ImU32 green  = UI::Col32(0, 255, 0, 255);

            cv.Line({c.x - offset - base, c.y}, {c.x - offset,        c.y}, green, 2.f);
            cv.Line({c.x + offset,        c.y}, {c.x + offset + base, c.y}, green, 2.f);
            cv.Line({c.x, c.y - offset - base}, {c.x, c.y - offset       }, green, 2.f);
            cv.Line({c.x, c.y + offset       }, {c.x, c.y + offset + base}, green, 2.f);
            cv.CircleFill(c, 1.5f, UI::Col32(255, 255, 255, 255));
        }
    }

    // --- FLOW FIELD DEBUG OVERLAY ---
    if (m_ShowFlowFieldDebug && !m_FlowField.empty())
    {
        auto      cv       = UI::Foreground();
        glm::mat4 viewProj = m_Camera.GetViewProjection();

        const ImU32 colGrid   = UI::Col32(  0, 255,   0,  80);
        const ImU32 colDir    = UI::Col32(  0, 255,   0, 200);
        const ImU32 colTarget = UI::Col32(255, 255,   0, 255);
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

            glm::vec2 sc[4]; bool allVisible = true;
            for (int i = 0; i < 4; i++)
                if (!UI::Screen::Project(corners[i], viewProj, sc[i])) { allVisible = false; break; }
            if (!allVisible) continue;

            cv.Quad(sc[0], sc[1], sc[2], sc[3],
                    (cell.bestCost == 0) ? colTarget : colGrid, 1.f);

            if (cell.bestCost > 0 && glm::length(cell.direction) > 0.01f)
            {
                glm::vec3 arrowEnd3 = worldCenter + cell.direction * (m_PathGridSize * 0.4f);
                glm::vec2 sCenter, sEnd;
                if (UI::Screen::Project(worldCenter, viewProj, sCenter) &&
                    UI::Screen::Project(arrowEnd3,   viewProj, sEnd))
                {
                    cv.Arrow(sCenter, sEnd, colDir, 1.5f, 4.f);
                }
            }
        }
    }

    if (m_ShowFlowFieldDebug && !m_ActiveChunks.empty())
    {
        auto      cv       = UI::Foreground();
        glm::mat4 viewProj = m_Camera.GetViewProjection();

        const float half     = m_ChunkSize * 0.5f;
        const ImU32 colChunk = UI::Col32(255,  0,  0, 160);
        const ImU32 colLabel = UI::Col32(255, 80, 80, 255);

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

            glm::vec2 sc[4]; bool allVisible = true;
            for (int i = 0; i < 4; i++)
                if (!UI::Screen::Project(corners[i], viewProj, sc[i])) { allVisible = false; break; }
            if (!allVisible) continue;

            cv.Quad(sc[0], sc[1], sc[2], sc[3], colChunk, 2.f);

            glm::vec2 sCenter;
            if (UI::Screen::Project(worldCenter, viewProj, sCenter)) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%d,%d", coord.first, coord.second);
                cv.Text(sCenter, colLabel, buf);
            }
        }
    }

    // --- HEALTH BAR ---
    UI::HealthBar(m_PlayerHealth, m_MaxHealth,
                  UI::Screen::Pos() + glm::vec2(30.f, 40.f),
                  {200.f, 18.f}, "PLAYER HP");

    // --- GAME OVER OVERLAY ---
    if (m_PlayerHealth <= 0.0f)
    {
        glm::vec2 c  = UI::Screen::Center();
        auto      cv = UI::Foreground();

        cv.RectFill({c.x - 200.f, c.y - 70.f}, {c.x + 200.f, c.y + 70.f},
                    UI::Col32(0, 0, 0, 180), 10.f);
        cv.Rect    ({c.x - 200.f, c.y - 70.f}, {c.x + 200.f, c.y + 70.f},
                    UI::Col32(200, 0, 0, 200), 10.f, 2.f);

        const char* diedText    = "YOU DIED!";
        const char* respawnText = "Press 'R' to Respawn";
        float       bigSize     = ImGui::GetFontSize() * 2.0f;
        glm::vec2   sz1         = cv.CalcTextSize(diedText, bigSize);
        glm::vec2   sz2         = cv.CalcTextSize(respawnText);

        cv.Text({c.x - sz1.x,        c.y - 40.f}, UI::Col32(220,   0,   0, 255), diedText,    bigSize);
        cv.Text({c.x - sz2.x * 0.5f, c.y + 20.f}, UI::Col32(200, 200, 200, 220), respawnText);
    }

    DrawRadar();
}

void MainGameLayer::OnEvent(Aether::Event& event)
{
    m_Camera.OnEvent(event);

    if (event.GetEventType() == Aether::EventType::MouseButtonPressed &&
        Aether::Input::IsMouseButtonPressed(Aether::Mouse::MouseCode::Button0) &&
        m_PlayerHealth > 0.0f)
    {
        if (m_IsReloading)       { AE_WARN("Can't shoot while reloading!"); return; }
        if (m_CurrentAmmo <= 0)  { m_AmmoEmptyTimer = 1.0f; AE_WARN("Out of ammo! Press R"); return; }
        if (m_ShootTimer > 0.0f) return;

        m_CurrentAmmo--;
        m_ShootTimer = m_ShootDuration;
        if (m_CurrentAmmo == 0) m_AmmoEmptyTimer = 1.0f;

        if (m_Scene.IsValid(m_Gun))
        {
            Aether::Entity gunAnimEnt = FindAnimatorEntity(m_Scene, m_Gun);
            if (gunAnimEnt != Aether::Null_Entity)
            {
                auto& animComp = m_Scene.GetComponent<Aether::AnimatorComponent>(gunAnimEnt);
                animComp.CurrentTime = 0.0f;
                animComp.IsPlaying   = true;
            }
        }

        Aether::AudioSystem::Stop(m_GunSource);
        Aether::AudioSystem::SetVolume(m_GunSource, 0.3f);
        Aether::AudioSystem::Play(m_GunSource);

        glm::vec3          origin    = m_Camera.GetPosition();
        glm::vec3          direction = glm::normalize(m_Camera.GetForwardDirection());
        Aether::RaycastHit hit       = Aether::PhysicsSystem::CastRay(m_Scene.GetPhysicsInstance(), origin, direction, 100.0f);

        if (hit.Hit)
        {
            Aether::UUID   bodyID = Aether::PhysicsSystem::GetUUID(m_Scene.GetPhysicsInstance(), hit.HitEntityHandle);
            Aether::Entity target = m_Scene.FindEntity(bodyID);
            if (target != Aether::Null_Entity && target != m_Player)
            {
                m_Scene.DestroyHierarchy(target);
                m_ActiveZombies.erase(
                    std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), target),
                    m_ActiveZombies.end());
            }
        }
        event.Handled = true;
    }
}

void MainGameLayer::DrawRadar()
{
    using namespace Aether;

    const float radarRadius      = 100.0f;
    const float maxTrackDistance = 50.0f;

    glm::vec2 vpPos  = UI::Screen::Pos();
    glm::vec2 vpSize = UI::Screen::Size();
    glm::vec2 center = {
        vpPos.x + 20.0f + radarRadius,
        vpPos.y + vpSize.y - 20.0f - radarRadius
    };

    auto cv = UI::Foreground();

    cv.CircleFill(center, radarRadius,        UI::Col32( 10,  30, 10, 200));
    cv.Circle    (center, radarRadius,        UI::Col32(  0, 255,  0, 255), 64, 2.f);
    cv.Circle    (center, radarRadius * 0.5f, UI::Col32(  0, 180,  0,  80), 64, 1.f);
    cv.Line({center.x - radarRadius, center.y}, {center.x + radarRadius, center.y},
            UI::Col32(0, 180, 0, 60), 1.f);
    cv.Line({center.x, center.y - radarRadius}, {center.x, center.y + radarRadius},
            UI::Col32(0, 180, 0, 60), 1.f);

    if (m_Scene.IsValid(m_Player))
    {
        auto&     pTransform = m_Scene.GetComponent<TransformComponent>(m_Player);
        glm::vec3 pPos       = pTransform.Translation;
        float     cosA       = cosf(-m_Camera.GetYaw());
        float     sinA       = sinf(-m_Camera.GetYaw());

        for (auto zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;

            glm::vec3 zPos = m_Scene.GetComponent<TransformComponent>(zombie).Translation;
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

                cv.CircleFill(center + glm::vec2(ox, oy), 3.5f, UI::Col32(255, 50, 50, 255));
            }
        }

        cv.CircleFill(center, 5.0f, UI::Col32(255, 255, 255, 255));
        cv.TriangleFill(
            {center.x,        center.y - 9.0f},
            {center.x - 4.0f, center.y + 4.0f},
            {center.x + 4.0f, center.y + 4.0f},
            UI::Col32(100, 220, 255, 220));
    }

    cv.TextCentered({center.x, center.y - radarRadius - 29.f},
                    UI::Col32(0, 220, 0, 200), "RADAR", 24.f);
}