#pragma once

#include <glm/glm.hpp>

namespace Aether {

	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 View;
		glm::vec3 Position; float _pad;
	};

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_Projection(projection) {}

		virtual ~Camera() = default;

		const glm::mat4& GetProjection() const { return m_Projection; }
		const glm::mat4& GetView() const { return m_View; }
        const glm::mat4& GetViewProjection() const { return m_ViewProjection; }
        const glm::vec3& GetPosition() const { return m_Position; }
	protected:
		glm::mat4 m_Projection = glm::mat4(1.0f);
        glm::mat4 m_View = glm::mat4(1.0f);
        glm::mat4 m_ViewProjection = glm::mat4(1.0f);
        glm::vec3 m_Position = glm::vec3(0.0f);
	};

}