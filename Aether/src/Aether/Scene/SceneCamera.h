#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/Camera.h"

namespace Aether {

	class AETHER_API SceneCamera : public Camera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };
	public:
		SceneCamera();
		virtual ~SceneCamera() = default;

		void SetPerspective(float verticalFOV, float neaClip, float faClip);
		void SetOrthographic(float size, float neaClip, float faClip);

		void SetViewportSize(uint32_t width, uint32_t height);

		float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
		void SetPerspectiveVerticalFOV(float verticalFov) { m_PerspectiveFOV = verticalFov; RecalculateProjection(); }
		float GetPerspectiveNeaClip() const { return m_PerspectiveNear; }
		void SetPerspectiveNeaClip(float neaClip) { m_PerspectiveNear = neaClip; RecalculateProjection(); }
		float GetPerspectiveFaClip() const { return m_PerspectiveFar; }
		void SetPerspectiveFaClip(float faClip) { m_PerspectiveFar = faClip; RecalculateProjection(); }

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
		float GetOrthographicNeaClip() const { return m_OrthographicNear; }
		void SetOrthographicNeaClip(float neaClip) { m_OrthographicNear = neaClip; RecalculateProjection(); }
		float GetOrthographicFaClip() const { return m_OrthographicFar; }
		void SetOrthographicFaClip(float faClip) { m_OrthographicFar = faClip; RecalculateProjection(); }

		ProjectionType GetProjectionType() const { return m_ProjectionType; }
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

		void SetView(const glm::mat4& view);
		float GetAspectRatio() const { return m_AspectRatio; }
	private:
		void RecalculateProjection();
	private:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;

		float m_AspectRatio = 1.778f;
	};

}
