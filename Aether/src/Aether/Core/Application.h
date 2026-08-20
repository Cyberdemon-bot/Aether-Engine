#pragma once

#include "Aether/Core/Log.h"
#include "Aether/Core/Window.h"
#include "Aether/Core/LayerStack.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Events/Event.h"
#include "Aether/Events/ApplicationEvent.h"

namespace Aether {

    class ImGuiLayer;
    class ConsoleLayer;

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

        void ToggleConsole(bool state);

        static Application& Get() { return *s_Instance; }
        Window& GetWindow() { return *m_Window; }
    private:
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
            ServiceManager::ShutdownService<T>();
            instance->Shutdown();
            delete instance;
        }

        bool OnWindowClose(WindowCloseEvent& e);
        static Application* s_Instance;
        bool m_Running = true;
        bool m_ConsoleOn = true;
        Scope<Window> m_Window; 
        LayerStack m_LayerStack;
        float m_LastFrameTime = 0.0f;
        ImGuiLayer* m_ImGuiLayer;
        ConsoleLayer* m_Console;
    };

    Application* CreateApplication();
}