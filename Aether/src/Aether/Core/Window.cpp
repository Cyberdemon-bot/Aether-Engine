#include "aepch.h"
#include "Aether/Core/Window.h"

#include "Platform/GLFW/GLFW_Window.h"

namespace Aether {
    Scope<Window> Window::Create(const WinProps& props)
	{
	    return CreateScope<GLFW_Window>(props);
	}
}