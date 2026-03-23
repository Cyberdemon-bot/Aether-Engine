#pragma once
#include "Composites.h"
#include "Aether/Scene/Component.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Assets/AssetsRegister.h"
#include <functional>
#include <string>
#include <vector>


namespace Aether::UI
{
    // =========================================================================
    //  TRS  —  Translation / Rotation / Scale inspector block
    //
    //  Accepts a TransformComponent directly or individual fields.
    //  Returns true if anything changed — caller sets Dirty.
    // =========================================================================

    inline bool TRS(glm::vec3& translation,
                    glm::quat&  rotation,
                    glm::vec3&  scale,
                    float       dragSpeed = 0.01f)
    {
        bool changed = false;
        auto guard   = ID("TRS");

        changed |= DragXYZ("Position", translation, dragSpeed);

        // Edit as euler degrees, convert back to quat
        glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
        if (DragXYZ("Rotation", euler, 0.5f))
        {
            rotation = glm::quat(glm::radians(euler));
            changed  = true;
        }

        changed |= DragXYZ("Scale", scale, dragSpeed, 90.f);
        return changed;
    }

    // Raw floats version (for when you don't have a quat)
    inline bool TRS(glm::vec3& translation,
                    glm::vec3& rotationEulerDeg,
                    glm::vec3& scale,
                    float      dragSpeed = 0.01f)
    {
        bool changed = false;
        auto guard   = ID("TRS");
        changed |= DragXYZ("Position", translation,      dragSpeed);
        changed |= DragXYZ("Rotation", rotationEulerDeg, 0.5f);
        changed |= DragXYZ("Scale",    scale,            dragSpeed);
        return changed;
    }

    // Full TransformComponent overload — sets Dirty automatically
    inline bool TRS(TransformComponent& t, float dragSpeed=0.01f)
    {
        bool changed = TRS(t.Translation, t.Rotation, t.Scale, dragSpeed);
        if (changed) t.Dirty = true;
        return changed;
    }

    // =========================================================================
    //  EntityNode  —  single node in a scene hierarchy tree
    //
    //  Decoupled via callbacks — no direct scene dependency.
    //
    //  struct EntityNodeDesc {
    //      label, selected, hasChildren,
    //      onSelect, onContext (lambda for menu items), children (lambda to recurse)
    //  };
    // =========================================================================

    struct EntityNodeDesc
    {
        const char*           label       = "";
        bool                  selected    = false;
        bool                  hasChildren = false;
        const void*           id          = nullptr; // unique ptr-sized id

        std::function<void()> onSelect    = nullptr;
        std::function<void()> onContext   = nullptr; // called inside context menu
        std::function<void()> children    = nullptr; // called if node is open
    };

    inline void EntityNode(const EntityNodeDesc& desc)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow   |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!desc.hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
        if (desc.selected)     flags |= ImGuiTreeNodeFlags_Selected;

        bool open = ImGui::TreeNodeEx(desc.id ? desc.id : (const void*)desc.label,
                                      flags, "%s", desc.label);

        if (ImGui::IsItemClicked() && desc.onSelect)
            desc.onSelect();

        if (desc.onContext)
        {
            if (auto ctx = ContextMenu())
                desc.onContext();
        }

        if (open)
        {
            if (desc.children) desc.children();
            ImGui::TreePop();
        }
    }

    // =========================================================================
    //  SceneHierarchy  —  full hierarchy panel backed by Aether::Scene
    //
    //  Handles root-only iteration, deselect on empty-click, context menus.
    //
    //  UI::SceneHierarchy("Hierarchy", m_Scene, m_SelectedEntity);
    // =========================================================================

    // Forward declaration for recursion
    inline void _DrawHierarchyNode(Scene& scene, Entity entity,
                                   Entity& selected);

    inline void SceneHierarchy(const char* windowTitle,
                                Scene& scene,
                                Entity& selected)
    {
        if (auto w = Window(windowTitle))
        {
            // Deselect on empty click
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
                selected = Null_Entity;

            auto view = scene.View<HierarchyComponent>();
            for (auto entity : view)
            {
                if (scene.GetComponent<HierarchyComponent>(entity).parent
                    == Null_Entity)
                {
                    _DrawHierarchyNode(scene, entity, selected);
                }
            }
        }
    }

    inline void _DrawHierarchyNode(Scene& scene, Entity entity,
                                   Entity& selected)
    {
        auto& tag  = scene.GetComponent<TagComponent>(entity);
        auto& hier = scene.GetComponent<HierarchyComponent>(entity);

        EntityNodeDesc desc;
        desc.label       = tag.Tag.c_str();
        desc.selected    = (entity == selected);
        desc.hasChildren = (hier.firstChild != Null_Entity);
        desc.id          = (const void*)(uint64_t)entity;

        desc.onSelect = [&]{ selected = entity; };

        desc.onContext = [&]
        {
            if (MenuItem("Select"))
                selected = entity;

            if (MenuItem("Unparent", hier.parent != Null_Entity))
                scene.BreakParent(entity);

            if (selected != Null_Entity &&
                selected != entity &&
                MenuItem("Make parent of selected"))
            {
                scene.MakeParent(selected, entity);
            }
        };

        desc.children = [&]
        {
            Entity child = hier.firstChild;
            while (child != Null_Entity)
            {
                Entity next =
                    scene.GetComponent<HierarchyComponent>(child).nextSibling;
                _DrawHierarchyNode(scene, child, selected);
                child = next;
            }
        };

        EntityNode(desc);
    }

    // =========================================================================
    //  TransformInspector  —  TRS + reset + physics sync
    //
    //  Shows entity name at top, edits TransformComponent,
    //  optionally syncs to a physics body.
    //
    //  UI::TransformInspector(scene, selected, &physBodyID);
    // =========================================================================

    inline void TransformInspector(Scene& scene, Entity selected)
    {
        if (selected == Null_Entity || !scene.IsValid(selected))
        {
            TextDisabled("Select an entity in the Hierarchy panel.");
            return;
        }

        auto& tag = scene.GetComponent<TagComponent>(selected);
        auto& t   = scene.GetComponent<TransformComponent>(selected);

        TextColored(Color::Green(), "%s", tag.Tag.c_str());
        Spacing();

        Spacing();
        if (Button("Reset Transform"))
        {
            t.Translation = glm::vec3(0.f);
            t.Rotation    = glm::quat(1.f, 0.f, 0.f, 0.f);
            t.Scale       = glm::vec3(1.f);
            t.Dirty       = true;
        }
    }

    // =========================================================================
    //  AnimatorControls  —  clip picker + play/pause/stop + scrubber
    //
    //  UI::AnimatorControls(animatorID, rigSystem);
    // =========================================================================

    inline void AnimatorControls(UUID animatorID,
                                 Ref<RigModule> rigSystem)
    {
        if (!rigSystem) { TextDisabled("RigSystem not available."); return; }

        auto guard = ID((uint64_t)animatorID);

        // Clip combo
        auto  clips      = rigSystem->GetClips(animatorID);
        int   currentIdx = rigSystem->GetCurrentClipIndex(animatorID);

        std::vector<std::string> clipNames;
        clipNames.reserve(clips.size());
        for (auto& c : clips)
            clipNames.push_back(AssetsRegister::Get(c));

        if (ComboList("Clip", clipNames, currentIdx))
            rigSystem->BindClip(animatorID, clips[currentIdx]);

        Separator();

        // Transport buttons
        bool isPlaying = rigSystem->IsPlaying(animatorID);

        if (Button("Play"))  rigSystem->Play(animatorID);
        SameLine();
        if (Button("Pause")) rigSystem->Pause(animatorID);
        SameLine();
        if (Button("Stop"))  rigSystem->Stop(animatorID);
        SameLine();

        Badge(isPlaying ? "PLAYING" : "STOPPED",
              isPlaying ? Color::Green() : Color::Red());

        // Speed
        float speed = rigSystem->GetSpeed(animatorID);
        if (SliderFloat("Speed", speed, 0.f, 3.f))
            rigSystem->SetSpeed(animatorID, speed);

        // Scrubber
        float currentTime = rigSystem->GetPlayBackTime(animatorID);
        float duration    = rigSystem->GetDuration(animatorID);
        Text("Time: %.2f / %.2f", currentTime, duration);
        if (duration > 0.f)
            ProgressBar(currentTime / duration);

        // Loop toggle
        bool looping = rigSystem->GetLoop(animatorID);
        if (Checkbox("Loop", looping))
            rigSystem->SetLoop(animatorID, looping);
    }

    // =========================================================================
    //  LightInspector  —  edits a LightParam in-place
    //
    //  UI::LightInspector(light.Config);
    // =========================================================================

    inline void LightInspector(LightParam& light,
                                const glm::mat4* worldTransform = nullptr)
    {
        if (worldTransform)
        {
            glm::vec3 dir = glm::normalize(glm::vec3(-(*worldTransform)[2]));
            TextDisabled("Direction: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
        }

        ColorEdit3("Color",     light.color);
        SliderFloat("Intensity", light.intensity, 0.f, 10.f);
        SliderFloat("Range",     light.range,     1.f, 200.f);

        if (light.type == LightType::Spot)
        {
            float innerDeg = glm::degrees(glm::acos(light.innerCone));
            float outerDeg = glm::degrees(glm::acos(light.outerCone));
            if (SliderFloat("Inner Cone", innerDeg, 1.f, 89.f))
                light.innerCone = glm::cos(glm::radians(innerDeg));
            if (SliderFloat("Outer Cone", outerDeg, innerDeg, 90.f))
                light.outerCone = glm::cos(glm::radians(outerDeg));
        }

        Checkbox("Cast Shadows", light.castShadows);
    }

    // =========================================================================
    //  PhysicsBodyEntry  —  one row in a physics body list
    //
    //  Checkbox to enable/disable + optional force/velocity inputs.
    // =========================================================================

    struct PhysBodyEntry
    {
        std::string  label;
        bool&        enabled;
        bool         isDynamic;
        BodyHandle   bodyHandle;
    };

    inline void PhysicsBodyList(std::vector<PhysBodyEntry>& entries)
    {
        SectionHeader("Active Bodies");
        for (auto& e : entries)
        {
            auto guard = ID(e.label.c_str());
            if (Checkbox(e.label.c_str(), e.enabled))
                PhysicsSystem::SetActive(e.bodyHandle, e.enabled);
        }
    }

    // =========================================================================
    //  PerformanceOverlay  —  FPS + frame time pinned to a screen corner
    //
    //  corner: 0=TL  1=TR  2=BL  3=BR
    //  UI::PerformanceOverlay(1);
    // =========================================================================

    inline void PerformanceOverlay(int corner = 1, float xpad = 10.0f, float ypad = 10.0f)
    {
        auto*  vp   = ImGui::GetMainViewport();
        float  io   = ImGui::GetIO().Framerate;
        ImVec2 pos;

        switch (corner)
        {
            case 0: pos = {vp->Pos.x + xpad,              vp->Pos.y + ypad             }; break;
            case 1: pos = {vp->Pos.x + vp->Size.x - xpad, vp->Pos.y + ypad             }; break;
            case 2: pos = {vp->Pos.x + xpad,              vp->Pos.y + vp->Size.y - ypad}; break;
            default:pos = {vp->Pos.x + vp->Size.x - xpad, vp->Pos.y + vp->Size.y - ypad}; break;
        }

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        if (corner == 1 || corner == 3) flags |= ImGuiWindowFlags_NoDecoration;

        if (auto w = Overlay("##perf", pos, 0.55f, flags))
        {
            Text("FPS        %.1f",    io);
            Text("Frame time %.2f ms", 1000.f / io);
        }
    }

    // =========================================================================
    //  HealthBar  —  drawn directly on foreground draw list (no window needed)
    //
    //  UI::HealthBar(hp, maxHp, {barX, barY}, {200, 18});
    // =========================================================================

    inline void HealthBar(float hp, float maxHp,
                          glm::vec2 pos  = {},
                          glm::vec2 size = {200.f, 18.f},
                          const char* label = "HP",
                          float rounding = 4.f)
    {
        if (pos.x == 0 && pos.y == 0)
        {
            auto vp = Screen::Pos();
            pos = {vp.x + 30.f, vp.y + 40.f};
        }

        float frac = glm::clamp(hp / maxHp, 0.f, 1.f);
        auto  c    = Foreground();

        c.RectFill({pos.x,                   pos.y},
                   {pos.x + size.x,          pos.y + size.y},
                   Col32(30, 30, 30, 180), rounding);

        if (frac > 0.f)
        {
            ImU32 fill = Col32(
                (uint8_t)((1.f - frac) * 255),
                (uint8_t)( frac        * 220),
                0, 220);
            c.RectFill({pos.x,                   pos.y},
                       {pos.x + size.x * frac,   pos.y + size.y},
                       fill, rounding);
        }

        c.Rect({pos.x, pos.y}, {pos.x + size.x, pos.y + size.y},
               Col32(180, 180, 180, 200), rounding, 1.5f);

        c.Text({pos.x, pos.y - 22.f},
               Col32(220, 220, 220, 230), label, 18.f);
    }

} // namespace UI