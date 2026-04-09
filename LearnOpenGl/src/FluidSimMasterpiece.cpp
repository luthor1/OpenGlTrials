#include "FluidSimMasterpiece.h"
#include "Engine/SpatialGrid.h"
#include "SimulationManager.h"
#include "MeshLibrary.h"
#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include "imgui/imgui.h"
#include <glad/glad.h>
#include <algorithm>
#include <cmath>

FluidSimMasterpiece::FluidSimMasterpiece() : m_VAO(0), m_VBO(0), m_IBO(0), m_InstPosVBO(0), m_InstColorVBO(0), m_IndexCount(0) {}
FluidSimMasterpiece::~FluidSimMasterpiece() { Shutdown(); }

void FluidSimMasterpiece::OnSetupUI() {
    ImGui::Text("Masterpiece Configuration");
    ImGui::SliderInt("Particle Count", &m_Count, 1000, 15000);
    ImGui::SliderFloat("Smoothing Radius", &m_SmoothingRadius, 0.05f, 0.5f);
}

void FluidSimMasterpiece::Initialize() {
    m_Particles.resize(m_Count);
    for (int i = 0; i < m_Count; ++i) {
        m_Particles[i].Position = glm::vec3(
            (rand() % 100 - 50) * 0.05f,
            (rand() % 100) * 0.05f + 1.0f,
            (rand() % 100 - 50) * 0.05f
        );
        m_Particles[i].Velocity = glm::vec3(0.0f);
        m_Particles[i].Color = glm::vec3(0.1f, 0.4f, 0.9f);
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    MeshLibrary::GetSphere(vertices, indices, 8);
    m_IndexCount = (int)indices.size();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_IBO);
    glGenBuffers(1, &m_InstPosVBO);
    glGenBuffers(1, &m_InstColorVBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Instances
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(3, 1);
}

void FluidSimMasterpiece::Update(float dt) {
    if (dt > 0.033f) dt = 0.033f; // Cap dt for stability
    ComputeDensityPressure();
    ComputeForces();
    Integrate(dt);
}

void FluidSimMasterpiece::ComputeDensityPressure() {
    SpatialGrid grid(m_SmoothingRadius);
    for (int i = 0; i < (int)m_Particles.size(); ++i) grid.Insert(i, m_Particles[i].Position);

    const float h2 = m_SmoothingRadius * m_SmoothingRadius;
    const float POLY6 = 315.0f / (64.0f * glm::pi<float>() * powf(m_SmoothingRadius, 9.0f));

    for (int i = 0; i < (int)m_Particles.size(); ++i) {
        float density = 0.0f;
        auto neighbors = grid.GetNeighbors(m_Particles[i].Position);
        for (int nj : neighbors) {
            glm::vec3 diff = m_Particles[i].Position - m_Particles[nj].Position;
            float r2 = glm::dot(diff, diff);
            if (r2 < h2) {
                density += POLY6 * powf(h2 - r2, 3.0f);
            }
        }
        m_Particles[i].Density = std::max(m_RestDensity, density);
        m_Particles[i].Pressure = m_GasConstant * (m_Particles[i].Density - m_RestDensity);
    }
}

void FluidSimMasterpiece::ComputeForces() {
    SpatialGrid grid(m_SmoothingRadius);
    for (int i = 0; i < (int)m_Particles.size(); ++i) grid.Insert(i, m_Particles[i].Position);

    const float h = m_SmoothingRadius;
    const float SPIKY_GRAD = -45.0f / (glm::pi<float>() * powf(h, 6.0f));
    const float VISC_LAP = 45.0f / (glm::pi<float>() * powf(h, 6.0f));

    for (int i = 0; i < (int)m_Particles.size(); ++i) {
        glm::vec3 fPressure(0.0f);
        glm::vec3 fViscosity(0.0f);
        glm::vec3 fSurface(0.0f);

        auto neighbors = grid.GetNeighbors(m_Particles[i].Position);
        for (int nj : neighbors) {
            if (i == nj) continue;
            glm::vec3 diff = m_Particles[i].Position - m_Particles[nj].Position;
            float r = glm::length(diff);
            if (r < h && r > 0.0001f) {
                // Pressure
                fPressure += -glm::normalize(diff) * m_Particles[i].Density * 
                             (m_Particles[i].Pressure + m_Particles[nj].Pressure) / (2.0f * m_Particles[nj].Density) * 
                             SPIKY_GRAD * powf(h - r, 2.0f);
                
                // Viscosity
                fViscosity += m_Viscosity * (m_Particles[nj].Velocity - m_Particles[i].Velocity) / m_Particles[nj].Density * 
                              VISC_LAP * (h - r);

                // Surface Tension (Simplified)
                fSurface += -m_SurfaceTension * diff * (h - r);
            }
        }
        m_Particles[i].Force = fPressure + fViscosity + fSurface + glm::vec3(0, -m_Gravity * m_Particles[i].Density, 0);
    }
}

void FluidSimMasterpiece::Integrate(float dt) {
    for (auto& p : m_Particles) {
        p.Velocity += dt * p.Force / p.Density;
        p.Position += dt * p.Velocity;

        // Boundaries
        if (p.Position.y < 0.0f) { p.Position.y = 0.0f; p.Velocity.y *= -0.5f; }
        if (p.Position.x < -2.0f) { p.Position.x = -2.0f; p.Velocity.x *= -0.5f; }
        if (p.Position.x > 2.0f) { p.Position.x = 2.0f; p.Velocity.x *= -0.5f; }
        if (p.Position.z < -2.0f) { p.Position.z = -2.0f; p.Velocity.z *= -0.5f; }
        if (p.Position.z > 2.0f) { p.Position.z = 2.0f; p.Velocity.z *= -0.5f; }

        // Color based on density
        float dRatio = (p.Density - m_RestDensity) / 500.0f;
        p.Color = glm::mix(glm::vec3(0.1f, 0.4f, 0.9f), glm::vec3(0.9f, 0.9f, 1.0f), glm::clamp(dRatio, 0.0f, 1.0f));
    }
}

void FluidSimMasterpiece::Render() {
    static auto shader = ResourceManager::Get().LoadShader("Masterpiece", "assets/instance_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    static auto standardShader = ResourceManager::Get().LoadShader("Standard3D", "assets/standard_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    
    Camera& cam = SimulationManager::Get().GetCamera();
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    glm::mat4 view = cam.GetViewMatrix();

    // 1. Draw Container (Wireframe Tank)
    standardShader->use();
    standardShader->setMat4("projection", projection);
    standardShader->setMat4("view", view);
    standardShader->setVec3("viewPos", cam.GetPosition());
    standardShader->setVec3("lightPos", glm::vec3(10, 15, 10));
    standardShader->setVec3("Color", glm::vec3(0.5f, 0.7f, 1.0f));
    
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(4.2f, 8.0f, 4.2f));
    model = glm::translate(model, glm::vec3(0, 0.5f, 0));
    standardShader->setMat4("model", model);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    MeshLibrary::DrawCube(); // Helper to draw a single cube
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // 2. Draw Fluid Particles
    shader->use();
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("viewPos", cam.GetPosition());
    shader->setVec3("lightPos", glm::vec3(5, 20, 10));
    shader->setFloat("scale", m_ParticleScale);

    std::vector<glm::vec3> posData(m_Particles.size());
    std::vector<glm::vec3> colorData(m_Particles.size());
    for (int i = 0; i < (int)m_Particles.size(); ++i) {
        posData[i] = m_Particles[i].Position;
        colorData[i] = m_Particles[i].Color;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, posData.size() * sizeof(glm::vec3), posData.data());
    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colorData.size() * sizeof(glm::vec3), colorData.data());

    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0, (int)m_Particles.size());
}

void FluidSimMasterpiece::OnRuntimeUI() {
    ImGui::SliderFloat("Viscosity", &m_Viscosity, 0.0f, 500.0f);
    ImGui::SliderFloat("Surface Tension", &m_SurfaceTension, 0.0f, 1.0f);
    ImGui::SliderFloat("Scale", &m_ParticleScale, 0.01f, 0.2f);
}

void FluidSimMasterpiece::Restart() { Initialize(); }
void FluidSimMasterpiece::Shutdown() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}
