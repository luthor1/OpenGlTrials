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

void MeshLibrary::GetCube(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();
    float v[] = { -1,-1,-1, 0,0,-1,  1,-1,-1, 0,0,-1,  1,1,-1, 0,0,-1, -1,1,-1, 0,0,-1,
                  -1,-1,1, 0,0,1,   1,-1,1, 0,0,1,   1,1,1, 0,0,1,  -1,1,1, 0,0,1,
                  -1,1,1, -1,0,0,  -1,1,-1, -1,0,0, -1,-1,-1, -1,0,0, -1,-1,1, -1,0,0,
                   1,1,1, 1,0,0,   1,1,-1, 1,0,0,   1,-1,-1, 1,0,0,   1,-1,1, 1,0,0,
                  -1,-1,-1, 0,-1,0,  1,-1,-1, 0,-1,0,  1,-1,1, 0,-1,0, -1,-1,1, 0,-1,0,
                  -1,1,-1, 0,1,0,   1,1,-1, 0,1,0,   1,1,1, 0,1,0,  -1,1,1, 0,1,0 };
    for(int i=0; i<24; i++) vertices.push_back({glm::vec3(v[i*6],v[i*6+1],v[i*6+2]), glm::vec3(v[i*6+3],v[i*6+4],v[i*6+5]), 0, 0.5f});
    unsigned int ind[] = {0,2,1,0,3,2, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,14,13,12,15,14, 16,17,18,16,18,19, 20,22,21,20,23,22};
    for(int i=0; i<36; i++) indices.push_back(ind[i]);
}

void MeshLibrary::DrawCube() {
    static unsigned int vao = 0, vbo = 0;
    if (vao == 0) {
        float vertices[] = { -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,0.5f, 0.5f,-0.5f,0.5f, 0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f };
        unsigned int indices[] = { 0,1,2,2,3,0, 4,5,6,6,7,4, 0,1,5,5,4,0, 2,3,7,7,6,2, 1,2,6,6,5,1, 0,3,7,7,4,0 };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        unsigned int ebo; glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}
