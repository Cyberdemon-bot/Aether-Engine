#pragma once
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace Aether::UI
{
    // -------------------------------------------------------------------------
    //  Color constructor helpers (avoids IM_COL32 at call sites)
    // -------------------------------------------------------------------------

    inline ImU32 Col32(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return IM_COL32(r, g, b, a);
    }

    inline ImU32 Col32f(float r, float g, float b, float a = 1.f)
    {
        return IM_COL32((uint8_t)(r*255), (uint8_t)(g*255),
                        (uint8_t)(b*255), (uint8_t)(a*255));
    }

    inline ImU32 Col32(ImVec4 c)
    {
        return ImGui::ColorConvertFloat4ToU32(c);
    }

    // -------------------------------------------------------------------------
    //  Screen — viewport / projection helpers
    // -------------------------------------------------------------------------

    namespace Screen
    {
        inline glm::vec2 Pos()    { auto* vp = ImGui::GetMainViewport(); return {vp->Pos.x,  vp->Pos.y};  }
        inline glm::vec2 Size()   { auto* vp = ImGui::GetMainViewport(); return {vp->Size.x, vp->Size.y}; }
        inline glm::vec2 Center() { return Pos() + Size() * 0.5f; }

        // Normalised anchor: (0,0)=top-left  (1,1)=bottom-right
        inline glm::vec2 Anchor(float nx, float ny)
        {
            return Pos() + Size() * glm::vec2(nx, ny);
        }

        // Projects a world-space position to screen pixels.
        // Returns false when the point is behind the camera or outside NDC.
        inline bool Project(const glm::vec3& worldPos,
                            const glm::mat4& viewProj,
                            glm::vec2&       outScreen)
        {
            glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.f);
            if (clip.w <= 0.f) return false;

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.x < -1.f || ndc.x > 1.f ||
                ndc.y < -1.f || ndc.y > 1.f) return false;

            glm::vec2 sz = Size();
            glm::vec2 vp = Pos();
            outScreen.x = vp.x + (ndc.x * 0.5f + 0.5f) * sz.x;
            outScreen.y = vp.y + (1.f - (ndc.y * 0.5f + 0.5f)) * sz.y;
            return true;
        }
    }

    // -------------------------------------------------------------------------
    //  Canvas — thin, glm-native wrapper around ImDrawList
    // -------------------------------------------------------------------------

    class Canvas
    {
    public:
        explicit Canvas(ImDrawList* dl) : m_DL(dl) {}

        // -- Lines ------------------------------------------------------------

        void Line(glm::vec2 p1, glm::vec2 p2, ImU32 color, float thickness = 1.f)
        {
            m_DL->AddLine(iv(p1), iv(p2), color, thickness);
        }

        void DashedLine(glm::vec2 p1, glm::vec2 p2, ImU32 color,
                        float dashLen = 6.f, float gapLen = 4.f, float thickness = 1.f)
        {
            glm::vec2 dir = p2 - p1;
            float     len = glm::length(dir);
            if (len < 1e-5f) return;
            dir /= len;

            float t = 0.f;
            bool  drawing = true;
            while (t < len)
            {
                float seg = drawing ? dashLen : gapLen;
                float end = glm::min(t + seg, len);
                if (drawing) Line(p1 + dir * t, p1 + dir * end, color, thickness);
                t = end;
                drawing = !drawing;
            }
        }

        void Arrow(glm::vec2 from, glm::vec2 to, ImU32 color,
                   float thickness = 1.5f, float headSize = 6.f)
        {
            Line(from, to, color, thickness);

            glm::vec2 dir  = glm::normalize(to - from);
            glm::vec2 perp = {-dir.y, dir.x};
            TriangleFill(
                to,
                to - dir * headSize + perp * headSize * 0.5f,
                to - dir * headSize - perp * headSize * 0.5f,
                color);
        }

        // -- Circles ----------------------------------------------------------

        void Circle(glm::vec2 center, float radius, ImU32 color,
                    int segments = 0, float thickness = 1.f)
        {
            m_DL->AddCircle(iv(center), radius, color, segments, thickness);
        }

        void CircleFill(glm::vec2 center, float radius, ImU32 color, int segments = 0)
        {
            m_DL->AddCircleFilled(iv(center), radius, color, segments);
        }

        // -- Rects ------------------------------------------------------------

        void Rect(glm::vec2 min, glm::vec2 max, ImU32 color,
                  float rounding = 0.f, float thickness = 1.f)
        {
            m_DL->AddRect(iv(min), iv(max), color, rounding, 0, thickness);
        }

        void RectFill(glm::vec2 min, glm::vec2 max, ImU32 color, float rounding = 0.f)
        {
            m_DL->AddRectFilled(iv(min), iv(max), color, rounding);
        }

        void RectFillGradient(glm::vec2 min, glm::vec2 max,
                              ImU32 colUL, ImU32 colUR,
                              ImU32 colBR, ImU32 colBL)
        {
            m_DL->AddRectFilledMultiColor(iv(min), iv(max), colUL, colUR, colBR, colBL);
        }

        // -- Triangles --------------------------------------------------------

        void Triangle(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3,
                      ImU32 color, float thickness = 1.f)
        {
            m_DL->AddTriangle(iv(p1), iv(p2), iv(p3), color, thickness);
        }

        void TriangleFill(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, ImU32 color)
        {
            m_DL->AddTriangleFilled(iv(p1), iv(p2), iv(p3), color);
        }

        // -- Quads ------------------------------------------------------------

        void Quad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4,
                  ImU32 color, float thickness = 1.f)
        {
            m_DL->AddQuad(iv(p1), iv(p2), iv(p3), iv(p4), color, thickness);
        }

        void QuadFill(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, ImU32 color)
        {
            m_DL->AddQuadFilled(iv(p1), iv(p2), iv(p3), iv(p4), color);
        }

        // -- Polylines --------------------------------------------------------

        void Polyline(const glm::vec2* points, int count, ImU32 color,
                      bool closed = false, float thickness = 1.f)
        {
            m_DL->AddPolyline((const ImVec2*)points, count, color, closed, thickness);
        }

        // -- Text -------------------------------------------------------------

        void Text(glm::vec2 pos, ImU32 color, const char* text,
                  float fontSize = 0.f, ImFont* font = nullptr)
        {
            ImFont* f    = font ? font : ImGui::GetFont();
            float   size = fontSize > 0.f ? fontSize : ImGui::GetFontSize();
            m_DL->AddText(f, size, iv(pos), color, text);
        }

        // Centered text helper
        void TextCentered(glm::vec2 center, ImU32 color, const char* text,
                          float fontSize = 0.f, ImFont* font = nullptr)
        {
            ImFont* f    = font ? font : ImGui::GetFont();
            float   size = fontSize > 0.f ? fontSize : ImGui::GetFontSize();
            ImVec2  sz   = f->CalcTextSizeA(size, FLT_MAX, 0.f, text);
            Text({center.x - sz.x * 0.5f, center.y - sz.y * 0.5f}, color, text, fontSize, font);
        }

        // -- Images -----------------------------------------------------------

        void Image(ImTextureID texID, glm::vec2 min, glm::vec2 max,
                   ImU32 tint = IM_COL32_WHITE)
        {
            m_DL->AddImage(texID, iv(min), iv(max), {0,0}, {1,1}, tint);
        }

        void ImageRounded(ImTextureID texID, glm::vec2 min, glm::vec2 max,
                          float rounding, ImU32 tint = IM_COL32_WHITE)
        {
            m_DL->AddImageRounded(texID, iv(min), iv(max), {0,0}, {1,1}, tint, rounding);
        }

        // -- Clip rect --------------------------------------------------------

        void PushClip(glm::vec2 min, glm::vec2 max, bool intersect = false)
        {
            m_DL->PushClipRect(iv(min), iv(max), intersect);
        }

        void PopClip() { m_DL->PopClipRect(); }

        // -- Text size helper -------------------------------------------------

        glm::vec2 CalcTextSize(const char* text, float fontSize = 0.f, ImFont* font = nullptr)
        {
            ImFont* f    = font ? font : ImGui::GetFont();
            float   size = fontSize > 0.f ? fontSize : ImGui::GetFontSize();
            ImVec2  sz   = f->CalcTextSizeA(size, FLT_MAX, 0.f, text);
            return {sz.x, sz.y};
        }

        // -- Raw access (escape hatch) ----------------------------------------
        ImDrawList* Raw() { return m_DL; }

    private:
        ImDrawList* m_DL;

        static ImVec2 iv(glm::vec2 v) { return {v.x, v.y}; }
    };

    // -------------------------------------------------------------------------
    //  Canvas factory functions
    // -------------------------------------------------------------------------

    // Draws on top of all windows (use for HUD, radar, crosshair...)
    inline Canvas Foreground() { return Canvas(ImGui::GetForegroundDrawList()); }

    // Draws behind all windows
    inline Canvas Background() { return Canvas(ImGui::GetBackgroundDrawList()); }

    // Draws into the current ImGui window
    inline Canvas WindowCanvas() { return Canvas(ImGui::GetWindowDrawList()); }

} // namespace UI