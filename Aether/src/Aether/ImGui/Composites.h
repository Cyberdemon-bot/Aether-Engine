#pragma once
#include "Wrapper.h"
#include <vector>
#include <string>
#include <functional>

namespace Aether::UI
{
    // =========================================================================
    //  Row  —  "Label | Widget" on one line at a fixed column
    //
    //  Returns whatever the widget returns (bool for changed, etc.)
    //  Usage:
    //      UI::Row("Speed", 120.f, [&]{ return UI::DragFloat("##v", speed); });
    // =========================================================================

    template<typename Fn>
    inline auto Row(const char* label, float labelWidth, Fn&& widget) -> decltype(widget())
    {
        ImGui::Text("%s", label);
        Column(labelWidth);
        return widget();
    }

    // =========================================================================
    //  SectionHeader  —  visual divider with centered label
    // =========================================================================

    inline void SectionHeader(const char* label)
    {
        Spacing();
        Separator();
        float w    = ImGui::GetContentRegionAvail().x;
        float tw   = ImGui::CalcTextSize(label).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (w - tw) * 0.5f);
        TextColored(Color::Gray(), "%s", label);
        Separator();
        Spacing();
    }

    // =========================================================================
    //  Badge  —  colored status pill (PLAYING, STOPPED, HIT, MISS...)
    //
    //  UI::Badge("PLAYING", UI::Color::Green());
    // =========================================================================

    inline void Badge(const char* label, ImVec4 color)
    {
        auto sc1 = StyleColor(ImGuiCol_Button,        ImVec4(color.x, color.y, color.z, 0.18f));
        auto sc2 = StyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.22f));
        auto sc3 = StyleColor(ImGuiCol_ButtonActive,  ImVec4(color.x, color.y, color.z, 0.30f));
        auto sc4 = StyleColor(ImGuiCol_Text,          color);
        auto sv  = StyleVar  (ImGuiStyleVar_FrameRounding, 12.f);
        ImGui::SmallButton(label);
    }

    // =========================================================================
    //  ComboList  —  full combo + selectable loop collapsed to one call
    //
    //  Mutates selectedIndex.
    //  Returns true if selection changed.
    //
    //  UI::ComboList("Mesh", meshNames, m_MeshIndex);
    // =========================================================================

    inline bool ComboList(const char* label,
                          const std::vector<std::string>& items,
                          int& selectedIndex)
    {
        const char* preview = (selectedIndex >= 0 && selectedIndex < (int)items.size())
            ? items[selectedIndex].c_str()
            : "---";

        bool changed = false;
        if (auto c = Combo(label, preview))
        {
            for (int i = 0; i < (int)items.size(); i++)
            {
                auto  g   = ID(i);
                bool  sel = (i == selectedIndex);
                if (Selectable(items[i].c_str(), sel)) { selectedIndex = i; changed = true; }
                if (sel) ImGui::SetItemDefaultFocus();
            }
        }
        return changed;
    }

    // =========================================================================
    //  DragXYZ  —  colored X/Y/Z axis drag widgets (Unity/Unreal inspector style)
    //
    //  Returns true if any component changed.
    // =========================================================================

    inline bool DragXYZ(const char* label, glm::vec3& v, float speed=0.01f,
                        float labelWidth=90.f)
    {
        bool changed = false;
        auto guard   = ID(label);

        ImGui::Text("%s", label);
        ImGui::SameLine(labelWidth);

        float avail = ImGui::GetContentRegionAvail().x;
        float itemW = (avail - 6.f) / 3.f;

        struct Axis { const char* name; ImVec4 col; float& val; };
        Axis axes[3] = {
            {"X", Color::AxisX(), v.x},
            {"Y", Color::AxisY(), v.y},
            {"Z", Color::AxisZ(), v.z},
        };

        for (int i = 0; i < 3; i++)
        {
            if (i > 0) ImGui::SameLine(0, 2.f);

            // Colored axis label button
            {
                auto sc = StyleColor(ImGuiCol_Button,        axes[i].col);
                auto sv = StyleVar  (ImGuiStyleVar_FrameRounding, 2.f);
                ImGui::Button(axes[i].name, {16.f, 0.f});
            }

            ImGui::SameLine(0, 2.f);
            char iid[8]; snprintf(iid, sizeof(iid), "##%d", i);
            ImGui::SetNextItemWidth(itemW - 20.f);
            changed |= ImGui::DragFloat(iid, &axes[i].val, speed);
        }
        return changed;
    }

    // =========================================================================
    //  ConfirmButton  —  requires a second click to execute (destructive actions)
    //
    //  Manages its own armed state per label (uses static map).
    //  UI::ConfirmButton("Delete Entity", "Sure?", [&]{ destroyEntity(); });
    // =========================================================================

    template<typename Fn>
    inline void ConfirmButton(const char* label, const char* confirmLabel, Fn&& onConfirm)
    {
        static std::unordered_map<const char*, bool> s_Armed;
        bool& armed = s_Armed[label];

        if (!armed)
        {
            if (Button(label)) armed = true;
        }
        else
        {
            {
                auto sc = StyleColor(ImGuiCol_Button,        Color::Red(0.7f));
                auto sc2= StyleColor(ImGuiCol_ButtonHovered, Color::Red(0.9f));
                if (Button(confirmLabel)) { onConfirm(); armed = false; }
            }
            SameLine();
            if (SmallButton("✕")) armed = false;
        }
    }

    // =========================================================================
    //  Spinner  —  simple animated loading indicator
    //
    //  Call every frame while loading.
    //  UI::Spinner("Loading...", 8.f, 2.f, UI::Color::Cyan());
    // =========================================================================

    inline void Spinner(const char* label, float radius=8.f,
                        float thickness=2.f, ImVec4 color=Color::Cyan())
    {
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        ImVec2      pos = ImGui::GetCursorScreenPos();
        float       t   = (float)ImGui::GetTime();

        ImGui::Dummy({radius * 2.f, radius * 2.f});
        SameLine();
        Label(label);

        ImVec2 center = {pos.x + radius, pos.y + radius};
        const int segs = 12;
        for (int i = 0; i < segs; i++)
        {
            float  angle = t * 4.f + i * (2.f * 3.14159f / segs);
            float  alpha = (float)i / segs;
            ImU32  col   = IM_COL32(
                (uint8_t)(color.x*255), (uint8_t)(color.y*255),
                (uint8_t)(color.z*255), (uint8_t)(alpha*255));
            ImVec2 p1 = {center.x + cosf(angle) * (radius - thickness),
                         center.y + sinf(angle) * (radius - thickness)};
            ImVec2 p2 = {center.x + cosf(angle) * radius,
                         center.y + sinf(angle) * radius};
            dl->AddLine(p1, p2, col, thickness);
        }
    }

    // =========================================================================
    //  InputFloat3Labeled  —  three labeled float inputs with a prefix
    //
    //  For when DragXYZ's axis colors aren't wanted (e.g. min/max extents).
    //  Returns true if any changed.
    // =========================================================================

    inline bool InputFloat3Labeled(const char* label, glm::vec3& v,
                                   const char* xLabel="X",
                                   const char* yLabel="Y",
                                   const char* zLabel="Z",
                                   float speed=0.1f)
    {
        bool changed = false;
        auto g = ID(label);
        ImGui::Text("%s", label);
        ImGui::SameLine(90.f);
        float avail = ImGui::GetContentRegionAvail().x;
        float w     = (avail - 4.f) / 3.f;
        float vals[3] = {v.x, v.y, v.z};
        const char* labels[3] = {xLabel, yLabel, zLabel};
        const ImVec4 cols[3]  = {Color::AxisX(), Color::AxisY(), Color::AxisZ()};

        for (int i = 0; i < 3; i++)
        {
            if (i > 0) ImGui::SameLine(0, 2.f);
            {
                auto sc = StyleColor(ImGuiCol_Text, cols[i]);
                ImGui::Text("%s", labels[i]);
            }
            ImGui::SameLine(0, 2.f);
            char iid[8]; snprintf(iid, sizeof(iid), "##%d", i);
            ImGui::SetNextItemWidth(w - ImGui::CalcTextSize(labels[i]).x - 4.f);
            if (ImGui::DragFloat(iid, &vals[i], speed)) changed = true;
        }
        if (changed) v = {vals[0], vals[1], vals[2]};
        return changed;
    }

    // =========================================================================
    //  ScrollingLog  —  auto-scrolling text log (for console output etc.)
    //
    //  lines   : vector of strings to display
    //  height  : child window height (0 = auto)
    //  UI::ScrollingLog("##log", logLines, 200.f);
    // =========================================================================

    inline void ScrollingLog(const char* id,
                              const std::vector<std::string>& lines,
                              float height=0.f,
                              ImVec4 textColor=Color::White())
    {
        if (auto c = Child(id, {0.f, height}, true))
        {
            for (auto& line : lines)
                TextColored(textColor, "%s", line.c_str());
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.f);
        }
    }

} // namespace UI