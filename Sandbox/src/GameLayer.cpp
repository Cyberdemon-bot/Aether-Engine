#include "GameLayer.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/AssetsRegister.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Resources/Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

GameLayer::GameLayer()
    : Layer("Game Layer")
    , m_Camera(45.0f, 1.778f, 0.1f, 1000.0f)
{
    m_Camera.SetDistance(5.0f);
}

void GameLayer::Attach()
{
    ImGuiContext* ctx = Aether::ImGuiLayer::GetContext();
    if (ctx) ImGui::SetCurrentContext(ctx);

    Aether::FramebufferSpec shadowFbSpec;
    shadowFbSpec.Width       = 2048;
    shadowFbSpec.Height      = 2048;
    shadowFbSpec.Attachments = {  Aether::ImageFormat::DEPTH24STENCIL8 };

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
    mainPass.ClearValue  = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    mainPass.CullFace    = Aether::State::BACK_CULL;
    mainPass.OnScreen    = false;
    mainPass.readList    = {{"u_DepthTex", shadowPass.TargetFBO->GetDepthAttachment()}};
    mainPass.attribList  = {{"u_LightIndex", 0}};

    Aether::FramebufferSpec volFbSpec;
    volFbSpec.Width       = sceneFbSpec.Width;
    volFbSpec.Height      = sceneFbSpec.Height;
    volFbSpec.Attachments = {
        Aether::ImageFormat::RGBA8,
        Aether::ImageFormat::DEPTH24STENCIL8,
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

    Aether::LightParam dirLight;
    dirLight.type        = Aether::LightType::Directional;
    dirLight.direction   = glm::normalize(glm::vec3(
        glm::sin(glm::radians(m_DirLightEuler.y)) * glm::cos(glm::radians(m_DirLightEuler.x)),
       -glm::sin(glm::radians(m_DirLightEuler.x)),
        glm::cos(glm::radians(m_DirLightEuler.y)) * glm::cos(glm::radians(m_DirLightEuler.x))));
    dirLight.color       = glm::vec3(1.0f, 0.95f, 0.85f);
    dirLight.intensity   = 1.0f;
    dirLight.castShadows = false;

    m_DirLightEntity = m_Scene.CreateEntity("Directional Light");
    m_Scene.AddComponent<Aether::LightComponent>(m_DirLightEntity).Config = dirLight;
    auto& dirLightTransform       = m_Scene.GetComponent<Aether::TransformComponent>(m_DirLightEntity);
    dirLightTransform.Translation = glm::vec3(0.0f);
    dirLightTransform.Dirty       = true;

    Aether::ConsoleLayer::RegisterCommand("load", AE_BIND_CONSOLE_FN(LoadModelAsync));
    Aether::ConsoleLayer::RegisterCommand("add",  AE_BIND_CONSOLE_FN(AddEntity));

    AE_CORE_INFO("GameLayer initialized!");
}

void GameLayer::Detach()
{
    m_ShadowShader.reset();
    m_MainShader.reset();
    m_VolShader.reset();
    m_Meshes.clear();
    m_Animators.clear();

    for (auto& [entity, entry] : m_PhysicsBodies)
        Aether::PhysicsSystem::DestroyBody(entry.bodyID);
    m_PhysicsBodies.clear();
}

void GameLayer::AddEntity(const std::vector<std::string>& args)
{
    for (auto& name : args)
        m_Scene.CreateEntity(name);
}

void GameLayer::LoadModelAsync(const std::vector<std::string>& args)
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

void GameLayer::RegisterPhysicsBody(Aether::Entity transformEntity, Aether::UUID colliderMeshID, bool isDynamic)
{
    if (!m_Scene.IsValid(transformEntity)) return;

    auto mesh = Aether::AssetManager::GetResource<Aether::Mesh>(colliderMeshID);
    if (!mesh) return;

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

    auto& t      = m_Scene.GetComponent<Aether::TransformComponent>(transformEntity);
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

    std::string shapeName = [&]() {
        switch (config.shape)
        {
            case Aether::ColliderShape::Box:     return "Box";
            case Aether::ColliderShape::Sphere:  return "Sphere";
            case Aether::ColliderShape::Capsule: return "Capsule";
            default:                             return "None";
        }
    }();

    std::string meshName  = Aether::AssetsRegister::Get(colliderMeshID);
    Aether::UUID bodyID = Aether::AssetsRegister::Register(shapeName + "_" + meshName);
    Aether::PhysicsSystem::CreateBody(bodyID, config);

    if (!m_Scene.HasComponent<Aether::ColliderComponent>(transformEntity))
        m_Scene.AddComponent<Aether::ColliderComponent>(transformEntity, bodyID, true);
    else
    {
        auto& rb           = m_Scene.GetComponent<Aether::ColliderComponent>(transformEntity);
        rb.BodyID          = bodyID;
        rb.ColliderOffset  = localOffset;
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

void GameLayer::DrainParseQueue()
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

        for (auto& meshID : result.meshIDs)      m_Meshes.push_back(meshID);
        for (auto& animID : result.animatorIDS)  m_Animators.push_back(animID);

        m_Scene.LoadHierarchy(result);

        AE_CORE_INFO("Loaded: {0} mesh(es), {1} animator(s)",
            result.meshIDs.size(), result.animatorIDS.size());
    }
}

void GameLayer::Update(Aether::Timestep ts)
{
    DrainParseQueue();

    if (!ImGui::GetIO().WantCaptureKeyboard)
        m_Camera.Update(ts);

    auto& window = Aether::Application::Get().GetWindow();
    m_Camera.SetViewportSize((float)window.GetWidth(), (float)window.GetHeight());

    if (m_AutoRotate)
    {
        auto meshView = m_Scene.View<Aether::MeshComponent, Aether::TransformComponent>();
        for (auto entity : meshView)
        {
            auto& t    = m_Scene.GetComponent<Aether::TransformComponent>(entity);
            t.Rotation.y += ts * m_RotationSpeed;
            t.Dirty       = true;
        }
    }
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

void GameLayer::OnEvent(Aether::Event& event)
{
    if (!event.Handled)
        m_Camera.OnEvent(event);
}

// =============================================================================
//  ImGui
// =============================================================================

void GameLayer::OnImGuiRender()
{
    ImGui::Begin("Performance");
    ImGui::Text("FPS: %.1f",          ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::End();

    DrawHierarchyPanel();
    DrawScenePanel();
    DrawAnimationPanel();
    DrawLightingPanel();
}

// =============================================================================
//  Hierarchy panel
// =============================================================================

void GameLayer::DrawEntityNode(Aether::Entity entity)
{
    auto& tag  = m_Scene.GetComponent<Aether::TagComponent>(entity);
    auto& hier = m_Scene.GetComponent<Aether::HierarchyComponent>(entity);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                             | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (hier.firstChild  == Aether::Null_Entity) flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_SelectedEntity == entity)      flags |= ImGuiTreeNodeFlags_Selected;

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
        // Read nextSibling before recursing in case hierarchy mutates
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

void GameLayer::DrawHierarchyPanel()
{
    if (!ImGui::Begin("Hierarchy")) { ImGui::End(); return; }

    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        m_SelectedEntity = Aether::Null_Entity;

    auto view = m_Scene.View<Aether::HierarchyComponent>();
    for (auto entity : view)
    {
        if (m_Scene.GetComponent<Aether::HierarchyComponent>(entity).parent == Aether::Null_Entity)
            DrawEntityNode(entity);
    }

    ImGui::End();
}

void GameLayer::DrawScenePanel()
{
    if (!ImGui::Begin("Scene")) { ImGui::End(); return; }

    ImGui::Text("Meshes:    %d", (int)m_Meshes.size());
    ImGui::Text("Animators: %d", (int)m_Animators.size());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Physics"))
    {
        std::string nodePreview = (m_PhysSelectedEntity != Aether::Null_Entity && m_Scene.IsValid(m_PhysSelectedEntity))
            ? m_Scene.GetComponent<Aether::TagComponent>(m_PhysSelectedEntity).Tag
            : "Select Node";

        if (ImGui::BeginCombo("Node##phys", nodePreview.c_str()))
        {
            auto view = m_Scene.View<Aether::TagComponent>();
            for (auto entity : view)
            {
                bool selected = (m_PhysSelectedEntity == entity);
                std::string tag = m_Scene.GetComponent<Aether::TagComponent>(entity).Tag;
                ImGui::PushID((uint64_t)entity);
                if (ImGui::Selectable(tag.c_str(), selected))
                    m_PhysSelectedEntity = entity;
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        std::string meshPreview = (m_PhysMeshIdx >= 0 && m_PhysMeshIdx < (int)m_Meshes.size())
            ? Aether::AssetsRegister::Get(m_Meshes[m_PhysMeshIdx])
            : "Select Mesh";

        if (ImGui::BeginCombo("Mesh##phys", meshPreview.c_str()))
        {
            for (int i = 0; i < (int)m_Meshes.size(); i++)
            {
                ImGui::PushID(i);
                bool selected = (m_PhysMeshIdx == i);
                std::string name = Aether::AssetsRegister::Get(m_Meshes[i]);
                if (ImGui::Selectable(name.c_str(), selected))
                    m_PhysMeshIdx = i;
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Is Dynamic", &m_PhysDynamic);

        bool canAdd = (m_PhysSelectedEntity != Aether::Null_Entity &&
                       m_Scene.IsValid(m_PhysSelectedEntity) &&
                       m_PhysMeshIdx >= 0 &&
                       m_PhysMeshIdx < (int)m_Meshes.size());

        if (!canAdd) ImGui::BeginDisabled();
        if (ImGui::Button("Add Physics Body"))
            RegisterPhysicsBody(m_PhysSelectedEntity, m_Meshes[m_PhysMeshIdx], m_PhysDynamic);
        if (!canAdd) ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::Text("Active Bodies:");
        for (auto& [entity, entry] : m_PhysicsBodies)
        {
            if (!m_Scene.IsValid(entity)) continue;

            ImGui::PushID((uint64_t)entity);
            std::string tag = m_Scene.GetComponent<Aether::TagComponent>(entity).Tag;

            ImGui::Checkbox(tag.c_str(), &entry.enabled);

            ImGui::PopID();
        }

        ImGui::Separator();

        auto physIt = (m_SelectedEntity != Aether::Null_Entity)
            ? m_PhysicsBodies.find(m_SelectedEntity)
            : m_PhysicsBodies.end();
        bool hasPhysBody = (physIt != m_PhysicsBodies.end());

        ImGui::Text("Apply to Selected Entity:");
        if (!hasPhysBody) ImGui::TextDisabled("(select an entity with a physics body)");
        else if (!physIt->second.isDynamic) ImGui::TextDisabled("(static body — forces not applicable)");
        else
        {
            // Add Force
            ImGui::DragFloat3("Force##input",    glm::value_ptr(m_ForceInput),    0.5f);
            ImGui::SameLine();
            if (ImGui::Button("Apply Force")) Aether::PhysicsSystem::AddForce(physIt->second.bodyID, m_ForceInput);
            ImGui::SameLine();
            if (ImGui::SmallButton("X##force"))
                m_ForceInput = glm::vec3(0.0f);

            ImGui::DragFloat3("Velocity##input", glm::value_ptr(m_VelocityInput), 0.5f);
            ImGui::SameLine();
            if (ImGui::Button("Set Velocity")) Aether::PhysicsSystem::SetVelocity(physIt->second.bodyID, m_VelocityInput);
            ImGui::SameLine();
            if (ImGui::SmallButton("X##vel"))
                m_VelocityInput = glm::vec3(0.0f);
        }
    }

    if (ImGui::CollapsingHeader("Raycast Test"))
    {
        if (ImGui::Button("Fill from Camera"))
        {
            m_RayOrigin    = m_Camera.GetPosition();
            m_RayDirection = m_Camera.GetForwardDirection();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(or set manually below)");

        ImGui::DragFloat3("Origin##ray",    glm::value_ptr(m_RayOrigin),    0.1f);

        // Keep direction normalised whenever the user edits it
        if (ImGui::DragFloat3("Direction##ray", glm::value_ptr(m_RayDirection), 0.01f, -1.0f, 1.0f))
        {
            float len = glm::length(m_RayDirection);
            if (len > 1e-5f)
                m_RayDirection /= len;
        }

        ImGui::SliderFloat("Max Distance##ray", &m_RayDistance, 0.1f, 1000.0f, "%.1f");

        ImGui::Spacing();
        if (ImGui::Button("Cast Ray"))
        {
            m_LastRayHits = Aether::PhysicsSystem::CastRayAll(m_RayOrigin, m_RayDirection, m_RayDistance);
            m_RayHasFired = true;
        }

        // -- Results ----------------------------------------------------------
        if (m_RayHasFired)
        {
            ImGui::Separator();
            ImGui::Text("Result: %d hit(s)", (int)m_LastRayHits.size());

            if (m_LastRayHits.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "  MISS");
                ImGui::TextDisabled("  (no collider hit within %.1f units)", m_RayDistance);
            }
            else
            {
                for (int i = 0; i < (int)m_LastRayHits.size(); i++)
                {
                    const auto& hit = m_LastRayHits[i];
                    ImGui::PushID(i);

                    std::string resolvedName = Aether::AssetsRegister::Get(hit.HitEntityID);
                    std::string label = resolvedName.empty() ? "(unregistered)" : resolvedName;

                    bool open = ImGui::TreeNodeEx(label.c_str(),
                        ImGuiTreeNodeFlags_SpanAvailWidth,
                        "[%d] %s  (%.2f)", i, label.c_str(), hit.Distance);

                    if (open)
                    {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.3f, 1.0f), "HIT");
                        ImGui::Text("Position : (%.3f, %.3f, %.3f)", hit.Position.x, hit.Position.y, hit.Position.z);
                        ImGui::Text("Normal   : (%.3f, %.3f, %.3f)", hit.Normal.x,   hit.Normal.y,   hit.Normal.z);
                        ImGui::Text("Distance : %.3f", hit.Distance);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }
        }
    }

    // ---- Camera -------------------------------------------------------------
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 pos = m_Camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Distance: %.1f", m_Camera.GetDistance());
        if (ImGui::Button("Reset Camera"))
            m_Camera.SetDistance(5.0f);
    }

    // ---- Transform ----------------------------------------------------------
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (m_SelectedEntity != Aether::Null_Entity && m_Scene.IsValid(m_SelectedEntity))
        {
            auto& tag = m_Scene.GetComponent<Aether::TagComponent>(m_SelectedEntity);
            auto& t   = m_Scene.GetComponent<Aether::TransformComponent>(m_SelectedEntity);

            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Editing: %s", tag.Tag.c_str());

            if (ImGui::DragFloat3("Position", glm::value_ptr(t.Translation), 0.1f))
                t.Dirty = true;
            if (ImGui::DragFloat4("Rotation", glm::value_ptr(t.Rotation),    0.1f))
                t.Dirty = true;
            if (ImGui::DragFloat3("Scale",    glm::value_ptr(t.Scale),       0.05f, 0.01f, 100.0f))
                t.Dirty = true;

            if (ImGui::Button("Reset Transform"))
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
                    Aether::PhysicsSystem::SetPhysTransform(it->second.bodyID, { t.Translation, t.Rotation });
            }
        }
        else
        {
            ImGui::TextDisabled("Select an entity in the Hierarchy panel.");
        }
    }

    // ---- Auto rotation ------------------------------------------------------
    if (ImGui::CollapsingHeader("Auto Rotation"))
    {
        ImGui::Checkbox("Auto Rotate", &m_AutoRotate);
        if (m_AutoRotate)
            ImGui::SliderFloat("Speed", &m_RotationSpeed, -5.0f, 5.0f);
    }

    ImGui::End();
}

// =============================================================================
//  Animation panel
// =============================================================================

void GameLayer::DrawAnimationPanel()
{
    if (!ImGui::Begin("Animation")) { ImGui::End(); return; }

    auto rigSystem = Aether::AnimationSystem::GetModule<Aether::RigModule>();
    if (!rigSystem)
    {
        ImGui::TextDisabled("RigSystem not initialized.");
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Bind Mesh to Animator", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string meshPreview = (m_BindMeshIndex >= 0 && m_BindMeshIndex < (int)m_Meshes.size())
            ? Aether::AssetsRegister::Get(m_Meshes[m_BindMeshIndex])
            : "Select Mesh";

        if (ImGui::BeginCombo("Mesh##bind", meshPreview.c_str()))
        {
            for (int i = 0; i < (int)m_Meshes.size(); i++)
            {
                ImGui::PushID(i);
                bool selected = (m_BindMeshIndex == i);
                std::string name = std::to_string(i) + ": " + Aether::AssetsRegister::Get(m_Meshes[i]);
                if (ImGui::Selectable(name.c_str(), selected))
                    m_BindMeshIndex = i;
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        std::string animPreview = (m_BindAnimatorIndex >= 0 && m_BindAnimatorIndex < (int)m_Animators.size())
            ? ("Animator " + std::to_string(m_BindAnimatorIndex))
            : "Select Animator";

        if (ImGui::BeginCombo("Animator##bind", animPreview.c_str()))
        {
            for (int i = 0; i < (int)m_Animators.size(); i++)
            {
                ImGui::PushID(i);
                bool selected = (m_BindAnimatorIndex == i);
                if (ImGui::Selectable(("Animator " + std::to_string(i)).c_str(), selected))
                    m_BindAnimatorIndex = i;
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        bool canBind = (m_BindMeshIndex    >= 0 && m_BindMeshIndex    < (int)m_Meshes.size() &&
                        m_BindAnimatorIndex >= 0 && m_BindAnimatorIndex < (int)m_Animators.size());

        if (!canBind) ImGui::BeginDisabled();
        if (ImGui::Button("Bind"))
        {
            Aether::UUID meshID = m_Meshes[m_BindMeshIndex];
            Aether::UUID animID = m_Animators[m_BindAnimatorIndex];

            auto meshView = m_Scene.View<Aether::MeshComponent>();
            for (auto entity : meshView)
            {
                if (m_Scene.GetComponent<Aether::MeshComponent>(entity).MeshPtr == Aether::AssetManager::GetResource<Aether::Mesh>(meshID))
                {
                    if (!m_Scene.HasComponent<Aether::AnimatorComponent>(entity))
                        m_Scene.AddComponent<Aether::AnimatorComponent>(entity);
                    m_Scene.GetComponent<Aether::AnimatorComponent>(entity).AnimatorID = animID;
                    break;
                }
            }
        }
        if (!canBind) ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Text("Active Bindings:");

        auto meshView = m_Scene.View<Aether::MeshComponent, Aether::AnimatorComponent>();
        for (auto entity : meshView)
        {
            Aether::UUID meshID = m_Scene.GetComponent<Aether::MeshComponent>(entity).MeshPtr->id;
            Aether::UUID animID = m_Scene.GetComponent<Aether::AnimatorComponent>(entity).AnimatorID;

            std::string meshName = Aether::AssetsRegister::Get(meshID);
            int animIdx = -1;
            for (int i = 0; i < (int)m_Animators.size(); i++)
                if (m_Animators[i] == animID) { animIdx = i; break; }

            ImGui::PushID((uint64_t)meshID);
            ImGui::Text("%s  ->  Animator %d", meshName.c_str(), animIdx);
            ImGui::SameLine();
            if (ImGui::SmallButton("Unbind"))
                m_Scene.RemoveComponent<Aether::AnimatorComponent>(entity);
            ImGui::PopID();
        }
    }

    ImGui::Separator();

    if (m_Animators.empty())
    {
        ImGui::TextDisabled("No animators loaded. Use 'load <path>' in the console.");
        ImGui::End();
        return;
    }

    for (int i = 0; i < (int)m_Animators.size(); i++)
    {
        Aether::UUID animatorID = m_Animators[i];
        ImGui::PushID(i);

        if (ImGui::CollapsingHeader(("Animator " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::vector<Aether::UUID> clips      = rigSystem->GetClips(animatorID);
            int                       currentIdx = rigSystem->GetCurrentClipIndex(animatorID);

            std::string clipPreview = (currentIdx >= 0 && currentIdx < (int)clips.size())
                ? Aether::AssetsRegister::Get(clips[currentIdx])
                : "Select Clip";

            if (ImGui::BeginCombo("Clip", clipPreview.c_str()))
            {
                for (int c = 0; c < (int)clips.size(); c++)
                {
                    ImGui::PushID(c);
                    bool sel = (c == currentIdx);
                    if (ImGui::Selectable(Aether::AssetsRegister::Get(clips[c]).c_str(), sel))
                        rigSystem->BindClip(animatorID, clips[c]);
                    if (sel) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            bool isPlaying = rigSystem->IsPlaying(animatorID);
            if (ImGui::Button("Play"))  rigSystem->Play(animatorID);
            ImGui::SameLine();
            if (ImGui::Button("Pause")) rigSystem->Pause(animatorID);
            ImGui::SameLine();
            if (ImGui::Button("Stop"))  rigSystem->Stop(animatorID);
            ImGui::SameLine();
            ImGui::TextColored(
                isPlaying ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                isPlaying ? " PLAYING" : " STOPPED");

            float speed = rigSystem->GetSpeed(animatorID);
            if (ImGui::SliderFloat("Speed", &speed, 0.0f, 3.0f))
                rigSystem->SetSpeed(animatorID, speed);

            float currentTime = rigSystem->GetPlayBackTime(animatorID);
            float duration    = rigSystem->GetDuration(animatorID);
            ImGui::Text("Time: %.2f / %.2f", currentTime, duration);
            if (duration > 0.0f)
                ImGui::ProgressBar(currentTime / duration);
        }

        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::End();
}

// =============================================================================
//  Lighting panel
// =============================================================================

void GameLayer::DrawLightingPanel()
{
    if (!ImGui::Begin("Lighting")) { ImGui::End(); return; }

    if (ImGui::CollapsingHeader("Spotlight", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& lightComp  = m_Scene.GetComponent<Aether::LightComponent>(m_LightEntity);
        auto& lightTrans = m_Scene.GetComponent<Aether::TransformComponent>(m_LightEntity);
        auto& light      = lightComp.Config;

        glm::vec3 dir = glm::normalize(glm::vec3(-lightTrans.WorldTransform[2]));
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);

        ImGui::ColorEdit3("Color",      glm::value_ptr(light.color));
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Range",     &light.range,     1.0f, 200.0f);

        float innerDeg = glm::degrees(glm::acos(light.innerCone));
        float outerDeg = glm::degrees(glm::acos(light.outerCone));
        if (ImGui::SliderFloat("Inner Cone", &innerDeg, 1.0f, 89.0f))
            light.innerCone = glm::cos(glm::radians(innerDeg));
        if (ImGui::SliderFloat("Outer Cone", &outerDeg, innerDeg, 90.0f))
            light.outerCone = glm::cos(glm::radians(outerDeg));

        ImGui::Checkbox("Cast Shadows", &light.castShadows);
    }

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& dirLightComp = m_Scene.GetComponent<Aether::LightComponent>(m_DirLightEntity);
        auto& dirLight     = dirLightComp.Config;

        ImGui::ColorEdit3("Color##dir",      glm::value_ptr(dirLight.color));
        ImGui::SliderFloat("Intensity##dir", &dirLight.intensity, 0.0f, 10.0f);

        // Euler angles are much friendlier to edit than a raw direction vector.
        // Pitch = rotation around X (tilts up/down), Yaw = rotation around Y (rotates around horizon).
        bool eulerChanged = false;
        eulerChanged |= ImGui::SliderFloat("Pitch##dir", &m_DirLightEuler.x, -90.0f,  90.0f, "%.1f deg");
        eulerChanged |= ImGui::SliderFloat("Yaw##dir",   &m_DirLightEuler.y, -180.0f, 180.0f, "%.1f deg");

        if (eulerChanged)
        {
            float pitchRad = glm::radians(m_DirLightEuler.x);
            float yawRad   = glm::radians(m_DirLightEuler.y);
            dirLight.direction = glm::normalize(glm::vec3(
                glm::sin(yawRad) * glm::cos(pitchRad),
               -glm::sin(pitchRad),
                glm::cos(yawRad) * glm::cos(pitchRad)));

            // The scene derives light direction from the entity's transform, not
            // Config.direction — so we must keep the rotation in sync too.
            auto& dirLightTrans = m_Scene.GetComponent<Aether::TransformComponent>(m_DirLightEntity);
            dirLightTrans.Rotation = glm::quat(glm::vec3(pitchRad, yawRad, 0.0f));
            dirLightTrans.Dirty    = true;
        }

        ImGui::Text("Direction: (%.2f, %.2f, %.2f)",
            dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);

        ImGui::Checkbox("Cast Shadows##dir", &dirLight.castShadows);

        if (ImGui::Button("Reset##dir"))
        {
            m_DirLightEuler    = glm::vec3(-45.0f, 0.0f, 0.0f);
            dirLight.color     = glm::vec3(1.0f, 0.95f, 0.85f);
            dirLight.intensity = 1.0f;
            float pitchRad     = glm::radians(m_DirLightEuler.x);
            float yawRad       = glm::radians(m_DirLightEuler.y);
            dirLight.direction = glm::normalize(glm::vec3(
                glm::sin(yawRad) * glm::cos(pitchRad),
               -glm::sin(pitchRad),
                glm::cos(yawRad) * glm::cos(pitchRad)));

            auto& dirLightTrans = m_Scene.GetComponent<Aether::TransformComponent>(m_DirLightEntity);
            dirLightTrans.Rotation = glm::quat(glm::vec3(pitchRad, yawRad, 0.0f));
            dirLightTrans.Dirty    = true;
        }
    }

    if (ImGui::CollapsingHeader("Volumetric Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Density",   &m_VolDensity,   0.0f, 0.2f);
        ImGui::SliderFloat("Intensity", &m_VolIntensity, 0.0f, 5.0f);
        ImGui::SliderInt  ("Steps",     &m_VolSteps,     8,    128);
    }

    if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Bias", &m_ShadowBias, 0.00001f, 0.005f, "%.5f");
    }

    ImGui::End();
}