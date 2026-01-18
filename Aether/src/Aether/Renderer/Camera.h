#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace Aether {
    // Hướng di chuyển
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,    // Bay lên (Space)
        DOWN   // Bay xuống (Shift/Ctrl)
    };

    // Các thông số mặc định
    const float YAW         = -90.0f; // Quay trái phải (mặc định nhìn về trục Z âm)
    const float PITCH       =  0.0f;  // Nhìn lên xuống
    const float SPEED       =  2.5f;  // Tốc độ di chuyển
    const float SENSITIVITY =  0.1f;  // Độ nhạy chuột
    const float ZOOM        =  45.0f; // Góc nhìn (FOV)

    class AETHER_API Camera
    {
    public:
        // Attributes
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;

        // Euler Angles
        float Yaw;
        float Pitch;

        // Options
        float MovementSpeed;
        float MouseSensitivity;
        float Zoom;

        // Constructor
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) 
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
        {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        // Trả về Ma trận View (LookAt) - Đây là phép màu biến toạ độ thế giới thành toạ độ camera
        glm::mat4 GetViewMatrix()
        {
            return glm::lookAt(Position, Position + Front, Up);
        }

        // Xử lý bàn phím
        void ProcessKeyboard(Camera_Movement direction, float deltaTime)
        {
            float velocity = MovementSpeed * deltaTime;
            if (direction == FORWARD)
                Position += Front * velocity;
            if (direction == BACKWARD)
                Position -= Front * velocity;
            if (direction == LEFT)
                Position -= Right * velocity;
            if (direction == RIGHT)
                Position += Right * velocity;
            if (direction == UP)
                Position += WorldUp * velocity; // Bay lên thẳng đứng
            if (direction == DOWN)
                Position -= WorldUp * velocity; // Bay xuống
        }

        // Xử lý chuột
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
        {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;

            Yaw   += xoffset;
            Pitch += yoffset;

            // Giới hạn nhìn lên/xuống để không bị lộn cổ (Gimbal lock)
            if (constrainPitch)
            {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            updateCameraVectors();
        }

    private:
        // Tính toán lại các vector Front, Right, Up dựa trên góc quay mới
        void updateCameraVectors()
        {
            // Tính vector Front mới
            glm::vec3 front;
            front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            front.y = sin(glm::radians(Pitch));
            front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            Front = glm::normalize(front);
            
            // Tính lại Right và Up
            Right = glm::normalize(glm::cross(Front, WorldUp));  
            Up    = glm::normalize(glm::cross(Right, Front));
        }
    };
}