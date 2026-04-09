#pragma once
#include <glm/glm.hpp>
#include "Camera.h"

class Picker {
public:
    static glm::vec3 GetRayFromMouse(double x, double y, int width, int height, Camera& cam, glm::mat4 projection) {
        float x_norm = (2.0f * (float)x) / width - 1.0f;
        float y_norm = 1.0f - (2.0f * (float)y) / height;
        glm::vec4 ray_clip = glm::vec4(x_norm, y_norm, -1.0f, 1.0f);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
        glm::vec3 ray_wor = glm::vec3(glm::inverse(cam.GetViewMatrix()) * ray_eye);
        return glm::normalize(ray_wor);
    }

    static bool IntersectSphere(glm::vec3 origin, glm::vec3 dir, glm::vec3 spherePos, float radius, float& t) {
        glm::vec3 oc = origin - spherePos;
        float b = glm::dot(oc, dir);
        float c = glm::dot(oc, oc) - radius * radius;
        float h = b * b - c;
        if (h < 0.0) return false;
        t = -b - sqrt(h);
        return true;
    }
};
