#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    float Metallic;
    float Roughness;
};

class MeshLibrary {
public:
    static void GetSphere(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int segments = 16);
    static void GetGrid(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, int size = 10);
};
