#pragma once
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
//  UI — general-purpose RAII ImGui wrapper
//
//  All primitives live under the UI:: namespace.
//  Guards are truthy when their block should be entered:
//
//      if (auto w = UI::Window("Inspector"))
//          if (auto t = UI::TreeNode("Transform"))
//              UI::TRS(pos, rot, scale);
// =============================================================================

namespace Aether::UI
{
    // =========================================================================
    //  Viewport helpers
    // =========================================================================

    inline ImGuiViewport* Viewport()    { return ImGui::GetMainViewport(); }
    inline ImVec2 ViewportPos()         { return Viewport()->Pos;  }
    inline ImVec2 ViewportSize()        { return Viewport()->Size; }
    inline ImDrawList* FgDraw()         { return ImGui::GetForegroundDrawList(); }
    inline ImDrawList* BgDraw()         { return ImGui::GetBackgroundDrawList(); }

    // Normalized screen position  (0,0) = top-left  (1,1) = bottom-right
    inline ImVec2 ScreenAnchor(float nx, float ny)
    {
        auto* vp = Viewport();
        return { vp->Pos.x + vp->Size.x * nx,
                 vp->Pos.y + vp->Size.y * ny };
    }
    inline ImVec2 ScreenCenter() { return ScreenAnchor(0.5f, 0.5f); }

    // =========================================================================
    //  Color helpers
    // =========================================================================

    inline ImVec4 RGBA(float r, float g, float b, float a = 1.f) { return {r, g, b, a}; }
    inline ImU32  Col32(float r, float g, float b, float a = 1.f)
    {
        return IM_COL32((ImU8)(r*255), (ImU8)(g*255), (ImU8)(b*255), (ImU8)(a*255));
    }

    namespace Color
    {
        inline ImVec4 Red    (float a=1.f) { return {1.f,  0.2f, 0.2f, a}; }
        inline ImVec4 Green  (float a=1.f) { return {0.2f, 1.f,  0.4f, a}; }
        inline ImVec4 Blue   (float a=1.f) { return {0.3f, 0.6f, 1.f,  a}; }
        inline ImVec4 Yellow (float a=1.f) { return {1.f,  1.f,  0.2f, a}; }
        inline ImVec4 Orange (float a=1.f) { return {1.f,  0.6f, 0.1f, a}; }
        inline ImVec4 Cyan   (float a=1.f) { return {0.3f, 1.f,  1.f,  a}; }
        inline ImVec4 White  (float a=1.f) { return {1.f,  1.f,  1.f,  a}; }
        inline ImVec4 Gray   (float a=1.f) { return {0.6f, 0.6f, 0.6f, a}; }
        inline ImVec4 Black  (float a=1.f) { return {0.f,  0.f,  0.f,  a}; }
    }

    // =========================================================================
    //  RAII Guard base
    // =========================================================================

    struct Guard
    {
        bool open = false;
        explicit operator bool() const { return open; }
    };

    // =========================================================================
    //  Windows
    // =========================================================================

    struct WindowGuard : Guard
    {
        WindowGuard(const char* title, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
        { open = ImGui::Begin(title, p_open, flags); }
        ~WindowGuard() { ImGui::End(); }
    };

    // Pinned, no-decoration, no-input overlay
    struct OverlayGuard : Guard
    {
        OverlayGuard(const char* id, ImVec2 pos, float bgAlpha = 0.f, ImGuiWindowFlags extra = 0)
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

    inline WindowGuard  Window (const char* title, bool* p_open=nullptr, ImGuiWindowFlags flags=0)             { return {title, p_open, flags}; }
    inline OverlayGuard Overlay(const char* id, ImVec2 pos, float alpha=0.f, ImGuiWindowFlags extra=0)         { return {id, pos, alpha, extra}; }
    inline ChildGuard   Child  (const char* id, ImVec2 size={0,0}, bool border=false, ImGuiWindowFlags flags=0) { return {id, size, border, flags}; }

    // =========================================================================
    //  Tree / Collapse / Tabs / Combo / Popup
    // =========================================================================

    struct TreeGuard : Guard
    {
        TreeGuard(const char* label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen)
        { open = ImGui::TreeNodeEx(label, flags); }
        ~TreeGuard() { if (open) ImGui::TreePop(); }
    };

    struct HeaderGuard : Guard
    {
        // CollapsingHeader needs no TreePop
        HeaderGuard(const char* label, ImGuiTreeNodeFlags flags = 0)
        { open = ImGui::CollapsingHeader(label, flags); }
    };

    struct TabBarGuard : Guard
    {
        TabBarGuard(const char* id, ImGuiTabBarFlags flags = 0)
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

    inline TreeGuard   TreeNode(const char* label, ImGuiTreeNodeFlags flags=ImGuiTreeNodeFlags_DefaultOpen) { return {label, flags}; }
    inline HeaderGuard Header  (const char* label, ImGuiTreeNodeFlags flags=0)                              { return {label, flags}; }
    inline TabBarGuard TabBar  (const char* id,    ImGuiTabBarFlags   flags=0)                              { return {id,    flags}; }
    inline TabGuard    Tab     (const char* label, bool* p_open=nullptr, ImGuiTabItemFlags flags=0)         { return {label, p_open, flags}; }
    inline ComboGuard  Combo   (const char* label, const char* preview,  ImGuiComboFlags   flags=0)         { return {label, preview, flags}; }
    inline PopupGuard  Popup   (const char* id,    ImGuiWindowFlags flags=0)                                { return {id,    flags}; }

    // =========================================================================
    //  Scoped push/pop
    // =========================================================================

    struct StyleColorGuard
    {
        StyleColorGuard(ImGuiCol idx, ImVec4 col) { ImGui::PushStyleColor(idx, col); }
        ~StyleColorGuard()                        { ImGui::PopStyleColor(); }
    };

    struct StyleVarGuard
    {
        StyleVarGuard(ImGuiStyleVar var, float  val) { ImGui::PushStyleVar(var, val); }
        StyleVarGuard(ImGuiStyleVar var, ImVec2 val) { ImGui::PushStyleVar(var, val); }
        ~StyleVarGuard()                             { ImGui::PopStyleVar(); }
    };

    struct FontScaleGuard
    {
        FontScaleGuard(float scale) { ImGui::SetWindowFontScale(scale); }
        ~FontScaleGuard()           { ImGui::SetWindowFontScale(1.0f); }
    };

    struct IDGuard
    {
        IDGuard(const char* id) { ImGui::PushID(id); }
        IDGuard(int id)         { ImGui::PushID(id); }
        ~IDGuard()              { ImGui::PopID(); }
    };

    inline StyleColorGuard StyleColor(ImGuiCol idx, ImVec4 col)  { return {idx, col}; }
    inline StyleVarGuard   StyleVar  (ImGuiStyleVar v, float f)  { return {v, f}; }
    inline StyleVarGuard   StyleVar  (ImGuiStyleVar v, ImVec2 f) { return {v, f}; }
    inline FontScaleGuard  FontScale (float scale)               { return {scale}; }
    inline IDGuard         ID        (const char* id)            { return {id}; }
    inline IDGuard         ID        (int id)                    { return {id}; }

    // =========================================================================
    //  Layout
    // =========================================================================

    inline void Separator()                  { ImGui::Separator(); }
    inline void Spacing()                    { ImGui::Spacing(); }
    inline void NewLine()                    { ImGui::NewLine(); }
    inline void SameLine(float offset = 0.f) { ImGui::SameLine(offset); }
    inline void FullWidth()                  { ImGui::SetNextItemWidth(-1.f); }

    // Jumps to a fixed column then fills the rest — for label|widget rows
    inline void Column(float x) { ImGui::SameLine(x); ImGui::SetNextItemWidth(-1.f); }

    // =========================================================================
    //  Text
    // =========================================================================

    inline void Text        (const char* fmt, ...) { va_list a; va_start(a,fmt); ImGui::TextV        (fmt,a); va_end(a); }
    inline void TextColored (ImVec4 col, const char* fmt, ...) { va_list a; va_start(a,fmt); ImGui::TextColoredV (col,fmt,a); va_end(a); }
    inline void TextDisabled(const char* fmt, ...) { va_list a; va_start(a,fmt); ImGui::TextDisabledV(fmt,a); va_end(a); }
    inline void Label       (const char* text)     { ImGui::TextUnformatted(text); }
    inline void Tooltip     (const char* text)     { if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text); }

    // =========================================================================
    //  Widgets  (glm-native overloads so you never write value_ptr manually)
    // =========================================================================

    // Buttons
    inline bool Button     (const char* label, ImVec2 size={0,0}) { return ImGui::Button(label, size); }
    inline bool SmallButton(const char* label)                    { return ImGui::SmallButton(label); }
    inline bool IconButton (const char* icon, const char* tooltip=nullptr)
    {
        bool clicked = ImGui::SmallButton(icon);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        return clicked;
    }

    // Drag
    inline bool DragFloat (const char* l, float&     v, float spd=0.1f, float mn=0.f, float mx=0.f) { return ImGui::DragFloat (l, &v, spd, mn, mx); }
    inline bool DragInt   (const char* l, int&       v, float spd=1.f,  int   mn=0,   int   mx=0)   { return ImGui::DragInt   (l, &v, spd, mn, mx); }
    inline bool DragFloat2(const char* l, glm::vec2& v, float spd=0.1f)                              { return ImGui::DragFloat2(l, glm::value_ptr(v), spd); }
    inline bool DragFloat3(const char* l, glm::vec3& v, float spd=0.1f)                              { return ImGui::DragFloat3(l, glm::value_ptr(v), spd); }
    inline bool DragFloat4(const char* l, glm::vec4& v, float spd=0.1f)                              { return ImGui::DragFloat4(l, glm::value_ptr(v), spd); }

    // Slider
    inline bool SliderFloat(const char* l, float& v, float mn, float mx) { return ImGui::SliderFloat(l, &v, mn, mx); }
    inline bool SliderInt  (const char* l, int&   v, int   mn, int   mx) { return ImGui::SliderInt  (l, &v, mn, mx); }

    // Misc
    inline bool Checkbox  (const char* l, bool&      v) { return ImGui::Checkbox  (l, &v); }
    inline bool ColorEdit3(const char* l, glm::vec3& v) { return ImGui::ColorEdit3(l, glm::value_ptr(v)); }
    inline bool ColorEdit4(const char* l, glm::vec4& v) { return ImGui::ColorEdit4(l, glm::value_ptr(v)); }
    inline bool InputText (const char* l, char* buf, size_t sz, ImGuiInputTextFlags f=0) { return ImGui::InputText(l, buf, sz, f); }

    // Progress bar with optional color tint
    inline void ProgressBar(float fraction, ImVec2 size={-1,0},
                             const char* overlay=nullptr,
                             ImVec4 tint={0.2f,0.8f,0.3f,0.9f})
    {
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, tint);
        ImGui::ProgressBar(fraction, size, overlay);
        ImGui::PopStyleColor();
    }

    // =========================================================================
    //  Common compound patterns
    // =========================================================================

    // label | widget on the same line at a fixed column
    //   UI::Row("Speed", 120.f, [&]{ return UI::DragFloat("##v", speed); });
    template<typename Fn>
    inline bool Row(const char* label, float labelWidth, Fn&& widget)
    {
        ImGui::Text("%s", label);
        Column(labelWidth);
        return widget();
    }

    // XYZ drag with colored axis buttons (like a Unity/Unreal inspector row)
    inline bool DragXYZ(const char* label, glm::vec3& v, float speed = 0.01f)
    {
        bool changed = false;
        auto guard = ID(label);

        ImGui::Text("%s", label);
        ImGui::SameLine(90.f);

        float itemW = (ImGui::GetContentRegionAvail().x - 6.f) / 3.f;

        struct Axis { const char* name; ImVec4 col; float& val; };
        Axis axes[3] = {
            {"X", {0.85f, 0.2f, 0.2f, 1.f}, v.x},
            {"Y", {0.2f,  0.75f,0.2f, 1.f}, v.y},
            {"Z", {0.2f,  0.4f, 0.9f, 1.f}, v.z},
        };

        for (int i = 0; i < 3; i++)
        {
            if (i > 0) ImGui::SameLine(0, 2.f);
            { auto sc = StyleColor(ImGuiCol_Button, axes[i].col); ImGui::Button(axes[i].name, {18.f, 0.f}); }
            ImGui::SameLine(0, 2.f);
            char iid[8]; snprintf(iid, sizeof(iid), "##%d", i);
            ImGui::SetNextItemWidth(itemW - 22.f);
            changed |= ImGui::DragFloat(iid, &axes[i].val, speed);
        }
        return changed;
    }

    // Full TRS block — returns true if anything changed
    inline bool TRS(glm::vec3& translation, glm::vec3& rotationEulerDeg, glm::vec3& scale)
    {
        bool changed = false;
        auto guard = ID("TRS");
        changed |= DragXYZ("Position", translation,      0.01f);
        changed |= DragXYZ("Rotation", rotationEulerDeg, 0.5f);
        changed |= DragXYZ("Scale",    scale,            0.01f);
        return changed;
    }

} // namespace UI