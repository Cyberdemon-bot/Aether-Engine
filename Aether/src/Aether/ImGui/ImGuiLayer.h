#pragma once

#include "Aether/Core/Layer.h"
#include "Aether/Core/Log.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace Aether {

    class AETHER_API ImGuiLayer : public Layer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        static ImGuiContext* GetContext();

        virtual void Attach() override;
        virtual void Detach() override;
        virtual void OnEvent(Event& e) override;
        virtual void OnImGuiRender() override;

        void Begin();
        void End();

    private:
        float m_Time = 0.0f;
        bool m_BlockEvents = true;
    };
}