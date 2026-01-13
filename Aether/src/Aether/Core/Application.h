#pragma once

#include "Aether/Core/Window.h"
#include "Aether/Core/LayerStack.h"
#include "Aether/Events/Event.h"
#include "Aether/Events/ApplicationEvent.h"
#include "Aether/ImGui/ImGuiLayer.h"

#include "Aether/Core/Timestep.h"

namespace Aether {

    class AETHER_API Application
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void Close();
        void OnEvent(Event& e);

        void PushLayer(Layer* Layer);
        void PushOverlay(Layer* layer);

        static Application& Get() { return *s_Instance; }
        Window& GetWindow() { return *m_Window; }
    private:
        bool OnWindowClose(WindowCloseEvent& e);
        static Application* s_Instance;
        Scope<Window> m_Window;
        bool m_Running = true;
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;
        ImGuiLayer* m_ImGuiLayer;
    };

    Application* CreateApplication();
}