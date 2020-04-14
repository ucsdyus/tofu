#ifndef UI_H_
#define UI_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>


namespace ui {

class Camera {
public:
    glm::vec3 WorldUp;  // Direction of World Up

    glm::vec3 Position;  // camera position
    glm::vec3 Front;  // main spine
    glm::vec3 Right;  // L/R
    glm::vec3 Up;  // vertical spine

    // Euler Angle
    float Yaw;
    float Pitch;

    // Zoom FoV: field of view
    float FoV;
    
    float MoveSpeed;
    float RotateSpeed;
    float ZoomSpeed;
    
    enum Movement {
        FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN
    };

    explicit Camera(glm::vec3 position) {
        WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        Position = position;
        // Front = glm::vec3(0.0f, 0.0f, -1.0f);
        // Right = glm::vec3(1.0f, 0.0f, 0.0f);
        // Up = glm::vec3(0.0f, 1.0f, 0.0f);

        Yaw = -90.0f;
        Pitch = 0.0f;
        FoV = 45.0f;

        MoveSpeed = 2.5f;
        RotateSpeed = 0.1f;
        ZoomSpeed = 1.0f;

        UpdateCamera();
    }

    virtual ~Camera() {}

    glm::mat4 GetViewMatrix() {
        // Position, LookAt Position, Up
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Move F/B/L/R
    void Move(Movement MV_CODE, float dt) {
        float ds = MoveSpeed * dt;
        switch (MV_CODE) {
        case Movement::FORWARD:
            Position += Front * ds;
            break;
        case Movement::BACKWARD:
            Position -= Front * ds;
            break;
        case Movement::LEFT:
            Position -= Right * ds;
            break;
        case Movement::RIGHT:
            Position += Right * ds;
            break;
        case Movement::UP:
            Position += WorldUp * ds;
            break;
        case Movement::DOWN:
            Position -= WorldUp * ds;
            break;
        }
    }

    // Rotate Yaw, Pitch
    void Rotate(float dx, float dy) {
        dx *= RotateSpeed;
        dy *= RotateSpeed;

        Yaw += dx;
        Pitch += dy;

        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        UpdateCamera();
    }

    void Zoom(float ds) {
        FoV -= ds * ZoomSpeed;
        if (FoV < 1.0f) FoV = 1.0f;
        if (FoV > 45.0f) FoV = 45.0f;
    }

private:
    void UpdateCamera() {
        glm::vec3 front;
        front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
        front.y = std::sin(glm::radians(Pitch));
        front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
        // Update
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};


class Perspective {
public:
    float Width;
    float Height;
    float NearPlane;
    float FarPlane;

    explicit Perspective(float width, float height, Camera* cam) {
        Width = width;
        Height = height;
        NearPlane = 0.1f;
        FarPlane = 100.0f;
        camera = cam;
    }
    virtual ~Perspective() {}

    glm::mat4 GetProjMatrix() {
        return glm::perspective(glm::radians(camera->FoV), Width / Height, NearPlane, FarPlane);
    }

private:
    Camera* camera;
};

}  // namespace ui


#endif  // UI_H_