#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Events/Event.h"

#include <sstream>

namespace Aether {

    struct WinProps
    {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        WinProps(const std::string& title = "Aether Engine", unsigned int width = 1600, unsigned int height = 900)
            :Title(title), Width(width), Height(height)
        {}
    };

    class AETHER_API Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        virtual ~Window() = default;

        virtual void Update() = 0;

        virtual unsigned int GetWidth() const = 0;
        virtual unsigned int GetHeight() const = 0;

        virtual unsigned int GetFramebufferWidth() const = 0;
		virtual unsigned int GetFramebufferHeight() const = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        virtual void* GetWindow() const = 0;

        static Scope<Window> Create(const WinProps& props = WinProps());
    };
}