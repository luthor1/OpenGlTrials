#include "FluidSimSPH.h"
#include "MeshLibrary.h"
#include "Shader.h"
#include "SimulationManager.h"
#include "Engine/SpatialGrid.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <random>
#include <glm/gtc/type_ptr.hpp>

FluidSimSPH::FluidSimSPH() {}
FluidSimSPH::~FluidSimSPH() { Shutdown(); }

void FluidSimSPH::OnSetupUI() {
    ImGui::Text("Akışkan Parametreleri");
    if (ImGui::SliderInt("Damlacık Sayısı", &m_Count, 100, 10000)) Restart();
    ImGui::SliderFloat("Yumuşatma Yarıçapı (h)", &m_SmoothingRadius, 0.05f, 0.5f);
    ImGui::SliderFloat("Viskozite", &m_Viscosity, 0.0f, 1000.0f);
}

void FluidSimSPH::Initialize() {
    m_Particles.resize(m_Count);
    std::default_random_engine gen;
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (int i = 0; i < m_Count; ++i) {
        m_Particles[i].Position = glm::vec3(dist(gen) * 0.5f, dist(gen) * 0.5f + 2.0f, dist(gen) * 0.5f);
        m_Particles[i].Velocity = glm::vec3(0.0f);
        m_Particles[i].Density = 0.0f;
    }

    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices;
    MeshLibrary::GetSphere(meshVertices, meshIndices, 6);
    m_IndexCount = (int)meshIndices.size();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_IBO);
    glGenBuffers(1, &m_InstPosVBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(Vertex), meshVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_IndexCount * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribDivisor(2, 1);
}

void FluidSimSPH::ComputeDensityPressure() {
    SpatialGrid grid(m_SmoothingRadius);
    for (int i = 0; i < m_Particles.size(); ++i) grid.Insert(i, m_Particles[i].Position);

    const float h2 = m_SmoothingRadius * m_SmoothingRadius;
    const float POLY6 = 315.0f / (64.0f * glm::pi<float>() * powf(m_SmoothingRadius, 9.0f));

    for (auto& pi : m_Particles) {
        pi.Density = 0.0f;
        auto neighbors = grid.GetNeighbors(pi.Position);
        for (int idx : neighbors) {
            auto& pj = m_Particles[idx];
            glm::vec3 r = pj.Position - pi.Position;
            float r2 = glm::dot(r, r);
            if (r2 < h2) {
                pi.Density += POLY6 * pow(h2 - r2, 3);
            }
        }
        pi.Pressure = m_GasConstant * (pi.Density - m_RestDensity);
        if (pi.Pressure < 0.0f) pi.Pressure = 0.0f;
    }
}

void FluidSimSPH::ComputeForces() {
    SpatialGrid grid(m_SmoothingRadius);
    for (int i = 0; i < (int)m_Particles.size(); ++i) grid.Insert(i, m_Particles[i].Position);

    const float SPIKY_GRAD = -45.0f / (glm::pi<float>() * powf(m_SmoothingRadius, 6.0f));
    const float VISC_LAP = 45.0f / (glm::pi<float>() * powf(m_SmoothingRadius, 6.0f));

    for (int i = 0; i < m_Particles.size(); ++i) {
        glm::vec3 fPressure(0.0f);
        glm::vec3 fViscosity(0.0f);
        auto& pi = m_Particles[i];

        auto neighbors = grid.GetNeighbors(pi.Position);
        for (int idx : neighbors) {
            if (i == idx) continue;
            auto& pj = m_Particles[idx];
            glm::vec3 r = pj.Position - pi.Position;
            float dist = glm::length(r);

            if (dist < m_SmoothingRadius && dist > 0.0001f) {
                fPressure += -glm::normalize(r) * (pi.Pressure + pj.Pressure) / (2.0f * pj.Density) * SPIKY_GRAD * (float)std::pow(m_SmoothingRadius - dist, 2);
                fViscosity += m_Viscosity * (pj.Velocity - pi.Velocity) / pj.Density * VISC_LAP * (m_SmoothingRadius - dist);
            }
        }
        glm::vec3 fGravity = glm::vec3(0, -m_Gravity, 0) * pi.Density;
        pi.Force = fPressure + fViscosity + fGravity;
    }
}

void FluidSimSPH::Integrate(float dt) {
    for (auto& p : m_Particles) {
        p.Velocity += dt * p.Force / p.Density;
        p.Position += dt * p.Velocity;

        // Boundaries
        if (p.Position.y < -1.0f) { p.Position.y = -1.0f; p.Velocity.y *= -0.5f; }
        if (abs(p.Position.x) > 2.0f) { p.Position.x = (p.Position.x > 0 ? 2.0f : -2.0f); p.Velocity.x *= -0.5f; }
        if (abs(p.Position.z) > 2.0f) { p.Position.z = (p.Position.z > 0 ? 2.0f : -2.0f); p.Velocity.z *= -0.5f; }
    }
}

void FluidSimSPH::Update(float dt) {
    ComputeDensityPressure();
    ComputeForces();
    Integrate(0.008f); // Fixed small timestep for stability

    std::vector<float> posData(m_Count * 3);
    for (int i = 0; i < m_Count; ++i) {
        posData[i * 3] = m_Particles[i].Position.x;
        posData[i * 3 + 1] = m_Particles[i].Position.y;
        posData[i * 3 + 2] = m_Particles[i].Position.z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 3 * sizeof(float), posData.data());
}

void FluidSimSPH::Render() {
    // 1. Draw Container
    static Shader lineShader("assets/particle_vertex.glsl", "assets/particle_fragment.glsl");
    static unsigned int boxVAO = 0, boxVBO = 0;
    if (boxVAO == 0) {
        float box[] = {
            -2, -1, -2,  2, -1, -2,   2, -1, -2,  2, 3, -2,   2, 3, -2, -2, 3, -2,  -2, 3, -2, -2, -1, -2,
            -2, -1,  2,  2, -1,  2,   2, -1,  2,  2, 3,  2,   2, 3,  2, -2, 3,  2,  -2, 3,  2, -2, -1,  2,
            -2, -1, -2, -2, -1,  2,   2, -1, -2,  2, -1,  2,   2,  3, -2,  2, 3,  2,  -2,  3, -2, -2, 3, 2
        };
        glGenVertexArrays(1, &boxVAO);
        glGenBuffers(1, &boxVBO);
        glBindVertexArray(boxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    
    Camera& cam = SimulationManager::Get().GetCamera();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);

    lineShader.use();
    lineShader.setMat4("projection", projection * view);
    lineShader.setFloat("particleSize", 1.0f);
    glBindVertexArray(boxVAO);
    glDrawArrays(GL_LINES, 0, 24);

    // 2. Draw Particles
    static Shader fluidShader("assets/instance_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    fluidShader.use();
    fluidShader.setMat4("view", view);
    fluidShader.setMat4("projection", projection);
    fluidShader.setVec3("viewPos", cam.GetPosition());
    fluidShader.setVec3("lightPos", glm::vec3(0, 10, 5));
    fluidShader.setFloat("scale", m_ParticleScale);

    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0, (int)m_Particles.size());
}

void FluidSimSPH::OnRuntimeUI() {
    ImGui::SliderFloat("Yerçekimi", &m_Gravity, 0.0f, 20.0f);
    ImGui::SliderFloat("Parçacık Boyutu", &m_ParticleScale, 0.01f, 0.2f);
}

void FluidSimSPH::Restart() { Shutdown(); Initialize(); }

void FluidSimSPH::Shutdown() {
    glDeleteVertexArrays(1, &m_VAO);
}
