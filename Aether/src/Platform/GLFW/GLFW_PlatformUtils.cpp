#include "aepch.h"
#include "Aether/Utils/PlatformUtils.h"
#include "Aether/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Aether {

	float Time::GetTime()
	{
		return glfwGetTime();
	}


	std::string FileDialogs::OpenFile(const char* filter)
	{
		return "";
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		return "";
	}

}
