#include "aepch.h"
#include "Aether/Core/Input.h"

#include "Aether/Core/Application.h"
#include <GLFW/glfw3.h>

namespace Aether {

	bool Input::IsKeyPressed(const KeyCode key)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS;
	}

	void Input::SetCursorMode(CursorMode mode)
    {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetWindow());
        switch (mode)
        {
            case CursorMode::Normal:
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                break;
            case CursorMode::Hidden:
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                break;
            case CursorMode::Locked:
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                break;
        }
    }

	bool Input::IsMouseButtonPressed(const MouseCode button)
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}

}