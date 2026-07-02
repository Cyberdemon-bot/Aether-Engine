#pragma once
#include "Composites.h"
#include "Aether/Scene/Component.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Assets/AssetsRegister.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"
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

    // Full TransformComponent overload — sets Dirty automaticall

    // =========================================================================
    //  EntityNode  —  single node in a scene hierarchy tree
    //
    //  Decoupled via callbacks — no direct scene dependency.
    // =========================================================================

    struct EntityNodeDesc
    {
        const char*           label       = "";
        bool                  selected    = false;
        bool                  hasChildren = false;
        const void*           id          = nullptr;

        std::function<void()> onSelect    = nullptr;
        std::function<void()> onContext   = nullptr;
        std::function<void()> children    = nullptr;
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

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && desc.onSelect)
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
    inline void _DrawHierarchyNode(Scene& scene, Entity entity, Entity& selected);

    inline void SceneHierarchy(const char* windowTitle,
                                Scene& scene,
                                Entity& selected)
    {
        if (auto w = Window(windowTitle))
        {
            // BUG FIX: was IsMouseDown — fired every frame the button was held,
            // wiping selection on the same frame as a click and also while
            // dragging the camera across the window.
            // Fix: IsMouseClicked (single-frame), only when no item is hovered
            // (so clicking a node doesn't also trigger the deselect).
            if (ImGui::IsMouseClicked(0) &&
                ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                !ImGui::IsAnyItemHovered())
            {
                selected = Null_Entity;
            }

            auto view = scene.View<HierarchyComponent>();
            for (auto entity : view)
            {
                if (scene.GetComponent<HierarchyComponent>(entity).parent == Null_Entity)
                    _DrawHierarchyNode(scene, entity, selected);
            }
        }
    }

    inline void _DrawHierarchyNode(Scene& scene, Entity entity, Entity& selected)
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
                Entity next = scene.GetComponent<HierarchyComponent>(child).nextSibling;
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
    //  UI::TransformInspector(scene, selected);
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

        if(TRS(t.Translation, t.Rotation, t.Scale, 0.01f)) t.Dirty = true;

        Spacing();
        if (Button("Reset Transform"))
        {
            t.Translation = glm::vec3(0.f);
            t.Rotation    = glm::quat(1.f, 0.f, 0.f, 0.f);
            t.Scale       = glm::vec3(1.f);
            t.Dirty = true;
        }
    }

    // =========================================================================
    //  AnimatorControls  —  clip picker + play/pause/stop + scrubber
    //
    //  UI::AnimatorControls(anim, rigSystem);
    // =========================================================================

    inline void AnimatorControls(AnimatorComponent& anim, RigModule* rig)
    {
        if (!rig) { TextDisabled("RigSystem not available."); return; }
        if (!anim.Skeleton.IsValid()) { TextDisabled("No skeleton bound."); return; }

        auto guard = ID("AnimatorControls");

        // --- Clip combo ---
        {
            std::vector<std::string> clipNames;
            clipNames.reserve(anim.Clips.size());
            for (int i = 0; i < (int)anim.Clips.size(); i++)
                clipNames.push_back("Clip " + std::to_string(i));

            if (ComboList("Clip", clipNames, anim.ActiveClipIdx))
            {
                anim.CurrentTime = 0.0f;
                anim.CacheDirty = true;
            }
        }

        Separator();

        // --- Transport ---
        {
            auto d = Disabled(!anim.IsPlaying ? false : false); // play always available to re-trigger
            bool canPlay = !anim.IsPlaying;
            {
                auto dd = Disabled(!canPlay);
                if (Button("Play"))  anim.IsPlaying = true;
            }
        }
        SameLine();
        {
            auto d = Disabled(!anim.IsPlaying);
            if (Button("Pause")) anim.IsPlaying = false;
        }
        SameLine();
        if (Button("Stop")) { anim.IsPlaying = false; anim.CurrentTime = 0.0f; }
        SameLine();
        Badge(anim.IsPlaying ? "PLAYING" : "STOPPED",
            anim.IsPlaying ? Color::Green() : Color::Red());

        // --- Loop toggle ---
        Checkbox("Loop", anim.Loop);

        // --- Speed ---
        SliderFloat("Speed", anim.Speed, 0.0f, 3.0f);

        // --- Scrubber ---
        if (anim.ActiveClipIdx >= 0 && anim.ActiveClipIdx < (int)anim.Clips.size())
        {
            auto* clipAsset = ServiceManager::GetService<AssetManager>()->GetAsset<Clip>(anim.Clips[anim.ActiveClipIdx]);
            if (clipAsset)
            {
                float duration = rig->GetDuration(clipAsset->GetHandle());
                if (duration > 0.0f)
                {
                    Text("Time: %.2f / %.2f", anim.CurrentTime, duration);
                    float t = anim.CurrentTime;
                    if (SliderFloat("##scrub", t, 0.0f, duration, "%.2f"))
                    {
                        anim.CurrentTime = t;
                        anim.IsPlaying   = false;
                    }
                    ProgressBar(anim.CurrentTime / duration);
                }
            }
        }
    }

    // =========================================================================
    //  LightInspector  —  edits a LightParam in-place
    
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

        ColorEdit3("Color",      light.color);
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
    // =========================================================================

    struct PhysBodyEntry
    {
        std::string      label;
        bool&            enabled;
        bool             isDynamic;
        Handle<RigidBody>  bodyHandle;
    };

    // =========================================================================
    //  PerformanceOverlay  —  FPS + frame time pinned to a screen corner
    //
    //  corner: 0=TL  1=TR  2=BL  3=BR
    // =========================================================================

    inline void PerformanceOverlay(int corner = 1, float xpad = 10.0f, float ypad = 10.0f)
    {
        auto*  vp  = ImGui::GetMainViewport();
        float  io  = ImGui::GetIO().Framerate;
        ImVec2 pos;

        switch (corner)
        {
            case 0:  pos = {vp->Pos.x + xpad,               vp->Pos.y + ypad             }; break;
            case 1:  pos = {vp->Pos.x + vp->Size.x - xpad,  vp->Pos.y + ypad             }; break;
            case 2:  pos = {vp->Pos.x + xpad,               vp->Pos.y + vp->Size.y - ypad}; break;
            default: pos = {vp->Pos.x + vp->Size.x - xpad,  vp->Pos.y + vp->Size.y - ypad}; break;
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

        c.RectFill({pos.x,          pos.y},
                   {pos.x + size.x, pos.y + size.y},
                   Col32(30, 30, 30, 180), rounding);

        if (frac > 0.f)
        {
            ImU32 fill = Col32(
                (uint8_t)((1.f - frac) * 255),
                (uint8_t)( frac        * 220),
                0, 220);
            c.RectFill({pos.x,                 pos.y},
                       {pos.x + size.x * frac, pos.y + size.y},
                       fill, rounding);
        }

        c.Rect({pos.x, pos.y}, {pos.x + size.x, pos.y + size.y},
               Col32(180, 180, 180, 200), rounding, 1.5f);

        c.Text({pos.x, pos.y - 22.f},
               Col32(220, 220, 220, 230), label, 18.f);
    }

} // namespace UI