#pragma once

#include "Aether/Core/Window.h"
#include "Aether/Renderer/GraphicsContext.h"
#include <GLFW/glfw3.h>
#include "glad/glad.h"

namespace Aether {

	class GLFW_Window : public Window
	{
	public:
		GLFW_Window(const WinProps& props);
		virtual ~GLFW_Window();

		virtual void OnUpdate() override;

		virtual unsigned int GetWidth() const override { return m_Data.Width; }
		virtual unsigned int GetHeight() const override { return m_Data.Height; }

		virtual unsigned int GetFramebufferWidth() const override { return m_Data.FramebufferWidth; }
		virtual unsigned int GetFramebufferHeight() const override { return m_Data.FramebufferHeight; }

		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override;

		virtual void* GetWindow() const override { return m_Window; }
	private:
		virtual void Init(const WinProps& props);
		virtual void Shutdown();

		GLFWwindow* m_Window;
		Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			unsigned int FramebufferWidth, FramebufferHeight;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}