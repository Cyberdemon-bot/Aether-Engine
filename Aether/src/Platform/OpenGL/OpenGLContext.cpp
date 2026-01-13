#include "aepch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Aether {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		AE_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{

		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		AE_CORE_ASSERT(status, "Failed to initialize Glad!");

		AE_CORE_INFO("OpenGL Info:");
		AE_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		AE_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		AE_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));

		AE_CORE_ASSERT((GLVersion.major == 4 && GLVersion.minor == 1), "Aether requires OpenGL version 4.1!");
	}

	void OpenGLContext::SwapBuffers()
	{

		glfwSwapBuffers(m_WindowHandle);
	}

}
