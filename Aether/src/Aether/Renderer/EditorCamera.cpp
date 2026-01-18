#include "aepch.h"
#include "EditorCamera.h"
#include "Aether/Core/Input.h"
#include "Aether/Core/KeyCodes.h"
#include "Aether/Core/MouseCodes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Aether {

    EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
    {
        UpdateView();
    }

    void EditorCamera::UpdateProjection()
    {
        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
        m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    }

    void EditorCamera::UpdateView()
    {
        m_Position = CalculatePosition();

        glm::quat orientation = GetOrientation();
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
        m_ViewMatrix = glm::inverse(m_ViewMatrix);
    }

    std::pair<float, float> EditorCamera::PanSpeed() const
    {
        float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
        float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return { xFactor, yFactor };
    }

    float EditorCamera::RotationSpeed() const
    {
        return 0.8f;
    }

    float EditorCamera::ZoomSpeed() const
    {
        float distance = m_Distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        speed = std::min(speed, 100.0f);
        return speed;
    }

    void EditorCamera::Update(Timestep ts)
    {
        const glm::vec2& mouse = { Input::GetMouseX(), Input::GetMouseY() };

        if (m_InitialMousePosition.x == 0.0f && m_InitialMousePosition.y == 0.0f)
        {
            m_InitialMousePosition = mouse;
        }

        glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
        m_InitialMousePosition = mouse;

        // Right mouse button - Rotate (orbit)
        bool isRightClick = Input::IsMouseButtonPressed(Mouse::ButtonRight);
        // Middle mouse button - Pan
        bool isMiddleClick = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);
        
        // Any mouse button interaction
        if (isRightClick || isMiddleClick)
        {
            if (!m_IsActive)
            {
                m_IsActive = true;
                Input::SetCursorMode(CursorMode::Locked);
            }

            if (isMiddleClick)
                MousePan(delta);
            else if (isRightClick)
                MouseRotate(delta);
        }
        else
        {
            if (m_IsActive)
            {
                m_IsActive = false;
                Input::SetCursorMode(CursorMode::Normal);
            }

            // WASD movement when not rotating/panning
            float moveSpeed = 10.0f;
            if (Input::IsKeyPressed(Key::LeftShift))
                moveSpeed *= 2.5f; // Faster movement with shift
            if (Input::IsKeyPressed(Key::LeftControl))
                moveSpeed *= 0.25f; // Slower movement with ctrl

            float velocity = moveSpeed * (float)ts;
            glm::vec3 forward = GetForwardDirection();
            glm::vec3 right = GetRightDirection();

            // WASD for horizontal movement
            if (Input::IsKeyPressed(Key::W))
                m_FocalPoint += forward * velocity;
            if (Input::IsKeyPressed(Key::S))
                m_FocalPoint -= forward * velocity;
            if (Input::IsKeyPressed(Key::A))
                m_FocalPoint -= right * velocity;
            if (Input::IsKeyPressed(Key::D))
                m_FocalPoint += right * velocity;
            
            // E/Q for vertical movement
            if (Input::IsKeyPressed(Key::E))
                m_FocalPoint += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
            if (Input::IsKeyPressed(Key::Q))
                m_FocalPoint -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
        }

        UpdateView();
    }

    void EditorCamera::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(AE_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
    }

    bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
    {
        float delta = e.GetYOffset() * 0.1f;
        MouseZoom(delta);
        UpdateView();
        return false;
    }

    void EditorCamera::MousePan(const glm::vec2& delta)
    {
        auto [xSpeed, ySpeed] = PanSpeed();
        m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
    }

    void EditorCamera::MouseRotate(const glm::vec2& delta)
    {
        float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
        m_Yaw += yawSign * delta.x * RotationSpeed();
        m_Pitch += delta.y * RotationSpeed();

        // Clamp pitch to prevent gimbal lock
        if (m_Pitch > 1.56f) m_Pitch = 1.56f;
        if (m_Pitch < -1.56f) m_Pitch = -1.56f;
    }

    void EditorCamera::MouseZoom(float delta)
    {
        m_Distance -= delta * ZoomSpeed();
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += GetForwardDirection();
            m_Distance = 1.0f;
        }
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 EditorCamera::CalculatePosition() const
    {
        return m_FocalPoint - GetForwardDirection() * m_Distance;
    }

    glm::quat EditorCamera::GetOrientation() const
    {
        return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    }

}