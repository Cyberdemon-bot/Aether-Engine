#include "LabLayer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

const char* imgui_layout = R"(
[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Console]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,6

[Window][Hierarchy]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,4

[Window][Scene]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,5

[Window][Animation]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,2

[Window][Lighting]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,3

[Window][Scripting]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,0

[Window][Bone Attachment]
Pos=760,9
Size=610,355
Collapsed=0
DockId=0x00000001,1

[Docking][Data]
DockNode  ID=0x00000001 Pos=760,9 Size=610,355 Selected=0x36FF2379
)";

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
    Aether::Application::Get().SetTickRate(120);
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    Aether::ImGuiLayer::LoadLayout(imgui_layout);

    m_Importer = Aether::ServiceManager::GetService<Aether::Importer>();
    auto fs = Aether::ServiceManager::GetService<Aether::FileSystem>();
    fs->Mount("", Aether::CreateRef<Aether::LooseFileProvider>("."));

    auto& window = Aether::Application::Get().GetWindow();
    m_AssetManager = Aether::ServiceManager::GetService<Aether::AssetManager>();

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
    mainPass.TargetFBO      = m_MainFbo.get();
    mainPass.Shader         = m_MainShader.get();
    mainPass.ClearColor     = true;
    mainPass.ClearDepth     = true;
    mainPass.UsingSkybox    = true;
    mainPass.ClearValue     = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    mainPass.CullFace       = Aether::State::BACK_CULL;
    mainPass.OnScreen       = false;
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
    volPass.TargetFBO      = m_VolFbo.get();
    volPass.Shader         = m_VolShader.get();
    volPass.ClearColor     = true;
    volPass.ClearDepth     = true;
    volPass.CullFace       = Aether::State::None;
    volPass.OnScreen       = true;
    volPass.UsingGeometry  = false;
    volPass.UsingShadowmap = true;
    volPass.readList       = {
        { "u_SceneColor", m_MainFbo->GetColorAttachment() },
        { "u_SceneDepth", m_MainFbo->GetDepthAttachment() }
    };

    m_Pipeline = { mainPass, volPass };
    auto* renderer = Aether::ServiceManager::GetService<Aether::Renderer>();
    renderer->SetPipeline(m_Pipeline.data(), m_Pipeline.size());

    // --- SKYBOX ---
    renderer->SetSkyBox("assets/textures/skybox.png");

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

    m_Scene.Init();

    m_LightEntity = m_Scene.CreateEntity("Spotlight");
    m_Scene.AddComponent<Aether::LightComponent>(m_LightEntity).Config = spotLight;
    auto& lightTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_LightEntity);
    lightTransform.Translation = spotLight.position;
    lightTransform.Dirty       = true;

    // --- CONSOLE COMMANDS ---
    Aether::ConsoleLayer::RegisterCommand("load",      AE_BIND_CONSOLE_FN(LoadModelAsync));
    Aether::ConsoleLayer::RegisterCommand("add",       AE_BIND_CONSOLE_FN(AddEntity));

    auto* script_engine = Aether::ServiceManager::GetService<Aether::ScriptEngine>();

    script_engine->ImportNativeFunc("PrintTest", [](const Aether::ScriptTable& args) -> Aether::ScriptTable
    {
        AE_WARN("printTested successfully with value {0}", args.GetElement<int>(0));
        return Aether::ScriptTable::Make(args.GetElement<int>(0) * 2);
    });

    AE_CORE_INFO("LabLayer initialized!");
}

void LabLayer::Detach()
{
    m_MainShader.reset();
    m_VolShader.reset();
    m_MainFbo.reset();
    m_VolFbo.reset();
    m_MeshIDs.clear();
    m_Scene.Shutdown();
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
        Aether::ServiceManager::GetService<Aether::JobSystem>()->SubmitJob([this, path]()
        {
            AE_CORE_INFO("Worker: Parsing {0}", path);
            auto parsed = m_Importer->ImportScene(path);
            {
                std::lock_guard<std::mutex> lock(m_ParseMutex);
                m_CompletedParses.push(parsed);
            }
            AE_CORE_INFO("Worker: Parsing complete for {0}", path);
        });
    }
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
        auto result = m_Importer->UploadScene(parsed);
        for (auto& meshID : result.meshIDs) m_MeshIDs.push_back(meshID);

        m_Scene.LoadHierarchy(&result);

        AE_CORE_INFO("Loaded: {0} mesh(es)", result.meshIDs.size());
    }
}

// =============================================================================
//  Physics
// =============================================================================

void LabLayer::RegisterPhysicsBody(Aether::Entity transformEntity,
                                   Aether::UUID    colliderMeshID,
                                   bool            isDynamic)
{
    if (!m_Scene.IsValid(transformEntity)) return;

    auto* mesh = m_AssetManager->GetAsset<Aether::AMesh>(colliderMeshID);
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

    glm::vec3 extents     = mesh->m_BoundsExtents * worldScale;
    glm::vec3 center      = glm::vec3(wt[3]) + rotMat * (mesh->m_BoundsCenter * worldScale);
    glm::vec3 localOffset = mesh->m_BoundsCenter * worldScale;

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

    if (!m_Scene.HasComponent<Aether::ColliderComponent>(transformEntity)) m_Scene.AddComponent<Aether::ColliderComponent>(transformEntity);
    auto& col = m_Scene.GetComponent<Aether::ColliderComponent>(transformEntity);
    col.ColliderOffset = localOffset;
    col.Type = config.motionType;
    col.Shape = config.shape;
    col.Size = config.size;
    col.Friction = 0.5;
    col.Restitution = 0.3f;
    col.ColliderHandle = Aether::Handle<Aether::RigidBody>::MakeInvalid();
    col.Visible = true;
}

// =============================================================================
//  IK callback — rebuilds the onPostEvaluate on the selected animator entity
// =============================================================================

void LabLayer::RebuildPostEvaluate()
{
    // Nothing to wire up if no entity is selected
    if (m_IKAnimatorEntity == Aether::Null_Entity ||
        !m_Scene.IsValid(m_IKAnimatorEntity) ||
        !m_Scene.HasComponent<Aether::AnimatorComponent>(m_IKAnimatorEntity))
        return;

    auto& anim = m_Scene.GetComponent<Aether::AnimatorComponent>(m_IKAnimatorEntity);

    // If nothing is enabled, clear the callback so the scene pays zero cost
    const bool anyEnabled = m_TwoBoneIK.enabled || m_LookAt.enabled || m_Blend.enabled;
    if (!anyEnabled)
    {
        anim.onPostEvaluate = nullptr;
        return;
    }

    // Capture all IK state by value so the lambda is self-contained and
    // survives UI changes until the next RebuildPostEvaluate() call.
    TwoBoneIKState ikState   = m_TwoBoneIK;
    LookAtState    laState   = m_LookAt;
    BlendState     blState   = m_Blend;

    anim.onPostEvaluate = [ikState, laState, blState, this]
        (Aether::Entity entity, Aether::RigModule* rig, float /*dt*/)
    {
        // Retrieve the component fresh inside the callback (the scene passes us
        // the entity so we can look it up safely)
        auto& scene = m_Scene;
        if (!scene.IsValid(entity)) return;
        if (!scene.HasComponent<Aether::AnimatorComponent>(entity)) return;

        auto& comp      = scene.GetComponent<Aether::AnimatorComponent>(entity);
        auto* skelAsset = m_AssetManager->GetAsset<Aether::ASkeleton>(comp.Skeleton);
        if (!skelAsset || !comp.CurrentPose.IsValid()) return;

        auto skelHnd = skelAsset->m_Handle;

        // ---- Two-Bone IK ------------------------------------------------
        if (ikState.enabled &&
            ikState.rootIdx >= 0 && ikState.midIdx >= 0 && ikState.endIdx >= 0)
        {
            Aether::TwoBoneIKSpec spec;
            spec.Skeleton = skelHnd;
            spec.Pose     = comp.CurrentPose;
            spec.Root     = ikState.rootIdx;
            spec.Mid      = ikState.midIdx;
            spec.End      = ikState.endIdx;
            spec.Target   = ikState.target;
            spec.Pole     = ikState.pole;
            spec.Weight   = ikState.weight;
            rig->ScheduleTwoBoneIK(spec);
            rig->ScheduleFinalize(skelHnd, comp.CurrentPose);
        }

        // ---- Look-At IK -------------------------------------------------
        if (laState.enabled && laState.boneIdx >= 0)
        {
            Aether::LookAtSpec spec;
            spec.Skeleton   = skelHnd;
            spec.Pose       = comp.CurrentPose;
            spec.Bone       = laState.boneIdx;
            spec.Target     = laState.target;
            spec.Forward    = laState.forward;
            spec.Up         = laState.up;
            spec.Weight     = laState.weight;
            spec.AngleLimit = laState.angleLimit;
            rig->ScheduleLookAt(spec);
            rig->ScheduleFinalize(skelHnd, comp.CurrentPose);
        }

        // ---- Clip Blend -------------------------------------------------
        if (blState.enabled)
        {
            auto* clipAAsset = m_AssetManager->GetAsset<Aether::AClip>(
                comp.Clips[blState.clipAIdx]);
            auto* clipBAsset = m_AssetManager->GetAsset<Aether::AClip>(
                comp.Clips[blState.clipBIdx]);

            if (clipAAsset && clipBAsset)
            {
                auto poseA = rig->CreatePose(skelHnd);
                auto poseB = rig->CreatePose(skelHnd);

                rig->ScheduleSample(skelHnd, clipAAsset->m_Handle,
                    comp.Cache, poseA, comp.CurrentTime);
                rig->ScheduleSample(skelHnd, clipBAsset->m_Handle,
                    comp.Cache, poseB, comp.CurrentTime);

                if (blState.additive)
                    rig->ScheduleAdditive(poseA, poseB, comp.CurrentPose, blState.alpha);
                else
                    rig->ScheduleBlend(poseA, poseB, comp.CurrentPose, blState.alpha);

                rig->ScheduleFinalize(skelHnd, comp.CurrentPose);

                // Tasks will be flushed by the scene's second ProcessTasks() call.
                // Destroy temp poses after scheduling (handles are ref-counted).
                rig->DestroyPose(poseA);
                rig->DestroyPose(poseB);
            }
        }
    };
}

// =============================================================================
//  Update
// =============================================================================

void LabLayer::OnTick(Aether::Timestep ts)
{
    m_Scene.OnTick(ts);
}

void LabLayer::OnUpdate(Aether::Timestep ts)
{
    DrainParseQueue();

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.OnUpdate(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    m_VolShader->Bind();
    m_VolShader->SetFloat("u_Density",    m_VolDensity);
    m_VolShader->SetFloat("u_Intensity",  m_VolIntensity);
    m_VolShader->SetInt  ("u_Steps",      m_VolSteps);
    m_VolShader->SetFloat("u_VolBias",    m_ShadowBias);
    m_VolShader->SetFloat("u_MaxDistance", 100.0f);

    m_MainShader->Bind();
    m_MainShader->SetFloat("u_Bias", m_ShadowBias);

    m_Scene.OnUpdate(ts, &m_Camera);
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
    //Aether::UI::PerformanceOverlay(0);

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawAnimationPanel();
    DrawLightingPanel();
    DrawScriptingPanel();
    DrawBoneAttachmentPanel();
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
                if (m_Scene.HasComponent<ScriptComponent>(m_ScriptTargetEntity))
                {
                    auto& sc = m_Scene.GetComponent<ScriptComponent>(m_ScriptTargetEntity);
                    if (sc.ScriptHandle.IsValid()) 
                        Aether::ServiceManager::GetService<Aether::ScriptEngine>()->DestroyInstance(sc.ScriptHandle);
                    m_Scene.RemoveComponent<ScriptComponent>(m_ScriptTargetEntity);
                }

                auto script = Aether::ServiceManager::GetService<Aether::ScriptEngine>()->LoadScript(m_Importer->ImportText(m_ScriptPath));
                Handle<ScriptInstance> handle = Aether::ServiceManager::GetService<Aether::ScriptEngine>()->CreateInstance(
                    &m_Scene, m_ScriptTargetEntity, script);

                if (handle.IsValid())
                {
                    m_Scene.AddComponent<ScriptComponent>(m_ScriptTargetEntity, handle);
                    Aether::ServiceManager::GetService<Aether::ScriptEngine>()->StartInstance(handle);
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
                    Aether::ServiceManager::GetService<Aether::ScriptEngine>()->DestroyInstance(sc.ScriptHandle);
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
            auto  g   = UI::ID((int)(uint64_t)entity);
            auto& tag = m_Scene.GetComponent<TagComponent>(entity);
            auto& sc  = m_Scene.GetComponent<ScriptComponent>(entity);
            UI::Text("%s  (slot %d)", tag.Tag.c_str(), sc.ScriptHandle.index);
        }
    }
}

// =============================================================================
//  Hierarchy panel
// =============================================================================

void LabLayer::DrawHierarchyPanel()
{
    Aether::UI::SceneHierarchy("Hierarchy", m_Scene, m_SelectedEntity);
}

// =============================================================================
//  Scene panel
// =============================================================================

void LabLayer::DrawScenePanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Scene"))
    {
        int animatorCount = 0;
        for (auto e : m_Scene.View<AnimatorComponent>()) { (void)e; animatorCount++; }

        UI::Text("Meshes:    %d", (int)m_MeshIDs.size());
        UI::Text("Animators: %d", animatorCount);
        UI::Separator();

        // ---- Physics --------------------------------------------------------
        if (auto h = UI::Header("Physics"))
        {
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

            std::vector<std::string> meshNames;
            meshNames.reserve(m_MeshIDs.size());
            auto it = Aether::ServiceManager::GetService<AssetRegister>();
            for (auto& id : m_MeshIDs)
                meshNames.push_back(it->GetInfo(id)); 
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

            UI::Separator();
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
    }
}

// =============================================================================
//  Animation panel
// =============================================================================

void LabLayer::RefreshJointCache(
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
    auto* skelAsset = Aether::ServiceManager::GetService<Aether::AssetManager>()->GetAsset<Aether::ASkeleton>(anim.Skeleton);
    if (!skelAsset) return;

    auto skelHnd = skelAsset->m_Handle;
    int count = skelAsset->m_JointCount;
    cache.reserve(count);
    for (int i = 0; i < count; i++)
        cache.push_back(std::to_string(i) + "  " + rig->GetJointName(skelHnd, i));
}

bool LabLayer::JointCombo(const char* label, int& selectedIdx,
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
        auto rigSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
        if (!rigSystem) { UI::TextDisabled("RigSystem not initialized."); return; }

        // ---- Bind mesh to animator ------------------------------------------
        if (auto h = UI::Header("Bind Mesh to Animator"))
        {
            std::vector<std::string> meshNames;
            meshNames.reserve(m_MeshIDs.size());
            for (int i = 0; i < (int)m_MeshIDs.size(); i++)
                meshNames.push_back(std::to_string(i) + ": " + Aether::ServiceManager::GetService<AssetRegister>()->GetInfo(m_MeshIDs[i]));
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
                        auto* mesh = m_AssetManager->GetAsset<AMesh>(mc.Mesh);
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
                auto* mesh = m_AssetManager->GetAsset<AMesh>(mc.Mesh);
                auto& anim = m_Scene.GetComponent<AnimatorComponent>(entity);

                std::string meshName = mesh ? Aether::ServiceManager::GetService<AssetRegister>()->GetInfo(mesh->id) : "(invalid)";
                std::string skelName = anim.Skeleton.IsValid()
                    ? Aether::ServiceManager::GetService<AssetRegister>()->GetInfo(m_AssetManager->GetAsset<ASkeleton>(anim.Skeleton)->id)
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
                UI::AnimatorControls(anim, rigSystem);
            UI::Spacing();
        }
        if (!anyAnimators)
            UI::TextDisabled("No animators in scene.");

        UI::Separator();

        // =====================================================================
        //  IK / Advanced section
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
                            // Clear the old entity's callback before switching
                            if (m_IKAnimatorEntity != Null_Entity &&
                                m_Scene.IsValid(m_IKAnimatorEntity) &&
                                m_Scene.HasComponent<AnimatorComponent>(m_IKAnimatorEntity))
                            {
                                m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity)
                                    .onPostEvaluate = nullptr;
                            }

                            m_IKAnimatorEntity   = entity;
                            m_JointBrowserEntity = Null_Entity; // invalidate joint cache
                            m_CachedJointNames.clear();

                            // Immediately wire up the callback on the new entity
                            RebuildPostEvaluate();
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            RefreshJointCache(m_IKAnimatorEntity, m_JointBrowserEntity,
                              m_CachedJointNames, m_Scene, rigSystem);

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

                bool changed = false;
                changed |= UI::Checkbox("Enable##tbik", m_TwoBoneIK.enabled);
                changed |= JointCombo("Root Joint##tbik", m_TwoBoneIK.rootIdx, m_CachedJointNames);
                changed |= JointCombo("Mid Joint##tbik",  m_TwoBoneIK.midIdx,  m_CachedJointNames);
                changed |= JointCombo("End Joint##tbik",  m_TwoBoneIK.endIdx,  m_CachedJointNames);
                changed |= UI::DragXYZ("Target##tbik", m_TwoBoneIK.target, 0.01f);
                changed |= UI::DragXYZ("Pole##tbik",   m_TwoBoneIK.pole,   0.01f);
                changed |= UI::SliderFloat("Weight##tbik", m_TwoBoneIK.weight, 0.f, 1.f);

                if (changed) RebuildPostEvaluate();

                if (m_TwoBoneIK.enabled)
                    UI::TextDisabled("IK runs every frame via onPostEvaluate.");
            }

            // ---- Look-At IK -------------------------------------------------
            UI::Separator();
            UI::SectionHeader("Look-At IK");
            {
                auto d = UI::Disabled(!hasAnimator);

                bool changed = false;
                changed |= UI::Checkbox("Enable##lookat", m_LookAt.enabled);
                changed |= JointCombo("Bone##lookat",       m_LookAt.boneIdx,    m_CachedJointNames);
                changed |= UI::DragXYZ("Target##lookat",  m_LookAt.target,  0.01f);
                changed |= UI::DragXYZ("Forward##lookat", m_LookAt.forward, 0.01f);
                changed |= UI::DragXYZ("Up##lookat",      m_LookAt.up,      0.01f);
                changed |= UI::SliderFloat("Weight##lookat",          m_LookAt.weight,     0.f, 1.f);
                changed |= UI::SliderFloat("Angle Limit (rad)##lookat", m_LookAt.angleLimit, 0.f, glm::pi<float>());

                if (changed) RebuildPostEvaluate();

                if (m_LookAt.enabled)
                    UI::TextDisabled("Look-At runs every frame via onPostEvaluate.");
            }

            // ---- Blend ------------------------------------------------------
            UI::Separator();
            UI::SectionHeader("Clip Blend");
            {
                auto d = UI::Disabled(!hasAnimator);

                bool changed = false;
                changed |= UI::Checkbox("Enable##blend",   m_Blend.enabled);
                changed |= UI::Checkbox("Additive##blend", m_Blend.additive);

                if (hasAnimator)
                {
                    auto& anim = m_Scene.GetComponent<AnimatorComponent>(m_IKAnimatorEntity);
                    std::vector<std::string> clipNames;
                    for (int i = 0; i < (int)anim.Clips.size(); i++)
                        clipNames.push_back("Clip " + std::to_string(i));

                    changed |= UI::ComboList("Clip A##blend", clipNames, m_Blend.clipAIdx);
                    changed |= UI::ComboList("Clip B##blend", clipNames, m_Blend.clipBIdx);
                }

                changed |= UI::SliderFloat("Alpha##blend", m_Blend.alpha, 0.f, 1.f);

                if (changed) RebuildPostEvaluate();

                if (m_Blend.enabled)
                    UI::TextDisabled("Blend runs every frame via onPostEvaluate.");
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
        if (auto h = UI::Header("Attach"))
        {
            // --- Child entity picker -----------------------------------------
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

            // --- Animator entity picker --------------------------------------
            std::string animPreview = (m_BoneAttachAnimatorEntity != Null_Entity &&
                                       m_Scene.IsValid(m_BoneAttachAnimatorEntity))
                ? m_Scene.GetComponent<TagComponent>(m_BoneAttachAnimatorEntity).Tag
                : "Select Animator Entity";

            if (auto c = UI::Combo("Animator Entity##boneattach", animPreview.c_str()))
            {
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

            // --- Joint picker ------------------------------------------------
            {
                auto rigSystem = Aether::ServiceManager::GetService<AnimationSystem>()->GetModule<Aether::RigModule>();
                if (rigSystem)
                    RefreshJointCache(m_BoneAttachAnimatorEntity, m_JointBrowserEntity,
                                      m_CachedJointNames, m_Scene, rigSystem);
            }
            {
                std::string preview = (m_BoneNameBuf[0] != '\0') ? m_BoneNameBuf : "-- pick joint --";
                if (ImGui::BeginCombo("Joint##boneattach", preview.c_str()))
                {
                    for (int i = 0; i < (int)m_CachedJointNames.size(); i++)
                    {
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
            ImGui::InputText("Joint Name (manual)##boneattach", m_BoneNameBuf, sizeof(m_BoneNameBuf));

            UI::Separator();

            // --- Attach button -----------------------------------------------
            bool boneName   = (m_BoneNameBuf[0] != '\0');
            bool childValid = (m_BoneAttachChildEntity != Null_Entity &&
                               m_Scene.IsValid(m_BoneAttachChildEntity));
            bool animValid  = (m_BoneAttachAnimatorEntity != Null_Entity &&
                               m_Scene.IsValid(m_BoneAttachAnimatorEntity));
            bool notSelf    = (m_BoneAttachChildEntity != m_BoneAttachAnimatorEntity);
            bool canAttach  = boneName && childValid && animValid && notSelf;

            {
                auto d = UI::Disabled(!canAttach);
                if (UI::Button("Attach##boneattach"))
                {
                    if (m_Scene.HasComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity))
                    {
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

                    m_Scene.GetComponent<TransformComponent>(m_BoneAttachChildEntity).Dirty = true;

                    AE_CORE_INFO("[BoneAttach] '{}' -> bone '{}' on '{}'",
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachChildEntity).Tag,
                        m_BoneNameBuf,
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachAnimatorEntity).Tag);
                }
            }

            // --- Detach button -----------------------------------------------
            bool hasAttach = childValid &&
                             m_Scene.HasComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);
            ImGui::SameLine();
            {
                auto d = UI::Disabled(!hasAttach);
                if (UI::Button("Detach##boneattach"))
                {
                    m_Scene.RemoveComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);
                    m_Scene.GetComponent<TransformComponent>(m_BoneAttachChildEntity).Dirty = true;

                    AE_CORE_INFO("[BoneAttach] Detached '{}' from bone.",
                        m_Scene.GetComponent<TagComponent>(m_BoneAttachChildEntity).Tag);
                }
            }

            if (hasAttach)
            {
                auto& existing = m_Scene.GetComponent<BoneAttachmentComponent>(m_BoneAttachChildEntity);

                if (m_BoneAttachAnimatorEntity != existing.AnimatorEntity)
                    m_BoneAttachAnimatorEntity = existing.AnimatorEntity;

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

                std::string animTag = "(invalid)";
                if (attach.AnimatorEntity != Null_Entity &&
                    m_Scene.IsValid(attach.AnimatorEntity))
                    animTag = m_Scene.GetComponent<TagComponent>(attach.AnimatorEntity).Tag;

                UI::Text("'%s'  ->  bone '%s'  on  '%s'",
                    tag.Tag.c_str(),
                    attach.JointName.c_str(),
                    animTag.c_str());

                ImGui::SameLine();
                if (UI::SmallButton("Select##bonerow"))
                {
                    m_BoneAttachChildEntity = entity;
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