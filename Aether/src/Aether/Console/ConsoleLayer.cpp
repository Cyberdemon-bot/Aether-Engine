#include "aepch.h"
#include "ConsoleLayer.h"

namespace Aether {
    ConsoleLayer* ConsoleLayer::s_Instance = nullptr;
    ConsoleLayer::ConsoleLayer()
        : Layer("ConsoleLayer")
    {
        s_Instance = this;
    }

    ConsoleLayer::~ConsoleLayer()
    {
        s_Instance = nullptr;
    }

    void ConsoleLayer::Attach()
    {
        PushLog("Console ready. Type 'help' for commands.", LogLevel::Info);

        RegisterCommand("clear", [&](const std::vector<std::string>& args) {
            if (s_Instance) s_Instance->m_Messages.clear();
        });

        RegisterCommand("help", [&](const std::vector<std::string>& args) {
            if (!s_Instance) return;

            std::vector<std::string> commandNames;
            for (const auto& entry : s_Instance->m_Commands)
            {
                commandNames.push_back(entry.first);
            }
            std::sort(commandNames.begin(), commandNames.end());
            std::string helpMsg = "Available commands: ";
            for (size_t i = 0; i < commandNames.size(); i++)
            {
                helpMsg += commandNames[i];
                if (i < commandNames.size() - 1)
                    helpMsg += ", ";
            }
            PushLog(helpMsg, LogLevel::Info);
        });
    }

    void ConsoleLayer::Detach()
    {
        PushLog("Console shutdown!", LogLevel::Info);
    }

    void ConsoleLayer::Update(Timestep ts) {}

    void ConsoleLayer::OnEvent(Event& e) {}

    void ConsoleLayer::OnImGuiRender()
    {
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Console");

        if (ImGui::Button("Clear"))
        {
            m_Messages.clear();
        }

        ImGui::Separator();

        const float footer = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollRegion", ImVec2(0, -footer), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        {
            std::lock_guard<std::mutex> lock(m_LogMutex);
            for (const auto& msg : m_Messages)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GetLogColor(msg.Level));
                ImGui::TextUnformatted(msg.Text.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (m_ScrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        
        m_ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::Separator();

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | 
                                     ImGuiInputTextFlags_CallbackHistory;

        bool reclaim_focus = false;
        if (ImGui::InputText("##Input", m_InputBuf, IM_ARRAYSIZE(m_InputBuf), flags, InputCallback, this))
        {
            char* s = m_InputBuf;
            while (*s == ' ') s++;
            
            if (*s)
            {
                ExecuteCommand(s);
                std::memset(m_InputBuf, 0, sizeof(m_InputBuf));
                m_ScrollToBottom = true;
                reclaim_focus = true;
                m_HistoryIndex = -1;
            }
        }

        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1);

        ImGui::End();
    }


    void ConsoleLayer::RegisterCommand(const std::string& name, CommandCallback callback)
    {
        if (!s_Instance) return;
        
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        s_Instance->m_Commands[lower] = callback;
    }

    void ConsoleLayer::ExecuteCommand(const std::string& commandLine)
    {
        if (!s_Instance) return;
        if (commandLine.empty()) return;

        auto tokens = s_Instance->ParseCommand(commandLine);
        if (tokens.empty()) return;

        PushLog("> " + commandLine, LogLevel::Command);
        
        if (s_Instance->m_CommandHistory.empty() || s_Instance->m_CommandHistory.back() != commandLine)
        {
            s_Instance->m_CommandHistory.push_back(commandLine);
            if (s_Instance->m_CommandHistory.size() > 100)
                s_Instance->m_CommandHistory.erase(s_Instance->m_CommandHistory.begin());
        }

        std::string cmd = tokens[0];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
        
        auto it = s_Instance->m_Commands.find(cmd);
        if (it != s_Instance->m_Commands.end())
        {
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());
            it->second(args);
        }
        else
        {
            PushLog("[Error] Unknown command: '" + tokens[0] + "'", LogLevel::Error);
        }
    }

    void ConsoleLayer::PushLog(const std::string& message, LogLevel level)
    {
        if (!s_Instance) return;
        std::lock_guard<std::mutex> lock(s_Instance->m_LogMutex);
        s_Instance->m_Messages.push_back({message, level});
        s_Instance->m_ScrollToBottom = true;
    }

    std::vector<std::string> ConsoleLayer::ParseCommand(const std::string& input)
    {
        std::vector<std::string> tokens;
        std::string token;
        bool inQuotes = false;
        
        for (char c : input)
        {
            if (c == '"')
            {
                inQuotes = !inQuotes;
            }
            else if (c == ' ' && !inQuotes)
            {
                if (!token.empty())
                {
                    tokens.push_back(token);
                    token.clear();
                }
            }
            else
            {
                token += c;
            }
        }
        
        if (!token.empty())
            tokens.push_back(token);
        
        return tokens;
    }

    ImVec4 ConsoleLayer::GetLogColor(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Error:   return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            case LogLevel::Warning: return ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
            case LogLevel::Command: return ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
            case LogLevel::Success: return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            default:                return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        }
    }

    int ConsoleLayer::InputCallback(ImGuiInputTextCallbackData* data)
    {
        ConsoleLayer* console = (ConsoleLayer*)data->UserData;
        const auto& history = console->m_CommandHistory;

        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (!history.empty())
            {
                if (console->m_HistoryIndex == -1)
                    console->m_HistoryIndex = history.size() - 1;
                else if (console->m_HistoryIndex > 0)
                    console->m_HistoryIndex--;

                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history[console->m_HistoryIndex].c_str());
            }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (console->m_HistoryIndex != -1)
            {
                console->m_HistoryIndex++;
                if (console->m_HistoryIndex >= (int)history.size())
                {
                    console->m_HistoryIndex = -1;
                    data->DeleteChars(0, data->BufTextLen);
                }
                else
                {
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history[console->m_HistoryIndex].c_str());
                }
            }
        }

        return 0;
    }

} 