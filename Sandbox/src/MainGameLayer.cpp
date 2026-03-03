#include "MainGameLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>
#include <algorithm>
#include <imgui.h>
#include "Aether/Core/Log.h"

MainGameLayer::MainGameLayer()
    : Layer("Main Game"), m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(6.0f);
}

void MainGameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    // --- 1. SHADOW PASS ---
    Aether::FramebufferSpec shadowFbSpec;
    shadowFbSpec.Width       = 1024;
    shadowFbSpec.Height      = 1024;
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
    mainPass.OnScreen    = true;
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
    volPass.IsActive = false;
    volPass.readList      = {
        { "u_SceneColor", mainPass.TargetFBO->GetColorAttachment() },
        { "u_SceneDepth", mainPass.TargetFBO->GetDepthAttachment() },
        { "u_ShadowMap",  shadowPass.TargetFBO->GetDepthAttachment() }
    };

    m_Pipeline = { shadowPass, mainPass, volPass }; // Lưu vào biến member
    Aether::Renderer::SetPipeline(m_Pipeline);

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
    pTransform.Rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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
        m_Scene.AddComponent<Aether::ColliderComponent>(m_Player, m_PlayerBodyID, cfg.shape, cfg.size, glm::vec3(0.0f, 1.0f, 0.0f));
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

    m_MuzzleFlashTexture = Aether::Texture2D::Create("assets/models/tiadan.png");

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
           DestroyHierarchy(entity);
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

    // --- BƯỚC 1: CẬP NHẬT CAMERA TRƯỚC (Để lấy hướng nhìn mới nhất từ chuột) ---
    m_Camera.Update(ts);

    float camDistance = m_Camera.GetDistance();
    m_CurrentRenderDistance = m_BaseRenderDistance + static_cast<int>(camDistance / m_ZoomInfluence);
    m_CurrentRenderDistance = std::clamp(m_CurrentRenderDistance, 1, 30);

    if (m_Scene.IsValid(m_Player))
    {
        auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

        // --- BƯỚC 2: TÍNH TOÁN DI CHUYỂN DỰA TRÊN CAMERA ĐÃ UPDATE ---
        glm::vec3 camForward = m_Camera.GetForwardDirection();
        glm::vec3 camRight   = m_Camera.GetRightDirection();
        static float s_HeadBobTimer = 0.0f;
        static float s_BobAmplitudeBlend = 0.0f;

        camForward.y = 0.0f; camRight.y = 0.0f;
        if (glm::length(camForward) > 0.0f) camForward = glm::normalize(camForward);
        if (glm::length(camRight)   > 0.0f) camRight   = glm::normalize(camRight);

        glm::vec3 moveDir(0.0f);
        if (Aether::Input::IsKeyPressed(Aether::Key::W)) moveDir += camForward;
        if (Aether::Input::IsKeyPressed(Aether::Key::S)) moveDir -= camForward;
        if (Aether::Input::IsKeyPressed(Aether::Key::A)) moveDir -= camRight;
        if (Aether::Input::IsKeyPressed(Aether::Key::D)) moveDir += camRight;

        bool isMoving = glm::length(moveDir) > 0.0f;
        static bool wasMoving = false;
        if (isMoving != wasMoving)
        {
            if (isMoving) rigSystem->Play(m_RunAnimation);
            else          rigSystem->Pause(m_RunAnimation);
            wasMoving = isMoving;
        }

        if (isMoving)
        {
            moveDir = glm::normalize(moveDir);
            pTransform.Translation += moveDir * (m_PlayerSpeed * (float)ts);

            if (!m_FirstPerson)
            {                
                float targetAngle = glm::atan(moveDir.x, moveDir.z);
                glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                if (glm::dot(pTransform.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                float blend = 1.0f - glm::exp(-15.0f * (float)ts);
                
                // BỔ SUNG NORMALIZE CHO PLAYER TẠI ĐÂY:
                pTransform.Rotation = glm::normalize(glm::slerp(pTransform.Rotation, targetRot, blend));
            }
            pTransform.Dirty = true;

            // Tăng timer để tạo nhịp bước đi (số 12.0f là tốc độ nhịp, có thể chỉnh)
            s_HeadBobTimer += (float)ts * m_bobSpeed; 
            // Tăng dần biên độ lắc lư lên 1.0 (mượt)
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 1.0f, (float)ts * 10.0f);
        }
        else 
        {
            // Khi dừng lại, giảm dần biên độ lắc lư về 0 để camera không bị giật
            s_BobAmplitudeBlend = glm::mix(s_BobAmplitudeBlend, 0.0f, (float)ts * 10.0f);
            
            // Reset timer về 0 nếu đã đứng yên hẳn để lần sau bước đi bắt đầu từ nhịp chuẩn
            if (s_BobAmplitudeBlend < 0.01f) {
                s_BobAmplitudeBlend = 0.0f;
                s_HeadBobTimer = 0.0f;
            }
        }
        
        // TÍNH TOÁN ĐỘ NẢY Y (LÊN/XUỐNG)
        // Góc nhìn thứ nhất nảy mạnh hơn (0.08f) so với góc nhìn thứ 3 (0.03f) để đỡ chóng mặt
        float targetAmplitude = m_FirstPerson ? m_bobStrength : m_bobStrength / 2.0f;
        // Dùng hàm sin để tạo sóng nảy. Hàm trị tuyệt đối (abs) tạo cảm giác nhún theo mỗi bước chân
        float bobOffsetY = glm::abs(glm::sin(s_HeadBobTimer)) * targetAmplitude * s_BobAmplitudeBlend;

        // --- BƯỚC 3: ĐỒNG BỘ CAMERA VỚI VỊ TRÍ MỚI CỦA PLAYER ---
        glm::vec3 playerTopPos = pTransform.Translation + glm::vec3(0.0f, 1.0f + bobOffsetY, 0.0f);
        glm::vec3 playerEyePos = pTransform.Translation + glm::vec3(0.0f, 1.7f + bobOffsetY, 0.0f);

        if (m_FirstPerson)
        {
            pTransform.Scale = {0.001f, 0.001f, 0.001f};
            m_Camera.SetDistance(0.5f);
            m_Camera.SetFocalPoint(playerEyePos); // Đã có hiệu ứng nảy
            
            pTransform.Rotation = glm::quat(glm::vec3(0.0f, -m_Camera.GetYaw(), 0.0f));
            pTransform.Dirty    = true;
        }
        else
        {
            pTransform.Scale = {1.0f, 1.0f, 1.0f};
            glm::vec3 shoulderOffset = m_Camera.GetRightDirection() * 0.5f; // Bạn đã viết dòng này
            m_Camera.SetFocalPoint(playerTopPos + shoulderOffset);         // Cập nhật FocalPoint có offset

            if (m_LockCamera)
            {
                m_Camera.SetDistance(5.0f); // Chỉnh lại khoảng cách cho vừa tầm nhìn
                if (m_Camera.GetPitch() < 0.2f) m_Camera.SetPitch(0.2f);
            }
        }

        // --- LOGIC NẠP ĐẠN ---
        if (Aether::Input::IsKeyPressed(Aether::Key::R) && !m_IsReloading && m_CurrentAmmo < m_MaxAmmo)
        {
            m_IsReloading = true;
            m_ReloadTimer = m_ReloadDuration;
            AE_INFO("Reloading...");
        }

        if (m_IsReloading)
        {
            m_ReloadTimer -= (float)ts;
            m_ReloadRotation += (float)ts * 7.0f; // Tốc độ xoay của vòng tròn

            if (m_ReloadTimer <= 0.0f)
            {
                m_CurrentAmmo = m_MaxAmmo; // Hồi đầy đạn (vô tận dự trữ)
                m_IsReloading = false;
                AE_INFO("Reload Complete!");
            }
        }

        // --- BƯỚC 4: CÁC HỆ THỐNG PHỤ THUỘC VÀ AI ---
        if (m_SunLight != Aether::Null_Entity && m_Scene.IsValid(m_SunLight))
        {
            auto& lightTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_SunLight);
            auto& lightComp = m_Scene.GetComponent<Aether::LightComponent>(m_SunLight);

            // Giữ ánh sáng luôn đi theo Player để đổ bóng ổn định
            lightTransform.Translation = playerTopPos + glm::vec3(0.0f, 50.0f, 0.0f);
            
            // Bạn có thể chỉnh hướng nắng xiên để bóng đổ dài hơn (nhìn kinh dị hơn)
            // lightTransform.Rotation = glm::quat(glm::vec3(glm::radians(-60.0f), glm::radians(45.0f), 0.0f));
            
            lightTransform.Dirty = true;
            lightComp.Config.castShadows = true; // Đảm bảo bóng đổ luôn bật
        }

        UpdateMapChunks(pTransform.Translation);

        m_FlowFieldTimer += (float)ts;
        if (m_FlowFieldTimer >= 0.2f)
        {
            UpdateFlowField(pTransform.Translation);
            m_FlowFieldTimer = 0.0f;
        }

        // Thêm một biến thời gian cục bộ hoặc dùng biến toàn cục của Engine nếu có để tính toán dao động
        static float s_TimeAccumulator = 0.0f;
        s_TimeAccumulator += (float)ts;

        // =========================================================
        // HỆ THỐNG QUẢN LÝ QUẦN THỂ ZOMBIE (DESPAWN & AMBIENT SPAWN)
        // =========================================================
        
        // Tính toán bán kính thực tế dựa trên Render Distance của Chunk
        float actualChunkSize = m_ChunkSize * 2.0f; 
        float despawnRadius = (m_CurrentRenderDistance * actualChunkSize) + (actualChunkSize * 1.5f);
        float despawnRadiusSq = despawnRadius * despawnRadius;

        // 1. DỌN RÁC: Tiêu diệt Zombie lọt ra khỏi bán kính
        for (auto it = m_ActiveZombies.begin(); it != m_ActiveZombies.end(); )
        {
            Aether::Entity zombie = *it;
            if (!m_Scene.IsValid(zombie)) {
                it = m_ActiveZombies.erase(it);
                continue;
            }

            auto& zT = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
            glm::vec3 diffToPlayer = pTransform.Translation - zT.Translation;
            diffToPlayer.y = 0.0f;

            if (glm::dot(diffToPlayer, diffToPlayer) > despawnRadiusSq)
            {
                // Tiêu huỷ toàn bộ dữ liệu của zombie này để giải phóng RAM/CPU
                if (m_ZombieAnimators.count(zombie)) {
                    rigSystem->DestroyAnimator(m_ZombieAnimators[zombie]);
                    m_ZombieAnimators.erase(zombie);
                }
                if (m_ZombieBodyIDs.count(zombie)) {
                    Aether::PhysicsSystem::DestroyBody(m_ZombieBodyIDs[zombie]);
                    m_ZombieBodyIDs.erase(zombie);
                }
                DestroyHierarchy(zombie);
                it = m_ActiveZombies.erase(it); // Rút khỏi danh sách
            }
            else {
                ++it;
            }
        }

        // 2. PHỤC KÍCH: Tự động đẻ Zombie ở rìa sương mù khi Player đứng yên
        static float s_SpawnTimer = 0.0f;
        s_SpawnTimer += (float)ts;
        
        // Cứ mỗi 1 giây kiểm tra 1 lần, nếu thiếu thì bù đắp
        if (s_SpawnTimer >= 1.0f)
        {
            s_SpawnTimer = 0.0f;
            if (m_ActiveZombies.size() < 50) 
            {
                // Random 1 góc bất kỳ 360 độ xung quanh Player
                float randomAngle = glm::radians((float)(std::rand() % 360));
                
                // Spawn ngay mép ngoài của Render Distance để Player không thấy nó "pop" ra giữa màn hình
                float spawnDist = (m_CurrentRenderDistance * actualChunkSize) + (actualChunkSize * 0.5f);
                
                glm::vec3 spawnOffset = glm::vec3(glm::cos(randomAngle), 0.0f, glm::sin(randomAngle)) * spawnDist;
                glm::vec3 spawnPos = pTransform.Translation + spawnOffset;
                spawnPos.y = -1.75f; // Chạm đất

                SpawnZombie(spawnPos);
            }
        }
        // =========================================================
        
        static uint32_t s_ZombieUpdateCounter = 0;
        s_ZombieUpdateCounter++;
        int currentZombieIndex = 0;

        for (Aether::Entity zombie : m_ActiveZombies)
        {
            if (!m_Scene.IsValid(zombie)) continue;
            currentZombieIndex++;

            if (currentZombieIndex % 2 == s_ZombieUpdateCounter % 2)
            {
                auto& zT = m_Scene.GetComponent<Aether::TransformComponent>(zombie);
                uint32_t zSeed = (uint32_t)zombie;

                int zX = static_cast<int>(std::round(zT.Translation.x / m_PathGridSize));
                int zZ = static_cast<int>(std::round(zT.Translation.z / m_PathGridSize));
                auto zCoord = std::make_pair(zX, zZ);

                // 1. HƯỚNG ĐI CƠ BẢN (Từ FlowField hoặc hướng thẳng Player)
                glm::vec3 baseDir(0.0f, 0.0f, 1.0f); // Luôn có hướng mặc định
                if (m_FlowField.find(zCoord) != m_FlowField.end() && glm::length(m_FlowField[zCoord].direction) > 0.001f) {
                    baseDir = m_FlowField[zCoord].direction;
                } else {
                    glm::vec3 diffBase = pTransform.Translation - zT.Translation;
                    diffBase.y = 0.0f;
                    if (glm::length(diffBase) > 0.001f) {
                        baseDir = glm::normalize(diffBase);
                    }
                }
                
                // Tạo một vector vuông góc với hướng đi hiện tại
                glm::vec3 rightDir = glm::vec3(-baseDir.z, 0.0f, baseDir.x);
                
                // Tính toán độ lảo đảo bằng sóng Sine (Tần số và biên độ có thể tùy chỉnh)
                float wobble = glm::sin(s_TimeAccumulator * 2.5f + zSeed) * 0.35f; 
                
                // 3. TRÁNH NÉ LẪN NHAU (Tối ưu hóa cực mạnh)
                glm::vec3 separationForce(0.0f);
                float sepRadius = 0.8f;
                float sepRadiusSq = sepRadius * sepRadius; // 0.64f
                int neighborCount = 0;

                for (Aether::Entity other : m_ActiveZombies) {
                    if (other == zombie || !m_Scene.IsValid(other)) continue;
                    auto& otherT = m_Scene.GetComponent<Aether::TransformComponent>(other);
                    glm::vec3 diff = zT.Translation - otherT.Translation;
                    diff.y = 0.0f;
                    
                    // Dùng dot product để lấy bình phương khoảng cách (tránh dùng hàm sqrt đắt đỏ)
                    float distSq = glm::dot(diff, diff); 
                    
                    if (distSq > 0.001f && distSq < sepRadiusSq) { 
                        // CHỈ tính sqrt khi tụi nó thực sự dính vào nhau
                        float dist = glm::sqrt(distSq); 
                        // Tối ưu hóa phép normalize: diff / dist chính là normalize(diff)
                        separationForce += (diff / dist) * (sepRadius - dist); 
                        neighborCount++;
                    }
                }
                
                // Nếu có quá nhiều zombie bu lại (neighborCount lớn), chia đều lực đẩy ra 
                // để tụi nó không bị cộng dồn lực đẩy văng xuyên tường hay văng lên trời
                if (neighborCount > 0) {
                    separationForce /= (float)neighborCount;
                }

                // 4. TỔNG HỢP HƯỚNG ĐI CUỐI CÙNG
                glm::vec3 totalForce = baseDir + rightDir * wobble + separationForce * 0.5f;
                glm::vec3 finalMoveDir = baseDir; // Mặc định lấy hướng gốc để phòng hờ

                // CHỐNG LỖI CHIA CHO 0 (Ngăn chặn Zombie biến thành quái vật khổng lồ)
                if (glm::length(totalForce) > 0.001f) {
                    finalMoveDir = glm::normalize(totalForce);
                }

                // 5. TỐC ĐỘ NGẪU NHIÊN CHO TỪNG CON
                // Thay đổi tốc độ từ 80% đến 120% tốc độ gốc dựa theo ID
                float randomSpeedMod = 0.8f + ((zSeed % 100) / 100.0f) * 0.4f; 
                float actualSpeed = m_ZombieSpeed * randomSpeedMod;

                // Tính khoảng cách tới Player
                glm::vec3 diffToPlayer = pTransform.Translation - zT.Translation;
                diffToPlayer.y = 0.0f;

                // CHẠY & XOAY (Bẻ lái mượt mà)
                if (glm::length(diffToPlayer) > 1.2f)
                {
                    // 1. Tính toán góc xoay lý thuyết (hướng nó muốn đi)
                    float targetAngle = glm::atan(finalMoveDir.x, finalMoveDir.z);
                    glm::quat targetRot = glm::quat(glm::vec3(0.0f, targetAngle, 0.0f));
                    if (glm::dot(zT.Rotation, targetRot) < 0.0f) targetRot = -targetRot;
                    
                    // 2. Làm mượt góc xoay & CHỐNG LỖI QUATERNION DRIFT (Phình to quái vật)
                    // Ép normalize để độ dài quaternion luôn = 1.0, không bị scale mesh
                    zT.Rotation = glm::normalize(glm::slerp(zT.Rotation, targetRot, 1.0f - glm::exp(-5.0f * (float)ts))); 
                    
                    // 3. QUAN TRỌNG NHẤT: Ép hướng nhìn luôn song song mặt đất
                    glm::vec3 facing = zT.Rotation * glm::vec3(0.0f, 0.0f, 1.0f);
                    facing.y = 0.0f; // Bắt buộc trục Y = 0 để zombie không trôi lên trời
                    
                    if (glm::length(facing) > 0.001f) {
                        glm::vec3 currentFacingDir = glm::normalize(facing);
                        zT.Translation += currentFacingDir * (actualSpeed * (float)ts);
                        
                        // Khoá cứng toạ độ Y của Zombie bằng với lúc mới đẻ ra (không lún đất)
                        zT.Translation.y = -1.75f; 
                    }
                    zT.Dirty    = true;
                }
            }
        }
    }

    // --- BƯỚC 5: SHADER UNIFORMS VÀ VŨ KHÍ ---
    m_VolShader->Bind();
    m_VolShader->SetFloat("u_Density",    m_VolDensity);
    m_VolShader->SetFloat("u_Intensity",  m_VolIntensity);
    m_VolShader->SetInt  ("u_Steps",      m_VolSteps);
    m_VolShader->SetFloat("u_VolBias",    m_ShadowBias);
    m_VolShader->SetFloat("u_MaxDistance", 100.0f);

    m_MainShader->Bind();
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);

    m_MainShader->SetInt("u_FogMode", m_FogMode);
    m_MainShader->SetFloat3("u_FogColor", m_FogColor);
    m_MainShader->SetFloat("u_FogDensity", m_FogDensity);
    m_MainShader->SetFloat("u_FogStart", m_FogStart);
    m_MainShader->SetFloat("u_FogEnd", m_FogEnd);
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);


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

    // Cuối cùng mới Render Scene với dữ liệu Cam và Player đã đồng bộ tuyệt đối
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

    // --- TẠO MỚI ---
    for (int x = -m_CurrentRenderDistance; x <= m_CurrentRenderDistance; ++x) {
        for (int z = -m_CurrentRenderDistance; z <= m_CurrentRenderDistance; ++z) {
            int targetX = currentX + x;
            int targetZ = currentZ + z;
            auto coord  = std::make_pair(targetX, targetZ);
            chunksToKeep.push_back(coord);

            if (m_ActiveChunks.find(coord) == m_ActiveChunks.end()) {
                // 1. Tạo thực thể đất
                Aether::Entity chunk = m_Scene.CreateEntity("MapGrid_" + std::to_string(targetX) + "_" + std::to_string(targetZ));
                auto& t = m_Scene.GetComponent<Aether::TransformComponent>(chunk);
                float yOffset = -(actualChunkSize / 2.0f);
                t.Translation = glm::vec3(targetX * actualChunkSize, yOffset, targetZ * actualChunkSize);
                t.Scale       = {myScaleXZ, 1.0f, myScaleXZ};
                t.Dirty       = true;

                auto& chunkcmp   = m_Scene.AddComponent<Aether::MeshComponent>(chunk);
                chunkcmp.MeshPtr   = m_BaseMapMesh;
                chunkcmp.Materials = {m_BaseMapMaterial};

                // 2. Khởi tạo dữ liệu Chunk
                ChunkData newData;
                newData.landEntity = chunk;

                // 3. Spawn Zombie và TRÓI BUỘC vào Chunk này
                if (std::rand() % 100 < 15 && (std::abs(x) > 2 || std::abs(z) > 2)) {
                    glm::vec3 spawnPos = t.Translation;
                    spawnPos.y = -1.75f;
                    
                    // Gọi SpawnZombie và nhận về Entity (Cần đổi SpawnZombie sang trả về Entity)
                    Aether::Entity zEnt = SpawnZombie(spawnPos);
                    if (zEnt != Aether::Null_Entity) {
                        newData.zombies.push_back(zEnt);
                    }
                }
                m_ActiveChunks[coord] = newData;
            }
        }
    }

    for (auto it = m_ActiveChunks.begin(); it != m_ActiveChunks.end(); ) {
        bool keep = false;
        for (const auto& c : chunksToKeep) { if (c == it->first) { keep = true; break; } }

        if (!keep) {
            for (Aether::Entity zombie : it->second.zombies) {
                if (m_Scene.IsValid(zombie)) {
                    m_ZombieAnimators.erase(zombie);
                    m_ZombieBodyIDs.erase(zombie);
                    m_ActiveZombies.erase(std::remove(m_ActiveZombies.begin(), m_ActiveZombies.end(), zombie), m_ActiveZombies.end());
                    DestroyHierarchy(zombie);
                }
            }
            // Xóa mảnh đất
            if (m_Scene.IsValid(it->second.landEntity))
                m_Scene.DestroyEntity(it->second.landEntity);
                
            it = m_ActiveChunks.erase(it);
        } else {
            ++it;
        }
    }
}

// ==========================================
// HÀM ĐẺ ZOMBIE — Mỗi con có animator riêng
// ==========================================
Aether::Entity MainGameLayer::SpawnZombie(const glm::vec3& position)
{
    static uint32_t s_ZombieCounter = 0;
    if (m_ActiveZombies.size() >= 50) return Aether::Null_Entity;
    s_ZombieCounter++;

    // 1. Animator
    Aether::UUID newAnimID = Aether::AssetsRegister::Register("ZombieAnim_" + std::to_string(s_ZombieCounter));
    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    if (rigSystem) {
        rigSystem->CloneAnimator(newAnimID, m_ZombieRunAnimation);
        rigSystem->BindClip(newAnimID, 4);
        rigSystem->Play(newAnimID);
    }

    // 2. Trick LoadHierarchy (Giữ nguyên phần này vì nó giúp load "thân xác" ổn nhất)
    Aether::UUID originalAnimID = m_ZombieSceneData.animatorIDS[0];
    m_ZombieSceneData.animatorIDS[0] = newAnimID;

    // 3. Entity
    Aether::Entity newZombie = m_Scene.CreateEntity("Zombie_Minion");
    auto& zTransform = m_Scene.GetComponent<Aether::TransformComponent>(newZombie);
    zTransform.Translation = position;
    zTransform.Scale       = {1.0f, 1.0f, 1.0f};
    zTransform.Rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    zTransform.Dirty       = true;

    m_Scene.LoadHierarchy(m_ZombieSceneData, newZombie);
    m_ZombieSceneData.animatorIDS[0] = originalAnimID;

    Aether::UUID bodyID = m_Scene.GetComponent<Aether::IDComponent>(newZombie).ID;
    {
        Aether::BodyConfig cfg;
        cfg.motionType  = Aether::MotionType::Kinematic;
        cfg.shape       = Aether::ColliderShape::Capsule;
        cfg.size        = glm::vec3(0.35f, 0.9f, 0.0f);
        cfg.transform   = { position, glm::quat(1,0,0,0) };
        cfg.offset = glm::vec3(0.0f, 1.0f, 0.0f);
        Aether::PhysicsSystem::CreateBody(bodyID, cfg);
        
        auto& col = m_Scene.AddComponent<Aether::ColliderComponent>(newZombie, bodyID, cfg.shape, cfg.size, glm::vec3(0.0f, 1.0f, 0.0f));
        //col.Visible = true;
    }

    // 5. Quản lý
    m_ZombieAnimators[newZombie] = newAnimID; 
    m_ZombieBodyIDs[newZombie]   = bodyID;
    m_ActiveZombies.push_back(newZombie);

    return newZombie; // Phải trả về để hàm UpdateMapChunks lưu lại
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

    // 3. Tạo vector hướng cho mỗi ô (LÀM MƯỢT BẰNG GRADIENT VECTOR)
    for (auto& pair : m_FlowField)
    {
        auto current = pair.first;
        if (pair.second.cost >= 255 || pair.second.bestCost == 999999) continue;

        glm::vec3 averageDir(0.0f);
        
        // Thay vì chỉ trỏ vào 1 ô duy nhất, ta tính tổng lực hút từ TẤT CẢ các ô xung quanh có cost thấp hơn.
        // Ô nào càng gần Player (bestCost càng nhỏ) thì lực hút về hướng đó càng mạnh.
        for (auto& offset : neighbors)
        {
            auto neighborCoord = std::make_pair(current.first + offset.first, current.second + offset.second);
            if (m_FlowField.find(neighborCoord) != m_FlowField.end())
            {
                int neighborCost = m_FlowField[neighborCoord].bestCost;
                if (neighborCost < pair.second.bestCost) // Nếu đi hướng này gần hơn
                {
                    // Lực hút = độ chênh lệch chi phí
                    float pullStrength = float(pair.second.bestCost - neighborCost);
                    glm::vec3 dirToNeighbor = glm::normalize(glm::vec3((float)offset.first, 0.0f, (float)offset.second));
                    averageDir += dirToNeighbor * pullStrength;
                }
            }
        }

        if (glm::length(averageDir) > 0.01f) {
            pair.second.direction = glm::normalize(averageDir); // Ra được hướng trơn tru (ví dụ 22.5 độ)
        } else {
            pair.second.direction = glm::vec3(0.0f);
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

    ImGui::Begin("Weapon Adjustment");
    ImGui::DragFloat3("Muzzle Offset", glm::value_ptr(m_MuzzleOffset), 0.01f);
    ImGui::End();

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawLightingPanel();

    // ============================================================================
    // --- PHẦN MỚI: UI HIỂN THỊ ĐẠN (Góc dưới bên phải) ---
    // ============================================================================
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos = ImVec2(viewport->Pos.x + viewport->Size.x - 180, viewport->Pos.y + viewport->Size.y - 100);
    ImGui::SetNextWindowPos(window_pos);
    
    // Tạo một cửa sổ trong suốt không tiêu đề
    ImGui::Begin("AmmoDisplay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "WEAPON: PISTOL"); 
    
    if (m_IsReloading) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "RELOADING...");
    } else {
        // Hiển thị: 30 / INF (Đạn vô tận)
        std::string ammoText = std::to_string(m_CurrentAmmo) + " / INF";
        ImGui::SetWindowFontScale(1.5f); // Làm chữ to hơn
        ImGui::Text("%s", ammoText.c_str());
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();


    // ============================================================================
    // --- PHẦN MỚI: VẼ TÂM NGẮM (Biến hình khi Reload) ---
    // ============================================================================
    ImVec2 center = ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, 
                           viewport->Pos.y + viewport->Size.y * 0.5f);
    
    auto drawList = ImGui::GetForegroundDrawList();
    float thickness = 2.0f;
    ImU32 green = IM_COL32(0, 255, 0, 255);
    ImU32 white = IM_COL32(255, 255, 255, 255);

    if (m_IsReloading)
    {
        // --- TÂM HÌNH TRÒN XOAY (Khi đang nạp đạn) ---
        float radius = 15.0f;
        int numSegments = 8; // Số lượng vạch trong vòng tròn
        for (int i = 0; i < numSegments; i++)
        {
            // Tính toán góc cho từng vạch cộng thêm biến xoay m_ReloadRotation
            float angle = m_ReloadRotation + (i * ((2.0f * 3.14159f) / numSegments));
            
            // Vẽ các đoạn thẳng ngắn tạo thành vòng tròn
            ImVec2 p1 = ImVec2(center.x + cos(angle) * (radius - 5), center.y + sin(angle) * (radius - 5));
            ImVec2 p2 = ImVec2(center.x + cos(angle) * radius, center.y + sin(angle) * radius);
            
            drawList->AddLine(p1, p2, white, thickness);
        }
        // Vẽ thêm chấm đỏ mờ ở giữa để định hướng
        drawList->AddCircleFilled(center, 1.5f, IM_COL32(255, 0, 0, 150));
    }
    else
    {
        // --- TÂM 4 VẠCH (Khi bình thường) ---
        // Theo ý bạn: Bắn không bị giật ra (chỉ giật khi di chuyển nếu muốn)
        static float crosshairSpread = 0.0f;
        bool isMoving = glm::length(m_PlayerVelocity) > 0.1f; 
        
        // Độ giãn chỉ thay đổi theo việc di chuyển, không liên quan đến việc bắn
        if (isMoving) crosshairSpread = glm::mix(crosshairSpread, 12.0f, 0.1f);
        else          crosshairSpread = glm::mix(crosshairSpread, 0.0f, 0.1f);

        float baseLength = 10.0f;
        float offset = 5.0f + crosshairSpread;

        // Vạch Trái
        drawList->AddLine(ImVec2(center.x - offset - baseLength, center.y), ImVec2(center.x - offset, center.y), green, thickness);
        // Vạch Phải
        drawList->AddLine(ImVec2(center.x + offset, center.y), ImVec2(center.x + offset + baseLength, center.y), green, thickness);
        // Vạch Trên
        drawList->AddLine(ImVec2(center.x, center.y - offset - baseLength), ImVec2(center.x, center.y - offset), green, thickness);
        // Vạch Dưới
        drawList->AddLine(ImVec2(center.x, center.y + offset), ImVec2(center.x, center.y + offset + baseLength), green, thickness);
        
        // Chấm nhỏ cố định ở tâm
        drawList->AddCircleFilled(center, 1.5f, white);
    }
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

    if (ImGui::CollapsingHeader("Atmosphere & Fog", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("Fog Color", glm::value_ptr(m_FogColor));
        ImGui::SliderFloat("Fog Density", &m_VolDensity, 0.0f, 0.1f);
        ImGui::SliderFloat("Fog Intensity", &m_VolIntensity, 0.0f, 5.0f);
    }

    if (ImGui::CollapsingHeader("Camera Movement"))
    {
        ImGui::SliderFloat("Bob Speed", &m_bobSpeed, 5.0f, 20.0f);
        ImGui::SliderFloat("Bob Strength", &m_bobStrength, 0.01f, 0.2f);
    }

    ImGui::End();
}

void MainGameLayer::OnEvent(Aether::Event& event)
{
    m_Camera.OnEvent(event);
    auto& pTransform = m_Scene.GetComponent<Aether::TransformComponent>(m_Player);

    // --- PHÍM V: CHUYỂN GÓC NHÌN ---
    if (event.GetEventType() == Aether::EventType::KeyPressed)
    {
        if (Aether::Input::IsKeyPressed(Aether::Key::V))
        {
            m_FirstPerson = !m_FirstPerson;
            if (m_FirstPerson) {
                pTransform.Scale = {0.001f, 0.001f, 0.001f};
                m_Camera.SetDistance(0.5f);
            } else {
                pTransform.Scale = {1.0f, 1.0f, 1.0f};
                m_Camera.SetDistance(6.0f);
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
                m_Camera.SetDistance(6.0f);
                event.Handled = true;
                return;
            }
            event.Handled = true;
            return;
        }
    }

    // Tìm đoạn này trong MainGameLayer.cpp -> OnEvent
    if (event.GetEventType() == Aether::EventType::MouseButtonPressed)
    {
        if (Aether::Input::IsMouseButtonPressed(Aether::Mouse::Button0))
        {
            if (m_IsReloading) { 
                AE_WARN("Cant shoot while reloading!");
                return; 
            }
            if (m_CurrentAmmo <= 0) {
                AE_WARN("Out of ammo! Press R");
                return;
            }
            m_CurrentAmmo--;

            if (!m_Scene.IsValid(m_Gun)) return;
            auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
            rigSystem->Play(m_ShootAnimation);

            glm::vec3 origin = m_Camera.GetPosition();
            glm::vec3 direction = glm::normalize(m_Camera.GetForwardDirection());
            float maxRange = 100.0f;

            Aether::RaycastHit hit = Aether::PhysicsSystem::CastRay(origin, direction, maxRange);

            if (hit.Hit)
            {
                Aether::Entity entity = m_Scene.FindEntity(hit.HitEntityID);
                if (entity != Aether::Null_Entity && entity != m_Player)
                {
                    DestroyHierarchy(entity);
                    m_ActiveZombies.erase(std::remove(m_ActiveZombies.begin(),m_ActiveZombies.end(),entity),m_ActiveZombies.end());
                }
            }


            event.Handled = true;
        }
    }
}

void MainGameLayer::DestroyHierarchy(Aether::Entity entity)
{
    if (!m_Scene.IsValid(entity)) return;

    // 1. Tìm và xóa tất cả các node con (Mesh, Bones, v.v.)
    auto& hierarchy = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);
    Aether::Entity child = hierarchy.firstChild;
    while (child != Aether::Null_Entity)
    {
        // Phải lấy sibling trước khi xóa con
        Aether::Entity next = m_Scene.GetComponent<Aether::HierarchyComponent>(child).nextSibling;
        DestroyHierarchy(child); // Đệ quy xóa node con
        child = next;
    }

    // 2. Sau khi con đã sạch, mới xóa chính node cha
    m_Scene.DestroyEntity(entity);
}