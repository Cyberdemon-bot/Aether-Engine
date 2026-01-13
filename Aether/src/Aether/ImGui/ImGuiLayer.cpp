#include "ImGuiLayer.h"

#include "Aether/Core/Application.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Aether {

    ImGuiLayer::ImGuiLayer()
        : Layer("ImGuiLayer")
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
    }

    ImGuiContext* ImGuiLayer::GetContext() 
    {
        return ImGui::GetCurrentContext();
    }

    void ImGuiLayer::Attach()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // When viewports are enabled we tweak WindowRounding/WindowBg
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        Application& app = Application::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetWindow());

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        
        ImGui_ImplOpenGL3_Init(nullptr);
    
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void ImGuiLayer::Detach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    
    // Hàm này để test thử xem ImGui có hiện không
    void ImGuiLayer::OnImGuiRender()
    {
        static bool show = true;
        //ImGui::ShowDemoWindow(&show); // Bật dòng này lên nếu muốn xem Demo của ImGui
    }

    void ImGuiLayer::Begin()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();
        Application& app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }
}