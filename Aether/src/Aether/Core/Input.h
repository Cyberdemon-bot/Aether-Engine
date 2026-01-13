#pragma once

#include "Aether/Core/KeyCodes.h"
#include "Aether/Core/MouseCodes.h"

#include <glm/glm.hpp>

namespace Aether {

	enum class CursorMode {
        Normal = 0,
        Hidden = 1,
        Locked = 2 
    };

	class AETHER_API Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();

		static void SetCursorMode(CursorMode mode);
	};
}
