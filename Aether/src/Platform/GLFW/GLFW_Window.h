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

		void Update() override;

		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }

		unsigned int GetFramebufferWidth() const override { return m_Data.FramebufferWidth; }
		unsigned int GetFramebufferHeight() const override { return m_Data.FramebufferHeight; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		virtual void* GetWindow() const override { return m_Window; }
	private:
		virtual void Init(const WinProps& props);
		virtual void Shutdown();
	private:
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