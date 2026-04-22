#include "LabLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

LabLayer::LabLayer()
    : Layer("Lab Layer")
    , m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(5.0f);
}

// =============================================================================
//  Attach / Detach
// =============================================================================

void LabLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    auto& window = Aether::Application::Get().GetWindow();

    // --- MAIN PASS ---
    Aether::FramebufferSpec mainSpec;
    mainSpec.Width       = window.GetWidth();
    mainSpec.Height      = window.GetHeight();
    mainSpec.Attachments = { Aether::ImageFormat::RGBA8, Aether::ImageFormat::DEPTH24STENCIL8 };
    m_MainFbo = Aether::FrameBuffer::Create(mainSpec);

    m_MainShader = Aether::Shader::Create("assets/shaders/Standard.shader");
    m_MainShader->Bind();
    m_MainShader->SetUBOSlot("Camera", 0);
    m_MainShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass mainPass;
    mainPass.TargetFBO   = m_MainFbo.get();
    mainPass.Shader      = m_MainShader.get();
    mainPass.ClearColor  = true;
    mainPass.ClearDepth  = true;
    mainPass.UsingSkybox = true;
    mainPass.ClearValue  = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    mainPass.CullFace    = Aether::State::BACK_CULL;
    mainPass.OnScreen    = false;
    mainPass.UsingShadowmap = true;

    // --- VOLUMETRIC PASS ---
    Aether::FramebufferSpec volSpec;
    volSpec.Width       = mainSpec.Width;
    volSpec.Height      = mainSpec.Height;
    volSpec.Attachments = { Aether::ImageFormat::RGBA8, Aether::ImageFormat::DEPTH24STENCIL8 };
    m_VolFbo = Aether::FrameBuffer::Create(volSpec);

    m_VolShader = Aether::Shader::Create("assets/shaders/Volumetric.shader");
    m_VolShader->Bind();
    m_VolShader->SetUBOSlot("Camera", 0);
    m_VolShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass volPass;
    volPass.TargetFBO     = m_VolFbo.get();
    volPass.Shader        = m_VolShader.get();
    volPass.ClearColor    = true;
    volPass.ClearDepth    = true;
    volPass.CullFace      = Aether::State::None;
    volPass.OnScreen      = true;
    volPass.UsingGeometry = false;
    volPass.UsingShadowmap = true;
    volPass.readList      = {
        { "u_SceneColor", m_MainFbo->GetColorAttachment()  },
        { "u_SceneDepth", m_MainFbo->GetDepthAttachment()  }
    };

    m_Pipeline = { mainPass, volPass };
    Aether::Renderer::SetPipeline(m_Pipeline);

    // --- SKYBOX ---
    Aether::Renderer::SetSkyBox("assets/textures/skybox.png");

    // --- SPOT LIGHT ---
    Aether::LightParam spotLight;
    spotLight.type        = Aether::LightType::Spot;
    spotLight.position    = glm::vec3(0.0f, 5.0f, 0.0f);
    spotLight.direction   = glm::vec3(0.0f, -1.0f, 0.0f);
    spotLight.color       = glm::vec3(1.0f);
    spotLight.intensity   = 3.0f;
    spotLight.range       = 100.0f;
    spotLight.innerCone   = glm::cos(glm::radians(12.5f));
    spotLight.outerCone   = glm::cos(glm::radians(25.0f));
    spotLight.castShadows = true;

    m_LightEntity = m_Scene.CreateEntity("Spotlight");
    m_Scene.AddComponent<Aether::LightComponent>(m_LightEntity).Config = spotLight;
    auto& lightTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_LightEntity);
    lightTransform.Translation = spotLight.position;
    lightTransform.Dirty       = true;

    // --- CONSOLE COMMANDS ---
    Aether::ConsoleLayer::RegisterCommand("load", AE_BIND_CONSOLE_FN(LoadModelAsync)); 
    Aether::ConsoleLayer::RegisterCommand("loadcache", AE_BIND_CONSOLE_FN(LoadCacheModelAsync));
    Aether::ConsoleLayer::RegisterCommand("add",  AE_BIND_CONSOLE_FN(AddEntity));

    AE_CORE_INFO("LabLayer initialized!");
}

void LabLayer::Detach()
{
    for (auto& [entity, entry] : m_PhysicsBodies)
        Aether::PhysicsSystem::DestroyBody(entry.handle);
    m_PhysicsBodies.clear();

    m_MainShader.reset();
    m_VolShader.reset();
    m_MainFbo.reset();
    m_VolFbo.reset();

    m_MeshIDs.clear();
    // NOTE: m_AnimatorIDs removed — no longer tracked as a separate list.
}

// =============================================================================
//  Async model loading
// =============================================================================

void LabLayer::AddEntity(const std::vector<std::string>& args)
{
    for (auto& name : args)
        m_Scene.CreateEntity(name);
}

void LabLayer::LoadModelAsync(const std::vector<std::string>& args)
{
    if (args.empty()) return;
    std::string path = args[0];

    if (args.size() == 1)
    {
        Aether::JobSystem::SubmitJob([this, path]()
        {
            AE_CORE_INFO("Worker: Parsing {0}", path);
            auto parsed = Aether::Importer::Import(path);
            {
                std::lock_guard<std::mutex> lock(m_ParseMutex);
                m_CompletedParses.push(parsed);
            }
            AE_CORE_INFO("Worker: Parsing complete for {0}", path);
        });
    }
    if (args.size() == 2)
    {
        std::string name = args[1];
        Aether::JobSystem::SubmitJob([this, path, name]()
        {
            AE_CORE_INFO("Worker: Parsing {0}", path);
            auto parsed = Aether::Importer::Import(path, true, name.c_str());
            {
                std::lock_guard<std::mutex> lock(m_ParseMutex);
                m_CompletedParses.push(parsed);
            }
            AE_CORE_INFO("Worker: Parsing complete for {0}", path);
        });
    }
}

void LabLayer::LoadCacheModelAsync(const std::vector<std::string>& args)
{
    if (args.empty()) return;
    std::string name = args[0];

    Aether::JobSystem::SubmitJob([this, name]()
    {
        AE_CORE_INFO("Worker: Loading cache {0}", name);
        auto parsed = Aether::Importer::ImportCache(name.c_str());
        {
            std::lock_guard<std::mutex> lock(m_ParseMutex);
            m_CompletedParses.push(parsed);
        }
        AE_CORE_INFO("Worker: Load cache complete for {0}", name);
    });
}

void LabLayer::DrainParseQueue()
{
    std::queue<Aether::Ref<Aether::ParsedScene>> localQueue;
    {
        std::lock_guard<std::mutex> lock(m_ParseMutex);
        std::swap(localQueue, m_CompletedParses);
    }

    while (!localQueue.empty())
    {
        auto parsed = localQueue.front();
        localQueue.pop();

        AE_CORE_INFO("Main thread: Uploading to GPU...");
        auto result = Aether::Importer::Upload(parsed);
        for (auto& meshID : result.meshIDs) m_MeshIDs.push_back(meshID);

        m_Scene.LoadHierarchy(result);

        AE_CORE_INFO("Loaded: {0} mesh(es)", result.meshIDs.size());
    }
}

// =============================================================================
//  Physics
// =============================================================================

void LabLayer::RegisterPhysicsBody(Aether::Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic)
{
    if (!m_Scene.IsValid(transformEntity)) return;

    auto* mesh = Aether::AssetManager::GetAsset<Aether::Mesh>(colliderMeshID);
    if (!mesh) return;

    // Walk up hierarchy to compute world transform
    std::vector<Aether::Entity> chain;
    Aether::Entity cur = transformEntity;
    while (cur != Aether::Null_Entity && m_Scene.IsValid(cur))
    {
        chain.push_back(cur);
        cur = m_Scene.GetComponent<Aether::HierarchyComponent>(cur).parent;
    }

    glm::mat4 worldTransform(1.0f);
    for (int i = (int)chain.size() - 1; i >= 0; i--)
        worldTransform *= m_Scene.GetComponent<Aether::TransformComponent>(chain[i]).GetLocalTransform();

    auto& t          = m_Scene.GetComponent<Aether::TransformComponent>(transformEntity);
    t.WorldTransform = worldTransform;
    m_Scene.MarkDirty(transformEntity);

    const glm::mat4& wt = worldTransform;
    glm::vec3 worldScale(
        glm::length(glm::vec3(wt[0])),
        glm::length(glm::vec3(wt[1])),
        glm::length(glm::vec3(wt[2])));

    glm::mat3 rotMat(
        glm::normalize(glm::vec3(wt[0])),
        glm::normalize(glm::vec3(wt[1])),
        glm::normalize(glm::vec3(wt[2])));
    glm::quat worldRot = glm::quat_cast(rotMat);

    glm::vec3 extents     = mesh->GetBoundsExtents() * worldScale;
    glm::vec3 center      = glm::vec3(wt[3]) + rotMat * (mesh->GetBoundsCenter() * worldScale);
    glm::vec3 localOffset = mesh->GetBoundsCenter() * worldScale;

    Aether::BodyConfig config;
    config.motionType  = isDynamic ? Aether::MotionType::Dynamic : Aether::MotionType::Kinematic;
    config.shape       = Aether::ColliderShape::Box;
    config.size        = glm::vec3(
        std::max(std::abs(extents.x), 0.5f),
        std::max(std::abs(extents.y), 0.5f),
        std::max(std::abs(extents.z), 0.5f));
    config.transform   = { center, worldRot };
    config.friction    = 0.5f;
    config.restitution = 0.3f;

    const auto& handle = Aether::PhysicsSystem::CreateBody(config);

    if (!m_Scene.HasComponent<Aether::ColliderComponent>(transformEntity))
        m_Scene.AddComponent<Aether::ColliderComponent>(transformEntity, handle, true);
    else
    {
        auto& col          = m_Scene.GetComponent<Aether::ColliderComponent>(transformEntity);
        col.ColliderHandle = handle;
        col.ColliderOffset = localOffset;
    }

    PhysicsEntry entry;
    entry.handle     = handle;
    entry.enabled    = false;
    entry.lastActive = false;
    entry.isDynamic  = isDynamic;
    m_PhysicsBodies[transformEntity] = entry;

    if (isDynamic)
        Aether::PhysicsSystem::SetActive(handle, false);
}

// =============================================================================
//  Update
// =============================================================================

void LabLayer::Update(Aether::Timestep ts)
{
    DrainParseQueue();

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    for (auto& [entity, entry] : m_PhysicsBodies)
    {
        if (entry.enabled != entry.lastActive)
        {
            Aether::PhysicsSystem::SetActive(entry.handle, entry.enabled);
            entry.lastActive = entry.enabled;
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

    m_Scene.Update(ts, &m_Camera);

    // Inject persistent IK tasks AFTER the scene has run its own animation pass
    // (scene calls ClearTasks+ProcessTasks internally; these go in the next frame's
    // pre-render window if you wire them before scene update, but for a test panel
    // "apply next frame" is fine — just schedule here and they'll fire next Update)
    {
        auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
        if (rigSystem && m_IKAnimatorEntity != Aether::Null_Entity &&
            m_Scene.IsValid(m_IKAnimatorEntity) &&
            m_Scene.HasComponent<Aether::AnimatorComponent>(m_IKAnimatorEntity))
        {
            auto& anim = m_Scene.GetComponent<Aether::AnimatorComponent>(m_IKAnimatorEntity);
            auto* skelAsset = Aether::AssetManager::GetAsset<Aether::Skeleton>(anim.Skeleton);

            if (skelAsset && anim.CurrentPose.IsValid())
            {
                auto skelHnd = skelAsset->GetHandle();

                if (m_TwoBoneIK.enabled &&
                    m_TwoBoneIK.rootIdx >= 0 && m_TwoBoneIK.midIdx >= 0 && m_TwoBoneIK.endIdx >= 0)
                {
                    Aether::TwoBoneIKSpec spec;
                    spec.Skeleton = skelHnd;
                    spec.Pose     = anim.CurrentPose;
                    spec.Root     = m_TwoBoneIK.rootIdx;
                    spec.Mid      = m_TwoBoneIK.midIdx;
                    spec.End      = m_TwoBoneIK.endIdx;
                    spec.Target   = m_TwoBoneIK.target;
                    spec.Pole     = m_TwoBoneIK.pole;
                    spec.Weight   = m_TwoBoneIK.weight;
                    rigSystem->ScheduleTwoBoneIK(spec);
                    rigSystem->ScheduleFinalize(skelHnd, anim.CurrentPose);
                    rigSystem->ProcessTasks();
                    rigSystem->ClearTasks();
                }

                if (m_LookAt.enabled && m_LookAt.boneIdx >= 0)
                {
                    Aether::LookAtSpec spec;
                    spec.Skeleton   = skelHnd;
                    spec.Pose       = anim.CurrentPose;
                    spec.Bone       = m_LookAt.boneIdx;
                    spec.Target     = m_LookAt.target;
                    spec.Forward    = m_LookAt.forward;
                    spec.Up         = m_LookAt.up;
                    spec.Weight     = m_LookAt.weight;
                    spec.AngleLimit = m_LookAt.angleLimit;
                    rigSystem->ScheduleLookAt(spec);
                    rigSystem->ScheduleFinalize(skelHnd, anim.CurrentPose);
                    rigSystem->ProcessTasks();
                    rigSystem->ClearTasks();
                }
            }
        }
    }
}

void LabLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled)
        m_Camera.OnEvent(event);
}

// =============================================================================
//  ImGui
// =============================================================================

void LabLayer::OnImGuiRender()
{
    Aether::UI::PerformanceOverlay(0);

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawAnimationPanel();
    DrawLightingPanel();
    DrawScriptingPanel();
    DrawBoneAttachmentPanel();   // <-- NEW
}

void LabLayer::DrawScriptingPanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Scripting"))
    {
        // ---- Entity picker ---------------------------------------------------
        std::string entityPreview = (m_ScriptTargetEntity != Null_Entity &&
                                     m_Scene.IsValid(m_ScriptTargetEntity))
            ? m_Scene.GetComponent<TagComponent>(m_ScriptTargetEntity).Tag
            : "Select Entity";

        if (auto c = UI::Combo("Entity##script", entityPreview.c_str()))
        {
            for (auto entity : m_Scene.View<TagComponent>())
            {
                auto  g   = UI::ID((int)(uint64_t)entity);
                bool  sel = (m_ScriptTargetEntity == entity);
                auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                if (UI::Selectable(tag.Tag.c_str(), sel))
                    m_ScriptTargetEntity = entity;
                if (sel) ImGui::SetItemDefaultFocus();
            }
        }

        // ---- File path input -------------------------------------------------
        char buf[512];
        std::strncpy(buf, m_ScriptPath.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText("Script Path", buf, sizeof(buf)))
            m_ScriptPath = buf;

        // ---- Current script on selected entity (read-only info) --------------
        bool hasScript = (m_ScriptTargetEntity != Null_Entity &&
                          m_Scene.IsValid(m_ScriptTargetEntity) &&
                          m_Scene.HasComponent<ScriptComponent>(m_ScriptTargetEntity));

        if (hasScript)
            UI::Text("Status: Script attached");
        else
            UI::TextDisabled("Status: No script");

        // ---- Attach button ---------------------------------------------------
        bool canAttach = (m_ScriptTargetEntity != Null_Entity &&
                          m_Scene.IsValid(m_ScriptTargetEntity) &&
                          !m_ScriptPath.empty());
        {
            auto d = UI::Disabled(!canAttach);
            if (UI::Button("Attach Script"))
            {
                // Destroy old instance if one exists
                if (m_Scene.HasComponent<ScriptComponent>(m_ScriptTargetEntity))
                {
                    auto& sc = m_Scene.GetComponent<ScriptComponent>(m_ScriptTargetEntity);
                    if (sc.ScriptHandle.IsValid())
                        ScriptEngine::DestroyInstance(sc.ScriptHandle);
                    m_Scene.RemoveComponent<ScriptComponent>(m_ScriptTargetEntity);
                }

                Handle<ScriptTag> handle = ScriptEngine::CreateInstance(
                    &m_Scene, m_ScriptTargetEntity);

                if (handle.IsValid())
                {
                    m_Scene.AddComponent<ScriptComponent>(m_ScriptTargetEntity, handle);
                    ScriptEngine::LoadScript(handle, m_ScriptPath);
                    ScriptEngine::StartInstance(handle);
                    AE_CORE_INFO("[Scripting] Attached '{}' to entity '{}'",
                        m_ScriptPath,
                        m_Scene.GetComponent<TagComponent>(m_ScriptTargetEntity).Tag);
                }
                else
                {
                    AE_CORE_ERROR("[Scripting] Failed to create instance from '{}'", m_ScriptPath);
                }
            }
        }

        // ---- Detach button ---------------------------------------------------
        ImGui::SameLine();
        {
            auto d = UI::Disabled(!hasScript);
            if (UI::Button("Detach Script"))
            {
                auto& sc = m_Scene.GetComponent<ScriptComponent>(m_ScriptTargetEntity);
                if (sc.ScriptHandle.IsValid())
                    ScriptEngine::DestroyInstance(sc.ScriptHandle);
                m_Scene.RemoveComponent<ScriptComponent>(m_ScriptTargetEntity);
                AE_CORE_INFO("[Scripting] Detached script from entity '{}'",
                    m_Scene.GetComponent<TagComponent>(m_ScriptTargetEntity).Tag);
            }
        }

        // ---- Active scripts list --------------------------------------------
        UI::Separator();
        UI::SectionHeader("Active Scripts");

        for (auto entity : m_Scene.View<TagComponent, ScriptComponent>())
        {
            auto  g      = UI::ID((int)(uint64_t)entity);
            auto& tag    = m_Scene.GetComponent<TagComponent>(entity);
            auto& sc     = m_Scene.GetComponent<ScriptComponent>(entity);
            UI::Text("%s  (slot %d)", tag.Tag.c_str(), sc.ScriptHandle.index);
        }
    }
}

// =============================================================================
//  Hierarchy panel
// =============================================================================

void LabLayer::DrawHierarchyPanel()
{
    // UI::SceneHierarchy opens the window, iterates roots, and handles
    // deselect-on-empty-click and per-node context menus internally.
    Aether::UI::SceneHierarchy("Hierarchy", m_Scene, m_SelectedEntity);
}

// DrawEntityNode removed — UI::SceneHierarchy handles hierarchy traversal.

// =============================================================================
//  Scene panel
// =============================================================================

void LabLayer::DrawScenePanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Scene"))
    {
        // BUG FIX: derive animator count live from the ECS view instead of
        // m_AnimatorIDs which was never populated and always showed 0.
        int animatorCount = 0;
        for (auto e : m_Scene.View<AnimatorComponent>()) { (void)e; animatorCount++; }

        UI::Text("Meshes:    %d", (int)m_MeshIDs.size());
        UI::Text("Animators: %d", animatorCount);
        UI::Separator();

        // ---- Physics --------------------------------------------------------
        if (auto h = UI::Header("Physics"))
        {
            // Node picker — entities aren't strings, so manual combo loop
            std::string nodePreview = (m_PhysSelectedEntity != Null_Entity &&
                                       m_Scene.IsValid(m_PhysSelectedEntity))
                ? m_Scene.GetComponent<TagComponent>(m_PhysSelectedEntity).Tag
                : "Select Node";

            if (auto c = UI::Combo("Node##phys", nodePreview.c_str()))
            {
                for (auto entity : m_Scene.View<TagComponent>())
                {
                    auto  g   = UI::ID((int)(uint64_t)entity);
                    bool  sel = (m_PhysSelectedEntity == entity);
                    auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                    if (UI::Selectable(tag.Tag.c_str(), sel)) m_PhysSelectedEntity = entity;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
            }

            // Mesh picker
            std::vector<std::string> meshNames;
            meshNames.reserve(m_MeshIDs.size());
            for (auto& id : m_MeshIDs)
                meshNames.push_back(AssetsRegister::Get(id));
            UI::ComboList("Mesh##phys", meshNames, m_PhysMeshIdx);

            UI::Checkbox("Is Dynamic", m_PhysDynamic);

            bool canAdd = (m_PhysSelectedEntity != Null_Entity &&
                           m_Scene.IsValid(m_PhysSelectedEntity) &&
                           m_PhysMeshIdx >= 0 && m_PhysMeshIdx < (int)m_MeshIDs.size());
            {
                auto d = UI::Disabled(!canAdd);
                if (UI::Button("Add Physics Body"))
                    RegisterPhysicsBody(m_PhysSelectedEntity, m_MeshIDs[m_PhysMeshIdx], m_PhysDynamic);
            }

            UI::SectionHeader("Active Bodies");
            for (auto& [entity, entry] : m_PhysicsBodies)
            {
                if (!m_Scene.IsValid(entity)) continue;
                auto  g   = UI::ID((int)(uint64_t)entity);
                auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                if (UI::Checkbox(tag.Tag.c_str(), entry.enabled))
                    PhysicsSystem::SetActive(entry.handle, entry.enabled);
            }

            UI::Separator();

            // Force / velocity applicator for the selected entity
            auto physIt = (m_SelectedEntity != Null_Entity)
                ? m_PhysicsBodies.find(m_SelectedEntity)
                : m_PhysicsBodies.end();

            UI::Text("Apply to Selected Entity:");

            if (physIt == m_PhysicsBodies.end())
            {
                UI::TextDisabled("(select an entity with a physics body)");
            }
            else if (!physIt->second.isDynamic)
            {
                UI::TextDisabled("(static body — forces not applicable)");
            }
            else
            {
                UI::DragXYZ("Force",    m_ForceInput,    0.5f);
                UI::SameLine();
                if (UI::Button("Apply Force"))
                    PhysicsSystem::AddForce(physIt->second.handle, m_ForceInput);
                UI::SameLine();
                if (UI::SmallButton("X##force")) m_ForceInput = glm::vec3(0.0f);

                UI::DragXYZ("Velocity", m_VelocityInput, 0.5f);
                UI::SameLine();
                if (UI::Button("Set Velocity"))
                    PhysicsSystem::SetVelocity(physIt->second.handle, m_VelocityInput);
                UI::SameLine();
                if (UI::SmallButton("X##vel")) m_VelocityInput = glm::vec3(0.0f);
            }
        }

        // ---- Camera ---------------------------------------------------------
        if (auto h = UI::Header("Camera"))
        {
            glm::vec3 pos = m_Camera.GetPosition();
            UI::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
            UI::Text("Distance: %.1f", m_Camera.GetDistance());
            if (UI::Button("Reset Camera"))
                m_Camera.SetDistance(5.0f);
        }

        // ---- Transform ------------------------------------------------------
        if (auto h = UI::Header("Transform"))
        {
            UI::TransformInspector(m_Scene, m_SelectedEntity);
        }

        if (auto h = UI::Header("Export"))
        {
            UI::InputText("Path##export", m_SceneSavePath, sizeof(m_SceneSavePath));
            if (UI::Button("Export Scene"))
            {
                if (SceneSerializer::Serialize(m_Scene, m_SceneSavePath))
                    AE_CORE_INFO("Scene exported to '{0}'", m_SceneSavePath);
                else
                    AE_CORE_ERROR("Failed to export scene to '{0}'", m_SceneSavePath);
            }
        }

        if (auto h = UI::Header("Load"))
        {
            UI::InputText("Path##load", m_SceneLoadPath, sizeof(m_SceneLoadPath));
            if (UI::Button("Load Scene"))
            {
                if (SceneSerializer::DeserializeInto(m_SceneLoadPath, m_Scene))
                    AE_CORE_INFO("Scene loaded from '{0}'", m_SceneLoadPath);
                else
                    AE_CORE_ERROR("Failed to load scene from '{0}'", m_SceneLoadPath);
            }
        }
    }
}

// =============================================================================
//  Animation panel
// =============================================================================

// Helper: (re)populate m_CachedJointNames when the selected animator changes
static void RefreshJointCache(
    Aether::Entity entity,
    Aether::Entity& cachedEntity,
    std::vector<std::string>& cache,
    Aether::Scene& scene,
    Aether::RigModule* rig)
{
    if (entity == cachedEntity && !cache.empty()) return;
    cache.clear();
    cachedEntity = entity;
    if (entity == Aether::Null_Entity || !scene.IsValid(entity)) return;
    if (!scene.HasComponent<Aether::AnimatorComponent>(entity)) return;

    auto& anim = scene.GetComponent<Aether::AnimatorComponent>(entity);
    auto* skelAsset = Aether::AssetManager::GetAsset<Aether::Skeleton>(anim.Skeleton);
    if (!skelAsset) return;

    auto skelHnd = skelAsset->GetHandle();
    int count = rig->GetJointCount(skelHnd);
    cache.reserve(count);
    for (int i = 0; i < count; i++)
        cache.push_back(std::to_string(i) + "  " + rig->GetJointName(skelHnd, i));
}

// Small helper: draw a combo that lets the user pick a joint index.
// Returns true when the selection changed.
static bool JointCombo(const char* label, int& selectedIdx,
                       const std::vector<std::string>& names)
{
    if (names.empty()) { ImGui::TextDisabled("(no skeleton)"); return false; }
    std::string preview = (selectedIdx >= 0 && selectedIdx < (int)names.size())
        ? names[selectedIdx] : "-- pick joint --";
    bool changed = false;
    if (ImGui::BeginCombo(label, preview.c_str()))
    {
        for (int i = 0; i < (int)names.size(); i++)
        {
            bool sel = (selectedIdx == i);
            if (ImGui::Selectable(names[i].c_str(), sel)) { selectedIdx = i; changed = true; }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

void LabLayer::DrawAnimationPanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Animation"))
    {
        auto rigSystem = AnimationSystem::GetModule<RigModule>();
        if (!rigSystem) { UI::TextDisabled("RigSystem not initialized."); return; }

        // ---- Bind mesh to animator ------------------------------------------
        if (auto h = UI::Header("Bind Mesh to Animator"))
        {
            std::vector<std::string> meshNames;
            meshNames.reserve(m_MeshIDs.size());
            for (int i = 0; i < (int)m_MeshIDs.size(); i++)
                meshNames.push_back(std::to_string(i) + ": " + AssetsRegister::Get(m_MeshIDs[i]));
            UI::ComboList("Mesh##bind", meshNames, m_BindMeshIndex);

            auto animView = m_Scene.View<AnimatorComponent>();
            std::vector<Entity> animEntities(animView.begin(), animView.end());

            std::vector<std::string> animNames;
            animNames.reserve(animEntities.size());
            for (int i = 0; i < (int)animEntities.size(); i++)
                animNames.push_back(m_Scene.GetComponent<TagComponent>(animEntities[i]).Tag);
            UI::ComboList("Animator##bind", animNames, m_BindAnimatorIndex);

            bool canBind = (m_BindMeshIndex    >= 0 && m_BindMeshIndex    < (int)m_MeshIDs.size() &&
                            m_BindAnimatorIndex >= 0 && m_BindAnimatorIndex < (int)animEntities.size());
            {
                auto d = UI::Disabled(!canBind);
                if (UI::Button("Bind"))
                {
                    UUID meshID = m_MeshIDs[m_BindMeshIndex];
                    Entity targetAnimEnt = animEntities[m_BindAnimatorIndex];

                    for (auto entity : m_Scene.View<MeshComponent>())
                    {
                        auto& mc   = m_Scene.GetComponent<MeshComponent>(entity);
                        auto* mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
                        if (mesh && mesh->id == meshID)
                        {
                            m_Scene.CloneComponent<AnimatorComponent>(entity, targetAnimEnt);
                            break;
                        }
                    }
                }
            }

            UI::SectionHeader("Active Bindings");
            for (auto entity : m_Scene.View<MeshComponent, AnimatorComponent>())
            {
                auto& mc   = m_Scene.GetComponent<MeshComponent>(entity);
                auto* mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
                auto& anim = m_Scene.GetComponent<AnimatorComponent>(entity);

                std::string meshName = mesh ? AssetsRegister::Get(mesh->id) : "(invalid)";
                std::string skelName = anim.Skeleton.IsValid()
                    ? AssetsRegister::Get(AssetManager::GetAsset<Skeleton>(anim.Skeleton)->id)
                    : "(no skeleton)";

                auto g = UI::ID(mesh ? (int)(uint64_t)mesh->id : (int)(uint64_t)entity);
                UI::Text("%s  ->  [%s]", meshName.c_str(), skelName.c_str());
                UI::SameLine();
                if (UI::SmallButton("Unbind"))
                    m_Scene.RemoveComponent<AnimatorComponent>(entity);
            }
        }

        UI::Separator();

        // ---- Per-animator playback controls ---------------------------------
        bool anyAnimators = false;
        for (auto entity : m_Scene.View<AnimatorComponent>())
        {
            anyAnimators = true;
            auto& tag  = m_Scene.GetComponent<TagComponent>(entity);
            auto& anim = m_Scene.GetComponent<AnimatorComponent>(entity);

            auto g = UI::ID((uint64_t)entity);
            if (auto h = UI::Header(tag.Tag.c_str()))
                UI::AnimatorControls(anim, rigSystem.get());
            UI::Spacing();
        }
        if (!anyAnimators)
            UI::TextDisabled("No animators in scene.");

        UI::Separator();

        // =====================================================================
        //  IK / Advanced section
        //  All IK operations target a single selected animator entity.
        // =====================================================================
        if (auto h = UI::Header("IK & Advanced"))
        {
            // ---- Animator entity picker -------------------------------------
            {
                std::string preview = (m_IKAnimatorEntity != Null_Entity &&
                                       m_Scene.IsValid(m_IKAnimatorEntity))
                    ? m_Scene.GetComponent<TagComponent>(m_IKAnimatorEntity).Tag
                    : "Select Animator";

                if (ImGui::BeginCombo("Animator##ik", preview.c_str()))
                {
                    for (auto entity : m_Scene.View<AnimatorComponent, TagComponent>())
                    {
                        auto  g   = UI::ID((int)(uint64_t)entity);
                        bool  sel = (m_IKAnimatorEntity == entity);
                        auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                        if (ImGui::Selectable(tag.Tag.c_str(), sel))
                        {
                            m_IKAnimatorEntity = entity;
                            // Invalidate joint cache so it rebuilds next frame
                            m_JointBrowserEntity = Null_Entity;
                            m_CachedJointNames.clear();
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            // Rebuild joint name cache if entity changed
            RefreshJointCache(m_IKAnimatorEntity, m_JointBrowserEntity,
                              m_CachedJointNames, m_Scene, rigSystem.get());

            bool hasAnimator = (m_IKAnimatorEntity != Null_Entity &&
                                m_Scene.IsValid(m_IKAnimatorEntity) &&
                                m_Scene.HasComponent<AnimatorComponent>(m_IKAnimatorEntity));

            // ---- Joint browser (read-only list) -----------------------------
            UI::Separator();
            UI::SectionHeader("Joint Browser");
            if (!hasAnimator)
            {
                UI::TextDisabled("Select an animator to browse joints.");
            }
            else if (m_CachedJointNames.empty())
            {
                UI::TextDisabled("Skeleton has no joints or is not loaded.");
            }
            else
            {
                // Scrollable child so it doesn't blow out the panel height
                ImGui::BeginChild("##jointlist", ImVec2(0, 160), true);
                for (int i = 0; i < (int)m_CachedJointNames.size(); i++)
                    ImGui::TextUnformatted(m_CachedJointNames[i].c_str());
                ImGui::EndChild();
            }

            // ---- Two-Bone IK ------------------------------------------------
            UI::Separator();
            UI::SectionHeader("Two-Bone IK");
            {
                auto d = UI::Disabled(!hasAnimator);

                UI::Checkbox("Enable##tbik", m_TwoBoneIK.enabled);
                JointCombo("Root Joint##tbik",  m_TwoBoneIK.rootIdx, m_CachedJointNames);
                JointCombo("Mid Joint##tbik",   m_TwoBoneIK.midIdx,  m_CachedJointNames);
                JointCombo("End Joint##tbik",   m_TwoBoneIK.endIdx,  m_CachedJointNames);
                UI::DragXYZ("Target##tbik",  m_TwoBoneIK.target, 0.01f);
                UI::DragXYZ("Pole##tbik",    m_TwoBoneIK.pole,   0.01f);
                UI::SliderFloat("Weight##tbik", m_TwoBoneIK.weight, 0.f, 1.f);

                bool canApply = hasAnimator &&
                                m_TwoBoneIK.rootIdx >= 0 &&
                                m_TwoBoneIK.midIdx  >= 0 &&
                                m_TwoBoneIK.endIdx  >= 0 &&
                                m_TwoBoneIK.rootIdx != m_TwoBoneIK.midIdx &&
                                m_TwoBoneIK.midIdx  != m_TwoBoneIK.endIdx;
                {
                    auto d2 = UI::Disabled(!canApply);
                    if (UI::Button("Apply Once##tbik") && canApply)
                    {
                        auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                        auto* skelAsset = AssetManager::GetAsset<Skeleton>(anim.Skeleton);
                        if (skelAsset && anim.CurrentPose.IsValid())
                        {
                            TwoBoneIKSpec spec;
                            spec.Skeleton = skelAsset->GetHandle();
                            spec.Pose     = anim.CurrentPose;
                            spec.Root     = m_TwoBoneIK.rootIdx;
                            spec.Mid      = m_TwoBoneIK.midIdx;
                            spec.End      = m_TwoBoneIK.endIdx;
                            spec.Target   = m_TwoBoneIK.target;
                            spec.Pole     = m_TwoBoneIK.pole;
                            spec.Weight   = m_TwoBoneIK.weight;
                            rigSystem->ScheduleTwoBoneIK(spec);
                        }
                    }
                }
                if (m_TwoBoneIK.enabled)
                    UI::TextDisabled("IK will be injected each frame via Update.");
            }

            // ---- Look-At IK -------------------------------------------------
            UI::Separator();
            UI::SectionHeader("Look-At IK");
            {
                auto d = UI::Disabled(!hasAnimator);

                UI::Checkbox("Enable##lookat", m_LookAt.enabled);
                JointCombo("Bone##lookat", m_LookAt.boneIdx, m_CachedJointNames);
                UI::DragXYZ("Target##lookat",  m_LookAt.target,  0.01f);
                UI::DragXYZ("Forward##lookat", m_LookAt.forward, 0.01f);
                UI::DragXYZ("Up##lookat",      m_LookAt.up,      0.01f);
                UI::SliderFloat("Weight##lookat",     m_LookAt.weight,     0.f, 1.f);
                UI::SliderFloat("Angle Limit (rad)##lookat", m_LookAt.angleLimit, 0.f, 3.14159f);

                bool canApply = hasAnimator && m_LookAt.boneIdx >= 0;
                {
                    auto d2 = UI::Disabled(!canApply);
                    if (UI::Button("Apply Once##lookat") && canApply)
                    {
                        auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                        auto* skelAsset = AssetManager::GetAsset<Skeleton>(anim.Skeleton);
                        if (skelAsset && anim.CurrentPose.IsValid())
                        {
                            LookAtSpec spec;
                            spec.Skeleton   = skelAsset->GetHandle();
                            spec.Pose       = anim.CurrentPose;
                            spec.Bone       = m_LookAt.boneIdx;
                            spec.Target     = m_LookAt.target;
                            spec.Forward    = m_LookAt.forward;
                            spec.Up         = m_LookAt.up;
                            spec.Weight     = m_LookAt.weight;
                            spec.AngleLimit = m_LookAt.angleLimit;
                            rigSystem->ScheduleLookAt(spec);
                        }
                    }
                }
            }

            // ---- Blend ------------------------------------------------------
            UI::Separator();
            UI::SectionHeader("Clip Blend");
            {
                auto d = UI::Disabled(!hasAnimator);
                UI::Checkbox("Enable##blend", m_Blend.enabled);
                UI::Checkbox("Additive##blend", m_Blend.additive);

                // Clip pickers — list clips on the selected animator
                if (hasAnimator)
                {
                    auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                    std::vector<std::string> clipNames;
                    for (int i = 0; i < (int)anim.Clips.size(); i++)
                        clipNames.push_back("Clip " + std::to_string(i));

                    UI::ComboList("Clip A##blend", clipNames, m_Blend.clipAIdx);
                    UI::ComboList("Clip B##blend", clipNames, m_Blend.clipBIdx);
                }

                UI::SliderFloat("Alpha##blend", m_Blend.alpha, 0.f, 1.f);

                bool canBlend = hasAnimator && [&]() -> bool {
                    auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                    return m_Blend.clipAIdx >= 0 && m_Blend.clipAIdx < (int)anim.Clips.size() &&
                           m_Blend.clipBIdx >= 0 && m_Blend.clipBIdx < (int)anim.Clips.size() &&
                           m_Blend.clipAIdx != m_Blend.clipBIdx &&
                           anim.CurrentPose.IsValid();
                }();

                {
                    auto d2 = UI::Disabled(!canBlend);
                    if (UI::Button("Apply Once##blend") && canBlend)
                    {
                        auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                        auto* skelAsset = AssetManager::GetAsset<Skeleton>(anim.Skeleton);
                        auto* clipAAsset = AssetManager::GetAsset<Clip>(anim.Clips[m_Blend.clipAIdx]);
                        auto* clipBAsset = AssetManager::GetAsset<Clip>(anim.Clips[m_Blend.clipBIdx]);
                        if (skelAsset && clipAAsset && clipBAsset)
                        {
                            // We need two scratch poses for A and B, then blend into CurrentPose.
                            // For a quick test we reuse CurrentPose as poseOut and sample A into
                            // a temporary pose, B into another, then blend.
                            auto skelHnd  = skelAsset->GetHandle();
                            auto poseA    = rigSystem->CreatePose(skelHnd);
                            auto poseB    = rigSystem->CreatePose(skelHnd);

                            rigSystem->ScheduleSample(skelHnd, clipAAsset->GetHandle(),
                                anim.Cache, poseA, anim.CurrentTime);
                            rigSystem->ScheduleSample(skelHnd, clipBAsset->GetHandle(),
                                anim.Cache, poseB, anim.CurrentTime);

                            if (m_Blend.additive)
                                rigSystem->ScheduleAdditive(poseA, poseB, anim.CurrentPose, m_Blend.alpha);
                            else
                                rigSystem->ScheduleBlend(poseA, poseB, anim.CurrentPose, m_Blend.alpha);

                            rigSystem->ScheduleFinalize(skelHnd, anim.CurrentPose);
                            rigSystem->ProcessTasks();
                            rigSystem->ClearTasks();

                            rigSystem->DestroyPose(poseA);
                            rigSystem->DestroyPose(poseB);
                        }
                    }
                }
            }
        } // end IK & Advanced header
    }
}

// =============================================================================
//  Lighting panel
// =============================================================================

void LabLayer::DrawLightingPanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Lighting"))
    {
        if (auto h = UI::Header("Spotlight"))
        {
            auto& lightComp  = m_Scene.GetComponent<LightComponent>(m_LightEntity);
            auto& lightTrans = m_Scene.GetComponent<TransformComponent>(m_LightEntity);
            UI::LightInspector(lightComp.Config, &lightTrans.WorldTransform);
        }

        if (auto h = UI::Header("Volumetric Lighting"))
        {
            UI::SliderFloat("Density",   m_VolDensity,   0.0f, 0.2f);
            UI::SliderFloat("Intensity", m_VolIntensity, 0.0f, 5.0f);
            UI::SliderInt  ("Steps",     m_VolSteps,     8,    128);
        }

        if (auto h = UI::Header("Shadow"))
        {
            UI::SliderFloat("Bias", m_ShadowBias, 0.00001f, 0.005f);
        }
    }
}

// =============================================================================
//  Bone Attachment panel
// =============================================================================

void LabLayer::DrawBoneAttachmentPanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Bone Attachment"))
    {
        // ---- Setup section --------------------------------------------------
        if (auto h = UI::Header("Attach"))
        {
            // --- Child entity picker (the object that will follow the bone) ---
            std::string childPreview = (m_BoneAttachChildEntity != Null_Entity &&
                                        m_Scene.IsValid(m_BoneAttachChildEntity))
                ? m_Scene.GetComponent<TagComponent>(m_BoneAttachChildEntity).Tag
                : "Select Child Entity";

            if (auto c = UI::Combo("Child Entity##boneattach", childPreview.c_str()))
            {
                for (auto entity : m_Scene.View<TagComponent>())
                {
                    auto  g   = UI::ID((int)(uint64_t)entity);
                    bool  sel = (m_BoneAttachChildEntity == entity);
                    auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                    if (UI::Selectable(tag.Tag.c_str(), sel))
                        m_BoneAttachChildEntity = entity;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
            }

            // --- Animator entity picker (the entity that owns the skeleton) --
            std::string animPreview = (m_BoneAttachAnimatorEntity != Null_Entity &&
                                       m_Scene.IsValid(m_BoneAttachAnimatorEntity))
                ? m_Scene.GetComponent<TagComponent>(m_BoneAttachAnimatorEntity).Tag
                : "Select Animator Entity";

            if (auto c = UI::Combo("Animator Entity##boneattach", animPreview.c_str()))
            {
                // Only show entities that actually have an AnimatorComponent
                for (auto entity : m_Scene.View<AnimatorComponent, TagComponent>())
                {
                    auto  g   = UI::ID((int)(uint64_t)entity);
                    bool  sel = (m_BoneAttachAnimatorEntity == entity);
                    auto& tag = m_Scene.GetComponent<TagComponent>(entity);
                    if (UI::Selectable(tag.Tag.c_str(), sel))
                        m_BoneAttachAnimatorEntity = entity;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
            }

            // --- Joint picker (uses shared joint name cache) -----------------
            {
                auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
                if (rigSystem)
                    RefreshJointCache(m_BoneAttachAnimatorEntity, m_JointBrowserEntity,
                                      m_CachedJointNames, m_Scene, rigSystem.get());
            }
            {
                std::string preview = (m_BoneNameBuf[0] != '\0') ? m_BoneNameBuf : "-- pick joint --";
                if (ImGui::BeginCombo("Joint##boneattach", preview.c_str()))
                {
                    for (int i = 0; i < (int)m_CachedJointNames.size(); i++)
                    {
                        // Entry format is "N  jointName" — strip prefix for comparison/storage
                        auto& entry = m_CachedJointNames[i];
                        auto sep = entry.find("  ");
                        std::string name = (sep != std::string::npos) ? entry.substr(sep + 2) : entry;
                        bool sel = (std::string(m_BoneNameBuf) == name);
                        if (ImGui::Selectable(entry.c_str(), sel))
                        {
                            std::strncpy(m_BoneNameBuf, name.c_str(), sizeof(m_BoneNameBuf));
                            m_BoneNameBuf[sizeof(m_BoneNameBuf) - 1] = '\0';
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            // Manual fallback if skeleton isn't loaded yet
            ImGui::InputText("Joint Name (manual)##boneattach", m_BoneNameBuf, sizeof(m_BoneNameBuf));

            UI::Separator();

            // --- Attach button -----------------------------------------------
            bool boneName    = (m_BoneNameBuf[0] != '\0');
            bool childValid  = (m_BoneAttachChildEntity    != Null_Entity &&
                                m_Scene.IsValid(m_BoneAttachChildEntity));
            bool animValid   = (m_BoneAttachAnimatorEntity != Null_Entity &&
                                m_Scene.IsValid(m_BoneAttachAnimatorEntity));
            // Guard: child must not be the same entity as the animator
            bool notSelf     = (m_BoneAttachChildEntity != m_BoneAttachAnimatorEntity);
            bool canAttach   = boneName && childValid && animValid && notSelf;

            {
                auto d = UI::Disabled(!canAttach);
                if (UI::Button("Attach##boneattach"))
                {

                    if (m_Scene.HasComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity))
                    {
                        // Update in place so the scene's cached bone index is invalidated properly
                        auto& existing = m_Scene.GetComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);
                        existing.Invalidate();
                        existing.AnimatorEntity = m_BoneAttachAnimatorEntity;
                        existing.JointName      = m_BoneNameBuf;
                    }
                    else
                    {
                        m_Scene.AddComponent<BoneAttachmentComponent>(
                            m_BoneAttachChildEntity,
                            m_BoneAttachAnimatorEntity,
                            std::string_view(m_BoneNameBuf));
                    }

                    // Mark the child's transform dirty so the scene picks it up next frame
                    m_Scene.GetComponent<TransformComponent>(m_BoneAttachChildEntity).Dirty = true;

                    AE_CORE_INFO("[BoneAttach] '{}' -> bone '{}' on '{}'",
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachChildEntity).Tag,
                        m_BoneNameBuf,
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachAnimatorEntity).Tag);
                }
            }

            // --- Detach button (only shown when the child already has one) ---
            bool hasAttach = childValid &&
                             m_Scene.HasComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);
            ImGui::SameLine();
            {
                auto d = UI::Disabled(!hasAttach);
                if (UI::Button("Detach##boneattach"))
                {
                    m_Scene.RemoveComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);
                    // Restore Dirty so normal transform propagation takes back over
                    m_Scene.GetComponent<TransformComponent>(m_BoneAttachChildEntity).Dirty = true;

                    AE_CORE_INFO("[BoneAttach] Detached '{}' from bone.",
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachChildEntity).Tag);
                }
            }

            // Helper: if child already has a BoneAttachmentComponent, populate
            // the panel fields from it so the user can inspect / edit live.
            if (hasAttach)
            {
                auto& existing = m_Scene.GetComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);

                // Sync animator picker to match the component
                if (m_BoneAttachAnimatorEntity != existing.AnimatorEntity)
                    m_BoneAttachAnimatorEntity = existing.AnimatorEntity;

                // Show current bone index as a read-only hint
                if (existing.JointIndex >= 0)
                    UI::Text("Resolved joint index: %d", existing.JointIndex);
                else
                    UI::TextDisabled("Joint not yet resolved (check joint name).");
            }
        }

        UI::Separator();

        // ---- Active attachments list ----------------------------------------
        if (auto h = UI::Header("Active Attachments"))
        {
            bool any = false;
            for (auto entity : m_Scene.View<BoneAttachmentComponent, TagComponent>())
            {
                any = true;
                auto  g      = UI::ID((int)(uint64_t)entity);
                auto& tag    = m_Scene.GetComponent<TagComponent>(entity);
                auto& attach = m_Scene.GetComponent<BoneAttachmentComponent>(entity);

                // Resolve animator tag safely
                std::string animTag = "(invalid)";
                if (attach.AnimatorEntity != Null_Entity &&
                    m_Scene.IsValid(attach.AnimatorEntity))
                    animTag = m_Scene.GetComponent<TagComponent>(attach.AnimatorEntity).Tag;

                UI::Text("'%s'  ->  bone '%s'  on  '%s'",
                    tag.Tag.c_str(),
                    attach.JointName.c_str(),
                    animTag.c_str());

                // Quick-select this child entity for editing
                ImGui::SameLine();
                if (UI::SmallButton("Select##bonerow"))
                {
                    m_BoneAttachChildEntity = entity;

                    // Populate edit fields from the component
                    std::strncpy(m_BoneNameBuf, attach.JointName.c_str(), sizeof(m_BoneNameBuf));
                    m_BoneNameBuf[sizeof(m_BoneNameBuf) - 1] = '\0';
                    m_BoneAttachAnimatorEntity = attach.AnimatorEntity;
                }
            }

            if (!any)
                UI::TextDisabled("No bone attachments in scene.");
        }
    }
}