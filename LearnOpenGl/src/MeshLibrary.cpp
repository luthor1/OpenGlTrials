#include "MeshLibrary.h"
#include <glm/gtc/constants.hpp>

void MeshLibrary::GetSphere(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int segments) {
    vertices.clear();
    indices.clear();

    for (int y = 0; y <= segments; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = std::cos(ySegment * glm::pi<float>());
            float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

            vertices.push_back({ glm::vec3(xPos, yPos, zPos), glm::vec3(xPos, yPos, zPos), 0.0f, 0.5f });
        }
    }

    for (int y = 0; y < segments; ++y) {
        for (int x = 0; x < segments; ++x) {
            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);

            indices.push_back((y + 1) * (segments + 1) + x);
            indices.push_back(y * (segments + 1) + x + 1);
            indices.push_back((y + 1) * (segments + 1) + x + 1);
        }
    }
}

void MeshLibrary::GetGrid(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int size) {
    vertices.clear();
    indices.clear();
    for (int i = -size; i <= size; i++) {
        vertices.push_back({ glm::vec3((float)i, 0.0f, (float)-size), glm::vec3(0, 1, 0), 0.0f, 1.0f });
        vertices.push_back({ glm::vec3((float)i, 0.0f, (float)size), glm::vec3(0, 1, 0), 0.0f, 1.0f });
        vertices.push_back({ glm::vec3((float)-size, 0.0f, (float)i), glm::vec3(0, 1, 0), 0.0f, 1.0f });
        vertices.push_back({ glm::vec3((float)size, 0.0f, (float)i), glm::vec3(0, 1, 0), 0.0f, 1.0f });
    }
}
