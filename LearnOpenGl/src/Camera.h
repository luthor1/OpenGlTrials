#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

class Camera {
public:
    Camera(glm::vec3 target = glm::vec3(0.0f)) 
        : Target(target), Distance(10.0f), Yaw(0.0f), Pitch(45.0f) {}

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(GetPosition(), Target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetPosition() {
        float x = Distance * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
        float y = Distance * sin(glm::radians(Pitch));
        float z = Distance * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        return Target + glm::vec3(x, y, z);
    }

    void ProcessMouseScroll(float yoffset) {
        Distance -= yoffset;
        if (Distance < 1.0f) Distance = 1.0f;
        if (Distance > 1000.0f) Distance = 1000.0f;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        Yaw -= xoffset * 0.2f;
        Pitch += yoffset * 0.2f;
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    glm::vec3 Target;
    float Distance;
    float Yaw;
    float Pitch;
};
