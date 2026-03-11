#pragma once
#include "Canvas.h"
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdarg>
#include <cstdio>
#include <functional>

namespace Aether::UI
{
    // =========================================================================
    //  Color palette
    // =========================================================================

    namespace Color
    {
        inline ImVec4 Red    (float a=1.f) { return {0.90f, 0.20f, 0.20f, a}; }
        inline ImVec4 Green  (float a=1.f) { return {0.20f, 0.90f, 0.35f, a}; }
        inline ImVec4 Blue   (float a=1.f) { return {0.25f, 0.55f, 1.00f, a}; }
        inline ImVec4 Yellow (float a=1.f) { return {1.00f, 0.90f, 0.20f, a}; }
        inline ImVec4 Orange (float a=1.f) { return {1.00f, 0.60f, 0.10f, a}; }
        inline ImVec4 Cyan   (float a=1.f) { return {0.20f, 0.90f, 1.00f, a}; }
        inline ImVec4 Purple (float a=1.f) { return {0.70f, 0.30f, 1.00f, a}; }
        inline ImVec4 White  (float a=1.f) { return {1.00f, 1.00f, 1.00f, a}; }
        inline ImVec4 Gray   (float a=1.f) { return {0.55f, 0.55f, 0.55f, a}; }
        inline ImVec4 Black  (float a=1.f) { return {0.00f, 0.00f, 0.00f, a}; }

        // Axis colors (XYZ = RGB convention)
        inline ImVec4 AxisX  (float a=1.f) { return {0.85f, 0.20f, 0.20f, a}; }
        inline ImVec4 AxisY  (float a=1.f) { return {0.20f, 0.75f, 0.20f, a}; }
        inline ImVec4 AxisZ  (float a=1.f) { return {0.20f, 0.40f, 0.90f, a}; }
    }

    // ImVec4 → Col32 shorthand
    inline ImU32 ToU32(ImVec4 c) { return ImGui::ColorConvertFloat4ToU32(c); }

    // =========================================================================
    //  RAII Guard base
    // =========================================================================

    struct Guard
    {
        bool open = false;
        explicit operator bool() const { return open; }
    };

    // =========================================================================
    //  Window guards
    // =========================================================================

    struct WindowGuard : Guard
    {
        WindowGuard(const char* title, bool* p_open=nullptr, ImGuiWindowFlags flags=0)
        { open = ImGui::Begin(title, p_open, flags); }
        ~WindowGuard() { ImGui::End(); }
    };

    // Pinned, no-decoration, no-input overlay window
    struct OverlayGuard : Guard
    {
        OverlayGuard(const char* id, ImVec2 pos, float bgAlpha=0.f, ImGuiWindowFlags extra=0)
        {
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowBgAlpha(bgAlpha);
            open = ImGui::Begin(id, nullptr,
                ImGuiWindowFlags_NoDecoration     |
                ImGuiWindowFlags_NoInputs         |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove           |
                ImGuiWindowFlags_NoSavedSettings  | extra);
        }
        ~OverlayGuard() { ImGui::End(); }
    };

    struct ChildGuard : Guard
    {
        ChildGuard(const char* id, ImVec2 size={0,0}, bool border=false, ImGuiWindowFlags flags=0)
        { open = ImGui::BeginChild(id, size, border, flags); }
        ~ChildGuard() { ImGui::EndChild(); }
    };

    inline WindowGuard  Window (const char* title, bool* p_open=nullptr, ImGuiWindowFlags flags=0)              { return {title, p_open, flags}; }
    inline OverlayGuard Overlay(const char* id, ImVec2 pos, float alpha=0.f, ImGuiWindowFlags extra=0)          { return {id, pos, alpha, extra}; }
    inline ChildGuard   Child  (const char* id, ImVec2 size={0,0}, bool border=false, ImGuiWindowFlags flags=0) { return {id, size, border, flags}; }

    // =========================================================================
    //  Tree / Collapse / Tabs / Combo / Popup
    // =========================================================================

    struct TreeGuard : Guard
    {
        TreeGuard(const char* label, ImGuiTreeNodeFlags flags=ImGuiTreeNodeFlags_DefaultOpen)
        { open = ImGui::TreeNodeEx(label, flags); }
        ~TreeGuard() { if (open) ImGui::TreePop(); }
    };

    // CollapsingHeader — no TreePop needed
    struct HeaderGuard : Guard
    {
        HeaderGuard(const char* label, ImGuiTreeNodeFlags flags=0)
        { open = ImGui::CollapsingHeader(label, flags); }
    };

    struct TabBarGuard : Guard
    {
        TabBarGuard(const char* id, ImGuiTabBarFlags flags=0)
        { open = ImGui::BeginTabBar(id, flags); }
        ~TabBarGuard() { if (open) ImGui::EndTabBar(); }
    };

    struct TabGuard : Guard
    {
        TabGuard(const char* label, bool* p_open=nullptr, ImGuiTabItemFlags flags=0)
        { open = ImGui::BeginTabItem(label, p_open, flags); }
        ~TabGuard() { if (open) ImGui::EndTabItem(); }
    };

    struct ComboGuard : Guard
    {
        ComboGuard(const char* label, const char* preview, ImGuiComboFlags flags=0)
        { open = ImGui::BeginCombo(label, preview, flags); }
        ~ComboGuard() { if (open) ImGui::EndCombo(); }
    };

    struct PopupGuard : Guard
    {
        PopupGuard(const char* id, ImGuiWindowFlags flags=0)
        { open = ImGui::BeginPopup(id, flags); }
        ~PopupGuard() { if (open) ImGui::EndPopup(); }
    };

    struct PopupContextGuard : Guard
    {
        PopupContextGuard(const char* id=nullptr, ImGuiPopupFlags flags=ImGuiPopupFlags_MouseButtonRight)
        { open = ImGui::BeginPopupContextItem(id, flags); }
        ~PopupContextGuard() { if (open) ImGui::EndPopup(); }
    };

    inline TreeGuard         TreeNode   (const char* label, ImGuiTreeNodeFlags flags=ImGuiTreeNodeFlags_DefaultOpen) { return {label, flags}; }
    inline HeaderGuard       Header     (const char* label, ImGuiTreeNodeFlags flags=0)                              { return {label, flags}; }
    inline TabBarGuard       TabBar     (const char* id,    ImGuiTabBarFlags   flags=0)                              { return {id,    flags}; }
    inline TabGuard          Tab        (const char* label, bool* p_open=nullptr, ImGuiTabItemFlags flags=0)         { return {label, p_open, flags}; }
    inline ComboGuard        Combo      (const char* label, const char* preview, ImGuiComboFlags flags=0)            { return {label, preview, flags}; }
    inline PopupGuard        Popup      (const char* id,    ImGuiWindowFlags flags=0)                                { return {id,    flags}; }
    inline PopupContextGuard ContextMenu(const char* id=nullptr, ImGuiPopupFlags flags=ImGuiPopupFlags_MouseButtonRight) { return {id, flags}; }

    // =========================================================================
    //  Scoped push/pop
    // =========================================================================

    struct StyleColorGuard
    {
        int count;
        StyleColorGuard(ImGuiCol idx, ImVec4 col, int n=1) : count(n) { ImGui::PushStyleColor(idx, col); }
        ~StyleColorGuard() { ImGui::PopStyleColor(count); }
    };

    struct StyleVarGuard
    {
        StyleVarGuard(ImGuiStyleVar var, float  val) { ImGui::PushStyleVar(var, val); }
        StyleVarGuard(ImGuiStyleVar var, ImVec2 val) { ImGui::PushStyleVar(var, val); }
        ~StyleVarGuard() { ImGui::PopStyleVar(); }
    };

    struct FontScaleGuard
    {
        FontScaleGuard(float scale) { ImGui::SetWindowFontScale(scale); }
        ~FontScaleGuard()           { ImGui::SetWindowFontScale(1.0f); }
    };

    struct IDGuard
    {
        IDGuard(const char* id)  { ImGui::PushID(id); }
        IDGuard(int id)          { ImGui::PushID(id); }
        IDGuard(uint64_t id)     { ImGui::PushID((int)id); }
        IDGuard(const void* id)  { ImGui::PushID(id); }
        ~IDGuard()               { ImGui::PopID(); }
    };

    struct IndentGuard
    {
        float amount;
        IndentGuard(float w=0.f) : amount(w) { ImGui::Indent(w); }
        ~IndentGuard()                        { ImGui::Unindent(amount); }
    };

    // Disables all widgets inside the scope when condition is true
    struct DisabledGuard
    {
        bool active;
        DisabledGuard(bool disabled) : active(disabled) { if (active) ImGui::BeginDisabled(); }
        ~DisabledGuard()                                 { if (active) ImGui::EndDisabled(); }
    };

    inline StyleColorGuard StyleColor(ImGuiCol idx, ImVec4 col) { return {idx, col}; }
    inline StyleVarGuard   StyleVar  (ImGuiStyleVar v, float f)  { return {v, f}; }
    inline StyleVarGuard   StyleVar  (ImGuiStyleVar v, ImVec2 f) { return {v, f}; }
    inline FontScaleGuard  FontScale (float scale)               { return {scale}; }
    inline IDGuard         ID        (const char* id)            { return {id}; }
    inline IDGuard         ID        (int id)                    { return {id}; }
    inline IDGuard         ID        (uint64_t id)               { return {id}; }
    inline IDGuard         ID        (const void* id)            { return {id}; }
    inline IndentGuard     Indent    (float w=0.f)               { return {w}; }
    inline DisabledGuard   Disabled  (bool condition)            { return {condition}; }

    // Functional form: UI::Disabled(cond, [&]{ Button("Ok"); });
    template<typename Fn>
    inline void Disabled(bool condition, Fn&& fn)
    {
        auto g = DisabledGuard(condition);
        fn();
    }

    // =========================================================================
    //  Layout helpers
    // =========================================================================

    inline void Separator()                  { ImGui::Separator(); }
    inline void Spacing()                    { ImGui::Spacing(); }
    inline void NewLine()                    { ImGui::NewLine(); }
    inline void SameLine(float offset=0.f)   { ImGui::SameLine(offset); }
    inline void FullWidth()                  { ImGui::SetNextItemWidth(-1.f); }

    // Jump to a fixed x offset and fill remaining width — for label|widget layout
    inline void Column(float x)
    {
        ImGui::SameLine(x);
        ImGui::SetNextItemWidth(-1.f);
    }

    // =========================================================================
    //  Text
    // =========================================================================

    inline void Text        (const char* fmt, ...) { va_list a; va_start(a, fmt); ImGui::TextV        (fmt, a); va_end(a); }
    inline void TextColored (ImVec4 col, const char* fmt, ...) { va_list a; va_start(a, fmt); ImGui::TextColoredV (col, fmt, a); va_end(a); }
    inline void TextDisabled(const char* fmt, ...) { va_list a; va_start(a, fmt); ImGui::TextDisabledV(fmt, a); va_end(a); }
    inline void Label       (const char* text)     { ImGui::TextUnformatted(text); }

    inline void Tooltip(const char* fmt, ...)
    {
        if (ImGui::IsItemHovered()) {
            va_list a; va_start(a, fmt);
            ImGui::SetTooltipV(fmt, a);
            va_end(a);
        }
    }

    // =========================================================================
    //  Buttons
    // =========================================================================

    inline bool Button     (const char* label, ImVec2 size={0,0}) { return ImGui::Button(label, size); }
    inline bool SmallButton(const char* label)                    { return ImGui::SmallButton(label); }

    inline bool IconButton(const char* icon, const char* tooltip=nullptr)
    {
        bool clicked = ImGui::SmallButton(icon);
        if (tooltip) Tooltip("%s", tooltip);
        return clicked;
    }

    // =========================================================================
    //  Widgets — glm-native, no value_ptr at call sites
    // =========================================================================

    // Drag
    inline bool DragFloat (const char* l, float&     v, float spd=0.1f, float mn=0.f, float mx=0.f, const char* fmt="%.3f") { return ImGui::DragFloat (l, &v, spd, mn, mx, fmt); }
    inline bool DragInt   (const char* l, int&       v, float spd=1.f,  int   mn=0,   int   mx=0)                           { return ImGui::DragInt   (l, &v, spd, mn, mx); }
    inline bool DragFloat2(const char* l, glm::vec2& v, float spd=0.1f, float mn=0.f, float mx=0.f)                        { return ImGui::DragFloat2(l, glm::value_ptr(v), spd, mn, mx); }
    inline bool DragFloat3(const char* l, glm::vec3& v, float spd=0.1f, float mn=0.f, float mx=0.f)                        { return ImGui::DragFloat3(l, glm::value_ptr(v), spd, mn, mx); }
    inline bool DragFloat4(const char* l, glm::vec4& v, float spd=0.1f, float mn=0.f, float mx=0.f)                        { return ImGui::DragFloat4(l, glm::value_ptr(v), spd, mn, mx); }

    // Slider
    inline bool SliderFloat(const char* l, float& v, float mn, float mx, const char* fmt="%.3f") { return ImGui::SliderFloat(l, &v, mn, mx, fmt); }
    inline bool SliderInt  (const char* l, int&   v, int   mn, int   mx)                         { return ImGui::SliderInt  (l, &v, mn, mx); }

    // Misc
    inline bool Checkbox  (const char* l, bool&      v) { return ImGui::Checkbox  (l, &v); }
    inline bool InputText (const char* l, char* buf, size_t sz, ImGuiInputTextFlags f=0) { return ImGui::InputText(l, buf, sz, f); }
    inline bool ColorEdit3(const char* l, glm::vec3& v, ImGuiColorEditFlags f=0) { return ImGui::ColorEdit3(l, glm::value_ptr(v), f); }
    inline bool ColorEdit4(const char* l, glm::vec4& v, ImGuiColorEditFlags f=0) { return ImGui::ColorEdit4(l, glm::value_ptr(v), f); }

    // Progress bar with optional color tint
    inline void ProgressBar(float fraction, ImVec2 size={-1,0},
                             const char* overlay=nullptr,
                             ImVec4 tint={0.2f, 0.8f, 0.3f, 0.9f})
    {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, tint);
        ImGui::ProgressBar(fraction, size, overlay);
        ImGui::PopStyleColor();
    }

    // Selectable wrapper
    inline bool Selectable(const char* label, bool selected=false, ImGuiSelectableFlags flags=0)
    {
        return ImGui::Selectable(label, selected, flags);
    }

    // MenuItem wrapper
    inline bool MenuItem(const char* label, bool enabled=true)
    {
        return ImGui::MenuItem(label, nullptr, false, enabled);
    }

} // namespace UI