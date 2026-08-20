#pragma once

#include "Aether/Core/Layer.h"
#include "Aether/Core/Delegate.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "imgui.h"

namespace Aether {
    using CommandCallback = Delegate<void(const std::vector<std::string>&)>;

    enum class LogLevel
    {
        Info,
        Warning,
        Error,
        Success,
        Command
    };

    struct LogMessage
    {
        std::string Text;
        LogLevel Level;
    };


    class AETHER_API ConsoleLayer : public Layer
    {
    public:
        ConsoleLayer();
        ~ConsoleLayer();

        virtual void Attach() override;
        virtual void Detach() override;
        virtual void OnUpdate(Timestep ts) override;
        virtual void OnEvent(Event& e) override;
        virtual void OnImGuiRender() override;

        static void RegisterCommand(const std::string& name, CommandCallback callback);
        static void ExecuteCommand(const std::string& commandLine);
        static void PushLog(const std::string& message, LogLevel level = LogLevel::Info);
    private:
        static ConsoleLayer* s_Instance;
        std::mutex m_LogMutex;

        std::vector<LogMessage> m_Messages;
        std::unordered_map<std::string, CommandCallback> m_Commands;
        std::vector<std::string> m_CommandHistory;
        
        char m_InputBuf[256] = {};
        bool m_ScrollToBottom = false;
        int m_HistoryIndex = -1;
        
        std::vector<std::string> ParseCommand(const std::string& input);
        ImVec4 GetLogColor(LogLevel level);
        static int InputCallback(ImGuiInputTextCallbackData* data);
    };
}
