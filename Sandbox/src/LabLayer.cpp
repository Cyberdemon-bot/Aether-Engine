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

    // --- SHADOW PASS ---
    Aether::FramebufferSpec shadowSpec;
    shadowSpec.Width       = 2048;
    shadowSpec.Height      = 2048;
    shadowSpec.Attachments = { Aether::ImageFormat::DEPTH24STENCIL8 };
    m_ShadowFbo = Aether::FrameBuffer::Create(shadowSpec);

    m_ShadowShader = Aether::Shader::Create("assets/shaders/ShadowMap.shader");
    m_ShadowShader->Bind();
    m_ShadowShader->SetUBOSlot("Bones",  1);
    m_ShadowShader->SetUBOSlot("Lights", 2);

    Aether::RenderPass shadowPass;
    shadowPass.TargetFBO     = m_ShadowFbo.get();
    shadowPass.Shader        = m_ShadowShader.get();
    shadowPass.ClearDepth    = true;
    shadowPass.ClearColor    = false;
    shadowPass.OnScreen      = false;
    shadowPass.UsingMaterial = false;
    shadowPass.CullFace      = Aether::State::FRONT_CULL;
    shadowPass.attribList    = { {"u_LightIndex", 0} };

    // --- MAIN PASS ---
    Aether::FramebufferSpec mainSpec;
    mainSpec.Width       = window.GetWidth();
    mainSpec.Height      = window.GetHeight();
    mainSpec.Attachments = { Aether::ImageFormat::RGBA8, Aether::ImageFormat::DEPTH24STENCIL8 };
    m_MainFbo = Aether::FrameBuffer::Create(mainSpec);

    m_MainShader = Aether::Shader::Create("assets/shaders/Standard.shader");
    m_MainShader->Bind();
    m_MainShader->SetUBOSlot("Camera", 0);
    m_MainShader->SetUBOSlot("Bones",  1);
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
    mainPass.readList    = { {"u_DepthTex", m_ShadowFbo->GetDepthAttachment()} };
    mainPass.attribList  = { {"u_LightIndex", 0} };

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
    volPass.readList      = {
        { "u_SceneColor", m_MainFbo->GetColorAttachment()  },
        { "u_SceneDepth", m_MainFbo->GetDepthAttachment()  },
        { "u_ShadowMap",  m_ShadowFbo->GetDepthAttachment()}
    };

    m_Pipeline = { shadowPass, mainPass, volPass };
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
    Aether::ConsoleLayer::RegisterCommand("add",  AE_BIND_CONSOLE_FN(AddEntity));

    AE_CORE_INFO("LabLayer initialized!");
}

void LabLayer::Detach()
{
    for (auto& [entity, entry] : m_PhysicsBodies)
        Aether::PhysicsSystem::DestroyBody(entry.bodyID);
    m_PhysicsBodies.clear();

    m_ShadowShader.reset();
    m_MainShader.reset();
    m_VolShader.reset();
    m_ShadowFbo.reset();
    m_MainFbo.reset();
    m_VolFbo.reset();

    m_MeshIDs.clear();
    m_AnimatorIDs.clear();
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

    Aether::JobSystem::SubmitJob([this, path]()
    {
        AE_CORE_INFO("Worker: Parsing {0}", path);
        auto parsed = Aether::Importer::Import(path);
        {
            std::lock_guard<std::mutex> lock(m_ParseMutex);
            m_CompletedParses.push(std::move(parsed));
        }
        AE_CORE_INFO("Worker: Parsing complete for {0}", path);
    });
}

void LabLayer::DrainParseQueue()
{
    std::queue<Aether::ParsedScene> localQueue;
    {
        std::lock_guard<std::mutex> lock(m_ParseMutex);
        std::swap(localQueue, m_CompletedParses);
    }

    while (!localQueue.empty())
    {
        auto parsed = std::move(localQueue.front());
        localQueue.pop();

        AE_CORE_INFO("Main thread: Uploading to GPU...");
        auto result = Aether::Importer::Upload(parsed);

        for (auto& meshID  : result.meshIDs)     m_MeshIDs.push_back(meshID);
        for (auto& animID  : result.animatorIDS) m_AnimatorIDs.push_back(animID);

        m_Scene.LoadHierarchy(result);

        AE_CORE_INFO("Loaded: {0} mesh(es), {1} animator(s)",
            result.meshIDs.size(), result.animatorIDS.size());
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
    t.Dirty          = false;

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

    Aether::UUID bodyID;
    Aether::PhysicsSystem::CreateBody(bodyID, config);

    if (!m_Scene.HasComponent<Aether::ColliderComponent>(transformEntity))
        m_Scene.AddComponent<Aether::ColliderComponent>(transformEntity, bodyID, true);
    else
    {
        auto& col         = m_Scene.GetComponent<Aether::ColliderComponent>(transformEntity);
        col.BodyID        = bodyID;
        col.ColliderOffset = localOffset;
    }

    PhysicsEntry entry;
    entry.bodyID     = bodyID;
    entry.enabled    = false;
    entry.lastActive = false;
    entry.isDynamic  = isDynamic;
    m_PhysicsBodies[transformEntity] = entry;

    if (isDynamic)
        Aether::PhysicsSystem::SetActive(bodyID, false);
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
            Aether::PhysicsSystem::SetActive(entry.bodyID, entry.enabled);
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
    using namespace Aether;

    if (auto w = UI::Window("Performance"))
    {
        UI::Text("FPS: %.1f",           ImGui::GetIO().Framerate);
        UI::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    }

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawAnimationPanel();
    DrawLightingPanel();
}

// =============================================================================
//  Hierarchy panel
// =============================================================================

void LabLayer::DrawEntityNode(Aether::Entity entity)
{
    auto& tag  = m_Scene.GetComponent<Aether::TagComponent>(entity);
    auto& hier = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (hier.firstChild  == Aether::Null_Entity) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedEntity == entity)              flags |= ImGuiTreeNodeFlags_Selected;

    // TreeNodeEx with void* id has no clean wrapper — keep raw call
    bool open = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", tag.Tag.c_str());

    if (ImGui::IsItemClicked())
        m_SelectedEntity = entity;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Set as parent of selected") &&
            m_SelectedEntity != Aether::Null_Entity &&
            m_SelectedEntity != entity)
        {
            m_Scene.MakeParent(m_SelectedEntity, entity);
        }
        if (ImGui::MenuItem("Unparent") && hier.parent != Aether::Null_Entity)
            m_Scene.BreakParent(entity);
        ImGui::EndPopup();
    }

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

void LabLayer::DrawHierarchyPanel()
{
    if (auto w = Aether::UI::Window("Hierarchy"))
    {
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            m_SelectedEntity = Aether::Null_Entity;

        auto view = m_Scene.View<Aether::HierarchyComponent>();
        for (auto entity : view)
        {
            if (m_Scene.GetComponent<Aether::HierarchyComponent>(entity).parent == Aether::Null_Entity)
                DrawEntityNode(entity);
        }
    }
}

// =============================================================================
//  Scene panel
// =============================================================================

void LabLayer::DrawScenePanel()
{
    using namespace Aether;

    if (auto w = UI::Window("Scene"))
    {
        UI::Text("Meshes:    %d", (int)m_MeshIDs.size());
        UI::Text("Animators: %d", (int)m_AnimatorIDs.size());
        UI::Separator();

        // ---- Physics --------------------------------------------------------
        if (auto h = UI::Header("Physics"))
        {
            std::string nodePreview = (m_PhysSelectedEntity != Null_Entity && m_Scene.IsValid(m_PhysSelectedEntity))
                ? m_Scene.GetComponent<TagComponent>(m_PhysSelectedEntity).Tag
                : "Select Node";

            if (auto c = UI::Combo("Node##phys", nodePreview.c_str()))
            {
                auto view = m_Scene.View<TagComponent>();
                for (auto entity : view)
                {
                    auto guard = UI::ID((int)(uint64_t)entity);
                    bool selected = (m_PhysSelectedEntity == entity);
                    std::string tag = m_Scene.GetComponent<TagComponent>(entity).Tag;
                    if (ImGui::Selectable(tag.c_str(), selected)) m_PhysSelectedEntity = entity;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
            }

            std::string meshPreview = (m_PhysMeshIdx >= 0 && m_PhysMeshIdx < (int)m_MeshIDs.size())
                ? AssetsRegister::Get(m_MeshIDs[m_PhysMeshIdx])
                : "Select Mesh";

            if (auto c = UI::Combo("Mesh##phys", meshPreview.c_str()))
            {
                for (int i = 0; i < (int)m_MeshIDs.size(); i++)
                {
                    auto guard = UI::ID(i);
                    bool selected = (m_PhysMeshIdx == i);
                    if (ImGui::Selectable(AssetsRegister::Get(m_MeshIDs[i]).c_str(), selected)) m_PhysMeshIdx = i;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
            }

            UI::Checkbox("Is Dynamic", m_PhysDynamic);

            bool canAdd = (m_PhysSelectedEntity != Null_Entity &&
                           m_Scene.IsValid(m_PhysSelectedEntity) &&
                           m_PhysMeshIdx >= 0 && m_PhysMeshIdx < (int)m_MeshIDs.size());

            if (!canAdd) ImGui::BeginDisabled();
            if (UI::Button("Add Physics Body"))
                RegisterPhysicsBody(m_PhysSelectedEntity, m_MeshIDs[m_PhysMeshIdx], m_PhysDynamic);
            if (!canAdd) ImGui::EndDisabled();

            UI::Separator();
            UI::Text("Active Bodies:");
            for (auto& [entity, entry] : m_PhysicsBodies)
            {
                if (!m_Scene.IsValid(entity)) continue;
                auto guard = UI::ID((int)(uint64_t)entity);
                std::string tag = m_Scene.GetComponent<TagComponent>(entity).Tag;
                UI::Checkbox(tag.c_str(), entry.enabled);
            }

            UI::Separator();

            auto physIt = (m_SelectedEntity != Null_Entity)
                ? m_PhysicsBodies.find(m_SelectedEntity)
                : m_PhysicsBodies.end();

            UI::Text("Apply to Selected Entity:");
            if (physIt == m_PhysicsBodies.end())
                UI::TextDisabled("(select an entity with a physics body)");
            else if (!physIt->second.isDynamic)
                UI::TextDisabled("(static body — forces not applicable)");
            else
            {
                UI::DragFloat3("Force##input",    m_ForceInput,    0.5f);
                UI::SameLine();
                if (UI::Button("Apply Force"))
                    PhysicsSystem::AddForce(physIt->second.bodyID, m_ForceInput);
                UI::SameLine();
                if (UI::SmallButton("X##force")) m_ForceInput = glm::vec3(0.0f);

                UI::DragFloat3("Velocity##input", m_VelocityInput, 0.5f);
                UI::SameLine();
                if (UI::Button("Set Velocity"))
                    PhysicsSystem::SetVelocity(physIt->second.bodyID, m_VelocityInput);
                UI::SameLine();
                if (UI::SmallButton("X##vel")) m_VelocityInput = glm::vec3(0.0f);
            }
        }

        // ---- Raycast --------------------------------------------------------
        if (auto h = UI::Header("Raycast Test"))
        {
            if (UI::Button("Fill from Camera"))
            {
                m_RayOrigin    = m_Camera.GetPosition();
                m_RayDirection = m_Camera.GetForwardDirection();
            }
            UI::SameLine();
            UI::TextDisabled("(or set manually below)");

            UI::DragFloat3("Origin##ray",    m_RayOrigin,    0.1f);
            if (UI::DragFloat3("Direction##ray", m_RayDirection, 0.01f))
            {
                float len = glm::length(m_RayDirection);
                if (len > 1e-5f) m_RayDirection /= len;
            }
            UI::SliderFloat("Max Distance##ray", m_RayDistance, 0.1f, 1000.0f);

            UI::Spacing();
            if (UI::Button("Cast Ray"))
            {
                m_LastRayHits = PhysicsSystem::CastRayAll(m_RayOrigin, m_RayDirection, m_RayDistance);
                m_RayHasFired = true;
            }

            if (m_RayHasFired)
            {
                UI::Separator();
                UI::Text("Result: %d hit(s)", (int)m_LastRayHits.size());

                if (m_LastRayHits.empty())
                {
                    UI::TextColored(UI::Color::Red(), "  MISS");
                    UI::TextDisabled("  (no collider hit within %.1f units)", m_RayDistance);
                }
                else
                {
                    for (int i = 0; i < (int)m_LastRayHits.size(); i++)
                    {
                        const auto& hit   = m_LastRayHits[i];
                        auto        guard = UI::ID(i);
                        std::string name  = AssetsRegister::Get(hit.HitEntityID);
                        std::string label = name.empty() ? "(unregistered)" : name;

                        if (auto t = UI::TreeNode(label.c_str(),
                            ImGuiTreeNodeFlags_SpanAvailWidth))
                        {
                            UI::TextColored(UI::Color::Green(), "HIT");
                            UI::Text("Position : (%.3f, %.3f, %.3f)", hit.Position.x, hit.Position.y, hit.Position.z);
                            UI::Text("Normal   : (%.3f, %.3f, %.3f)", hit.Normal.x,   hit.Normal.y,   hit.Normal.z);
                            UI::Text("Distance : %.3f", hit.Distance);
                        }
                    }
                }
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
            if (m_SelectedEntity != Null_Entity && m_Scene.IsValid(m_SelectedEntity))
            {
                auto& tag = m_Scene.GetComponent<TagComponent>(m_SelectedEntity);
                auto& t   = m_Scene.GetComponent<TransformComponent>(m_SelectedEntity);

                UI::TextColored(UI::Color::Green(), "Editing: %s", tag.Tag.c_str());

                if (UI::DragFloat3("Position", t.Translation, 0.1f))  t.Dirty = true;
                if (ImGui::DragFloat4("Rotation", glm::value_ptr(t.Rotation), 0.01f)) t.Dirty = true;
                if (UI::DragFloat3("Scale", t.Scale, 0.05f))           t.Dirty = true;

                if (UI::Button("Reset Transform"))
                {
                    t.Translation = glm::vec3(0.0f);
                    t.Rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    t.Scale       = glm::vec3(1.0f);
                    t.Dirty       = true;
                }

                if (t.Dirty)
                {
                    auto it = m_PhysicsBodies.find(m_SelectedEntity);
                    if (it != m_PhysicsBodies.end())
                        PhysicsSystem::SetPhysTransform(it->second.bodyID, { t.Translation, t.Rotation });
                }
            }
            else
                UI::TextDisabled("Select an entity in the Hierarchy panel.");
        }
    }
}

// =============================================================================
//  Animation panel
// =============================================================================

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
            std::string meshPreview = (m_BindMeshIndex >= 0 && m_BindMeshIndex < (int)m_MeshIDs.size())
                ? AssetsRegister::Get(m_MeshIDs[m_BindMeshIndex])
                : "Select Mesh";

            if (auto c = UI::Combo("Mesh##bind", meshPreview.c_str()))
            {
                for (int i = 0; i < (int)m_MeshIDs.size(); i++)
                {
                    auto  guard    = UI::ID(i);
                    bool  selected = (m_BindMeshIndex == i);
                    std::string name = std::to_string(i) + ": " + AssetsRegister::Get(m_MeshIDs[i]);
                    if (ImGui::Selectable(name.c_str(), selected)) m_BindMeshIndex = i;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
            }

            std::string animPreview = (m_BindAnimatorIndex >= 0 && m_BindAnimatorIndex < (int)m_AnimatorIDs.size())
                ? ("Animator " + std::to_string(m_BindAnimatorIndex))
                : "Select Animator";

            if (auto c = UI::Combo("Animator##bind", animPreview.c_str()))
            {
                for (int i = 0; i < (int)m_AnimatorIDs.size(); i++)
                {
                    auto guard    = UI::ID(i);
                    bool selected = (m_BindAnimatorIndex == i);
                    if (ImGui::Selectable(("Animator " + std::to_string(i)).c_str(), selected))
                        m_BindAnimatorIndex = i;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
            }

            bool canBind = (m_BindMeshIndex    >= 0 && m_BindMeshIndex    < (int)m_MeshIDs.size() &&
                            m_BindAnimatorIndex >= 0 && m_BindAnimatorIndex < (int)m_AnimatorIDs.size());

            if (!canBind) ImGui::BeginDisabled();
            if (UI::Button("Bind"))
            {
                UUID meshID = m_MeshIDs[m_BindMeshIndex];
                UUID animID = m_AnimatorIDs[m_BindAnimatorIndex];

                auto meshView = m_Scene.View<MeshComponent>();
                for (auto entity : meshView)
                {
                    auto& mc   = m_Scene.GetComponent<MeshComponent>(entity);
                    auto* mesh = AssetManager::GetAsset<Mesh>(mc.Mesh);
                    if (mesh && mesh->id == meshID)
                    {
                        if (!m_Scene.HasComponent<AnimatorComponent>(entity))
                            m_Scene.AddComponent<AnimatorComponent>(entity);
                        m_Scene.GetComponent<AnimatorComponent>(entity).AnimatorID = animID;
                        break;
                    }
                }
            }
            if (!canBind) ImGui::EndDisabled();

            UI::Separator();
            UI::Text("Active Bindings:");

            auto meshView = m_Scene.View<MeshComponent, AnimatorComponent>();
            for (auto entity : meshView)
            {
                auto& mc    = m_Scene.GetComponent<MeshComponent>(entity);
                auto* mesh  = AssetManager::GetAsset<Mesh>(mc.Mesh);
                UUID animID = m_Scene.GetComponent<AnimatorComponent>(entity).AnimatorID;

                std::string meshName = mesh ? AssetsRegister::Get(mesh->id) : "(invalid)";
                int animIdx = -1;
                for (int i = 0; i < (int)m_AnimatorIDs.size(); i++)
                    if (m_AnimatorIDs[i] == animID) { animIdx = i; break; }

                auto guard = UI::ID(mesh ? (int)(uint64_t)mesh->id : 0);
                UI::Text("%s  ->  Animator %d", meshName.c_str(), animIdx);
                UI::SameLine();
                if (UI::SmallButton("Unbind"))
                    m_Scene.RemoveComponent<AnimatorComponent>(entity);
            }
        }

        UI::Separator();

        if (m_AnimatorIDs.empty())
        {
            UI::TextDisabled("No animators loaded. Use 'load <path>' in the console.");
            return;
        }

        // ---- Per-animator controls ------------------------------------------
        for (int i = 0; i < (int)m_AnimatorIDs.size(); i++)
        {
            UUID animatorID = m_AnimatorIDs[i];
            auto guard      = UI::ID(i);

            if (auto h = UI::Header(("Animator " + std::to_string(i)).c_str()))
            {
                auto  clips      = rigSystem->GetClips(animatorID);
                int   currentIdx = rigSystem->GetCurrentClipIndex(animatorID);

                std::string clipPreview = (currentIdx >= 0 && currentIdx < (int)clips.size())
                    ? AssetsRegister::Get(clips[currentIdx])
                    : "Select Clip";

                if (auto c = UI::Combo("Clip", clipPreview.c_str()))
                {
                    for (int ci = 0; ci < (int)clips.size(); ci++)
                    {
                        auto cg  = UI::ID(ci);
                        bool sel = (ci == currentIdx);
                        if (ImGui::Selectable(AssetsRegister::Get(clips[ci]).c_str(), sel))
                            rigSystem->BindClip(animatorID, clips[ci]);
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                }

                UI::Separator();

                bool isPlaying = rigSystem->IsPlaying(animatorID);
                if (UI::Button("Play"))  rigSystem->Play(animatorID);
                UI::SameLine();
                if (UI::Button("Pause")) rigSystem->Pause(animatorID);
                UI::SameLine();
                if (UI::Button("Stop"))  rigSystem->Stop(animatorID);
                UI::SameLine();
                UI::TextColored(isPlaying ? UI::Color::Green() : UI::Color::Red(),
                                isPlaying ? " PLAYING" : " STOPPED");

                float speed = rigSystem->GetSpeed(animatorID);
                if (UI::SliderFloat("Speed", speed, 0.0f, 3.0f))
                    rigSystem->SetSpeed(animatorID, speed);

                float currentTime = rigSystem->GetPlayBackTime(animatorID);
                float duration    = rigSystem->GetDuration(animatorID);
                UI::Text("Time: %.2f / %.2f", currentTime, duration);
                if (duration > 0.0f)
                    UI::ProgressBar(currentTime / duration);
            }

            UI::Spacing();
        }
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
            auto& light      = lightComp.Config;

            glm::vec3 dir = glm::normalize(glm::vec3(-lightTrans.WorldTransform[2]));
            UI::Text("Direction: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
            UI::ColorEdit3("Color",      light.color);
            UI::SliderFloat("Intensity", light.intensity, 0.0f, 10.0f);
            UI::SliderFloat("Range",     light.range,     1.0f, 200.0f);

            float innerDeg = glm::degrees(glm::acos(light.innerCone));
            float outerDeg = glm::degrees(glm::acos(light.outerCone));
            if (UI::SliderFloat("Inner Cone", innerDeg, 1.0f, 89.0f))
                light.innerCone = glm::cos(glm::radians(innerDeg));
            if (UI::SliderFloat("Outer Cone", outerDeg, innerDeg, 90.0f))
                light.outerCone = glm::cos(glm::radians(outerDeg));

            UI::Checkbox("Cast Shadows", light.castShadows);
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