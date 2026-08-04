#pragma once

#include "Aether/Core/Window.h"
#include "Aether/Core/LayerStack.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Events/Event.h"
#include "Aether/Events/ApplicationEvent.h"
#include "Aether/ImGui/ImGuiLayer.h"
#include "Aether/Console/ConsoleLayer.h"

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

        template<typename T>
        void InitService()
        {
            ServiceManager::Provide(new T()); 
            ServiceManager::GetService<T>()->Init();
        }

        template<typename T>
        void ShutdownService()
        {
            T* instance = ServiceManager::GetService<T>();
            instance->Shutdown();
            delete instance;
        }

        static Application& Get() { return *s_Instance; }
        Window& GetWindow() { return *m_Window; }
    private:
        bool OnWindowClose(WindowCloseEvent& e);
        static Application* s_Instance;
        bool m_Running = true;
        Scope<Window> m_Window; 
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;
        ImGuiLayer* m_ImGuiLayer;
        ConsoleLayer* m_Console;
    };

    Application* CreateApplication();
}