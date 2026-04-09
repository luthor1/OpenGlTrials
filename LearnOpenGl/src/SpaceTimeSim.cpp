#include "SpaceTimeSim.h"
#include "SimulationManager.h"
#include "Camera.h"
#include "imgui/imgui.h"
#include "Engine/Renderer.h"
#include "Engine/ResourceManager.h"
#include "MeshLibrary.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

SpaceTimeSim::SpaceTimeSim() : m_GridVAO(0), m_PlanetVAO(0) {}
SpaceTimeSim::~SpaceTimeSim() { Shutdown(); }

void SpaceTimeSim::Initialize() {
    m_G = 0.5f;
    InitializeSolarSystem();

    // 2. Create Meshes (Larger for solar system)
    CreateGrid(100, 150.0f);
    CreatePlanetMesh();
}

void SpaceTimeSim::InitializeSolarSystem() {
    m_Objects.clear();
    
    // Sun (Center)
    float sunMass = 2000.0f;
    m_Objects.push_back({ glm::vec3(0,0,0), glm::vec3(0,0,0), sunMass, 3.5f, glm::vec3(1.0, 0.8, 0.0), "Sun" });

    auto AddPlanet = [&](std::string name, float dist, float mass, float radius, glm::vec3 color) {
        float v = sqrt(m_G * sunMass / dist);
        m_Objects.push_back({ glm::vec3(dist, 0, 0), glm::vec3(0, 0, v), mass, radius, color, name });
    };

    // Scaled Solar System
    AddPlanet("Mercury", 8.0f,   0.05f, 0.2f, glm::vec3(0.7, 0.7, 0.7));
    AddPlanet("Venus",   14.0f,  0.8f,  0.4f, glm::vec3(0.9, 0.7, 0.5));
    AddPlanet("Earth",   20.0f,  1.0f,  0.45f, glm::vec3(0.2, 0.5, 1.0));
    AddPlanet("Mars",    28.0f,  0.15f, 0.3f, glm::vec3(0.9, 0.3, 0.1));
    AddPlanet("Jupiter", 45.0f,  30.0f, 1.2f, glm::vec3(0.8, 0.6, 0.4));
    AddPlanet("Saturn",  60.0f,  20.0f, 1.0f, glm::vec3(0.9, 0.8, 0.6));
    AddPlanet("Uranus",  75.0f,  10.0f, 0.7f, glm::vec3(0.6, 0.8, 0.9));
    AddPlanet("Neptune", 90.0f,  8.0f,  0.7f, glm::vec3(0.3, 0.4, 0.9));
}

void SpaceTimeSim::Update(float dt) {
    if (dt > 0.02f) dt = 0.02f;

    // N-Body Newtonian Gravity
    for (int i = 0; i < (int)m_Objects.size(); ++i) {
        glm::vec3 accel(0.0f);
        for (int j = 0; j < (int)m_Objects.size(); ++j) {
            if (i == j) continue;
            glm::vec3 diff = m_Objects[j].Position - m_Objects[i].Position;
            float dist = glm::length(diff);
            if (dist < 0.1f) dist = 0.1f;
            
            float forceMag = (m_G * m_Objects[i].Mass * m_Objects[j].Mass) / (dist * dist);
            accel += glm::normalize(diff) * (forceMag / m_Objects[i].Mass);
        }
        m_Objects[i].Velocity += accel * dt;
    }

    // Integrate
    for (auto& obj : m_Objects) {
        obj.Position += obj.Velocity * dt;
    }
}

void SpaceTimeSim::Render() {
    Camera& cam = SimulationManager::Get().GetCamera();
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    glm::mat4 view = cam.GetViewMatrix();

    // 1. Render Space-Time Grid
    if (m_ShowGrid) {
        static auto gridShader = ResourceManager::Get().LoadShader("SpaceTimeGrid", "assets/spacetime_grid_vertex.glsl", "assets/spacetime_grid_fragment.glsl");
        gridShader->use();
        gridShader->setMat4("view", view);
        gridShader->setMat4("projection", projection);
        gridShader->setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.0, 0))); // Offset slightly down

        // Update Masses for Curvature
        std::vector<glm::vec4> massData;
        for (const auto& obj : m_Objects) {
            // w is Schwarzschild Radius rs = 2GM/c^2 (scaled for sim)
            float rs = (2.0f * m_G * obj.Mass) / 100.0f; 
            massData.push_back(glm::vec4(obj.Position, rs));
            if (massData.size() >= 16) break;
        }
        gridShader->setVec4Array("u_Masses", massData.data(), (int)massData.size());
        gridShader->setInt("u_MassCount", (int)massData.size());

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(m_GridVAO);
        glDrawElements(GL_LINES, m_GridIndexCount, GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);
    }

    // 2. Render Objects (Planets/Stars)
    static auto planetShader = ResourceManager::Get().LoadShader("PlanetShader", "assets/standard_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    planetShader->use();
    planetShader->setMat4("view", view);
    planetShader->setMat4("projection", projection);
    planetShader->setVec3("viewPos", cam.GetPosition());
    planetShader->setVec3("lightPos", glm::vec3(10, 20, 10));

    glBindVertexArray(m_PlanetVAO);
    for (const auto& obj : m_Objects) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.Position);
        model = glm::scale(model, glm::vec3(obj.Radius));
        planetShader->setMat4("model", model);
        planetShader->setVec3("uColor", obj.Color);
        glDrawElements(GL_TRIANGLES, m_PlanetIndexCount, GL_UNSIGNED_INT, 0);
    }
}

void SpaceTimeSim::OnRuntimeUI() {
    ImGui::TextColored(ImVec4(0,1,1,1), "SOLAR SYSTEM ENGINE (9 BODIES)");
    ImGui::Checkbox("Show Gravity Grid", &m_ShowGrid);
    ImGui::SliderFloat("Universal G", &m_G, 0.01f, 2.0f);
    
    if (ImGui::Button("Reset Solar System")) {
        InitializeSolarSystem();
        Restart();
    }
    
    ImGui::Separator();
    ImGui::Text("Planetary Telemetry:");
    for (int i = 0; i < (int)m_Objects.size(); ++i) {
        ImGui::BulletText("%s: Dist %.1f", m_Objects[i].Name.c_str(), glm::length(m_Objects[i].Position));
    }
}

void SpaceTimeSim::Restart() { Initialize(); }
void SpaceTimeSim::Shutdown() {
    if (m_GridVAO) glDeleteVertexArrays(1, &m_GridVAO);
    if (m_PlanetVAO) glDeleteVertexArrays(1, &m_PlanetVAO);
}

void SpaceTimeSim::CreateGrid(int res, float size) {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    float halfSize = size * 0.5f;
    float step = size / (float)res;

    for (int i = 0; i <= res; ++i) {
        for (int j = 0; j <= res; ++j) {
            vertices.push_back(glm::vec3(-halfSize + j * step, 0, -halfSize + i * step));
        }
    }

    // Lines for grid
    for (int i = 0; i < res; ++i) {
        for (int j = 0; j < res; ++j) {
            int row1 = i * (res + 1);
            int row2 = (i + 1) * (res + 1);
            // Horizontal
            indices.push_back(row1 + j); indices.push_back(row1 + j + 1);
            // Vertical
            indices.push_back(row1 + j); indices.push_back(row2 + j);
        }
    }

    m_GridIndexCount = (int)indices.size();
    glGenVertexArrays(1, &m_GridVAO);
    glGenBuffers(1, &m_GridVBO);
    unsigned int ebo; glGenBuffers(1, &ebo);

    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void SpaceTimeSim::CreatePlanetMesh() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    MeshLibrary::GetSphere(vertices, indices, 24);
    m_PlanetIndexCount = (int)indices.size();

    glGenVertexArrays(1, &m_PlanetVAO);
    glGenBuffers(1, &m_PlanetVBO);
    unsigned int ebo; glGenBuffers(1, &ebo);

    glBindVertexArray(m_PlanetVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_PlanetVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
}
