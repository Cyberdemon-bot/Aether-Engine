#pragma once

#include <glm/glm.hpp>

namespace Aether {
    
    enum class LightType
	{
		None = 0, Spot, Directional
	};

	struct LightParam
	{
		LightType type      = LightType::None;
		glm::vec3 position  = glm::vec3(0.0f);
		glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
		glm::vec3 color     = glm::vec3(1.0f);
		float intensity     = 1.0f;
		float range         = 10.0f;
		float innerCone     = 30.0f;
		float outerCone     = 45.0f;
		bool  castShadows   = false;
	};

	struct Light
	{
		glm::vec4 positionAndType;
		glm::vec4 directionAndRange;
		glm::vec4 colorAndIntensity;
		glm::vec4 coneAngles;
		glm::mat4 lightSpaceMatrix;
	};

	struct LightsData
	{
		Light lights[MAX_LIGHTS];
		uint32_t shadowMask;
		int lightCount;
		float _pad[3];
	};
}