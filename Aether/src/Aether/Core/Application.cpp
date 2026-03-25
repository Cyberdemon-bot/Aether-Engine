#include "aepch.h"
#include "Application.h"
#include "Aether/Core/Log.h"

#include "Aether/Renderer/Renderer.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Animation/AnimationSystem.h" 
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Audio/AudioSystem.h"
#include "Aether/Scripting/ScriptEngine.h"

#include "Aether/Core/Input.h"
#include "Aether/Utils/PlatformUtils.h"
#include "Aether/Assets/AssetManager.h"

namespace Aether {
    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create(WinProps("Aether Engine", 1366, 768));
        m_Window->SetEventCallback(AE_BIND_EVENT_FN(OnEvent));

        Renderer::Init();  
        JobSystem::Init();
        AnimationSystem::Init();
        PhysicsSystem::Init();
        AssetManager::Init();
        AudioSystem::Init();
        ScriptEngine::Init();

        m_ImGuiLayer = new ImGuiLayer();
        m_Console = new ConsoleLayer();
        PushLayer(m_Console);
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        AssetManager::Shutdown();
        Renderer::Shutdown();
        AnimationSystem::Shutdown();
        JobSystem::Shutdown();
        PhysicsSystem::Shutdown();
        AudioSystem::Shutdown();
        ScriptEngine::Shutdown();
    }

    void Application::Close()
	{
		m_Running = false;
	}

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->Attach();
    }

    void Application::PushOverlay(Layer* layer)
    {
        m_LayerStack.PushOverlay(layer);
        layer->Attach();
    }

    void Application::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(AE_BIND_EVENT_FN(OnWindowClose));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

    void Application::Run()
    {
        while (m_Running)
        {
            float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

            for (Layer* layer : m_LayerStack) layer->Update(timestep);
            

            m_ImGuiLayer->Begin(); 
            {
                for (Layer* layer : m_LayerStack)
                    layer->OnImGuiRender();
            }
            m_ImGuiLayer->End();

            m_Window->Update();
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false; 
        return true;
    }

}