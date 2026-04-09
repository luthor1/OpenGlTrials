#include "SoftBodySim.h"
#include "MeshLibrary.h"
#include "Shader.h"
#include "SimulationManager.h"
#include "Engine/ResourceManager.h"
#include "Engine/Renderer.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

SoftBodySim::SoftBodySim() : m_VAO(0), m_VBO(0), m_IBO(0), m_NormalVBO(0), m_IndexCount(0) {}
SoftBodySim::~SoftBodySim() { Shutdown(); }

void SoftBodySim::OnSetupUI() {
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.2f, 1.0f), "PBD MASTERPIECE ENGINE");
    ImGui::Separator();
    if (ImGui::SliderInt("Subdivisions", &m_Dim, 2, 12)) Restart();
    ImGui::SliderFloat("Gravity", &m_Gravity, 0.0f, 20.0f);
}

void SoftBodySim::Initialize() {
    CreateSoftBody();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_IBO);
    glGenBuffers(1, &m_NormalVBO);

    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_Particles.size() * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, m_NormalVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Particles.size() * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_MeshIndices.size() * sizeof(unsigned int), m_MeshIndices.data(), GL_STATIC_DRAW);
    
    m_IndexCount = (int)m_MeshIndices.size();
}

void SoftBodySim::CreateSoftBody() {
    m_Particles.clear();
    m_Constraints.clear();
    m_MeshIndices.clear();
    float spacing = 0.5f;

    for (int z = 0; z < m_Dim; z++) {
        for (int y = 0; y < m_Dim; y++) {
            for (int x = 0; x < m_Dim; x++) {
                SoftParticle p;
                p.Position = glm::vec3(x * spacing - (m_Dim-1)*spacing*0.5f, y * spacing + 1.0f, z * spacing - (m_Dim-1)*spacing*0.5f);
                p.OldPosition = p.Position;
                p.Velocity = glm::vec3(0);
                p.Mass = 1.0f;
                m_Particles.push_back(p);
            }
        }
    }

    auto getIdx = [&](int x, int y, int z) { return x + y * m_Dim + z * m_Dim * m_Dim; };

    for (int z = 0; z < m_Dim; z++) {
        for (int y = 0; y < m_Dim; y++) {
            for (int x = 0; x < m_Dim; x++) {
                int i = getIdx(x, y, z);
                if (x < m_Dim - 1) AddConstraint(i, getIdx(x + 1, y, z));
                if (y < m_Dim - 1) AddConstraint(i, getIdx(x, y + 1, z));
                if (z < m_Dim - 1) AddConstraint(i, getIdx(x, y, z + 1));
                
                // Diagonals (Shear/Bend)
                if (x < m_Dim - 1 && y < m_Dim - 1) AddConstraint(getIdx(x, y, z), getIdx(x + 1, y + 1, z));
                if (y < m_Dim - 1 && z < m_Dim - 1) AddConstraint(getIdx(x, y, z), getIdx(x, y + 1, z + 1));
                if (x < m_Dim - 1 && z < m_Dim - 1) AddConstraint(getIdx(x, y, z), getIdx(x + 1, y, z + 1));

                // 6 Faces Mesh
                if (x < m_Dim - 1 && y < m_Dim - 1) {
                    if (z == 0) { // Back
                        m_MeshIndices.push_back(getIdx(x, y, z)); m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x, y + 1, z));
                        m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x + 1, y + 1, z)); m_MeshIndices.push_back(getIdx(x, y + 1, z));
                    }
                    if (z == m_Dim - 1) { // Front
                        m_MeshIndices.push_back(getIdx(x, y, z)); m_MeshIndices.push_back(getIdx(x, y + 1, z)); m_MeshIndices.push_back(getIdx(x + 1, y, z));
                        m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x, y + 1, z)); m_MeshIndices.push_back(getIdx(x + 1, y + 1, z));
                    }
                }
                if (x < m_Dim - 1 && z < m_Dim - 1) {
                    if (y == 0) { // Bottom
                        m_MeshIndices.push_back(getIdx(x, y, z)); m_MeshIndices.push_back(getIdx(x, y, z + 1)); m_MeshIndices.push_back(getIdx(x + 1, y, z));
                        m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x, y, z + 1)); m_MeshIndices.push_back(getIdx(x + 1, y, z + 1));
                    }
                    if (y == m_Dim - 1) { // Top
                        m_MeshIndices.push_back(getIdx(x, y, z)); m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x, y, z + 1));
                        m_MeshIndices.push_back(getIdx(x + 1, y, z)); m_MeshIndices.push_back(getIdx(x + 1, y, z + 1)); m_MeshIndices.push_back(getIdx(x, y, z + 1));
                    }
                }
            }
        }
    }
}

void SoftBodySim::AddConstraint(int i, int j) {
    PBDConstraint c;
    c.p1 = i; c.p2 = j;
    c.restLength = glm::distance(m_Particles[i].Position, m_Particles[j].Position);
    m_Constraints.push_back(c);
}

void SoftBodySim::Update(float dt) {
    if (dt > 0.02f) dt = 0.02f;
    
    // Moving Collision Sphere
    static float time = 0.0f;
    time += dt * 0.5f;
    glm::vec3 spherePos = glm::vec3(sin(time) * 1.5f, 0.75f, cos(time * 0.7f) * 1.5f);
    float sphereRadius = 0.8f;
    m_LastSpherePos = spherePos;

    // 1. Predict Position with Damping
    for (auto& p : m_Particles) {
        p.Velocity += glm::vec3(0, -m_Gravity, 0) * dt;
        p.Velocity *= 0.99f; 
        p.OldPosition = p.Position;
        p.Position += p.Velocity * dt;
    }

    // 2. Solve Constraints (Iterative PBD)
    for (int iter = 0; iter < 5; iter++) {
        for (const auto& c : m_Constraints) {
            glm::vec3 dir = m_Particles[c.p2].Position - m_Particles[c.p1].Position;
            float dist = glm::length(dir);
            if (dist < 0.0001f) continue;
            float corr = (dist - c.restLength) / dist;
            float m1 = m_Particles[c.p1].Mass;
            float m2 = m_Particles[c.p2].Mass;
            m_Particles[c.p1].Position += (m2 / (m1 + m2)) * dir * corr;
            m_Particles[c.p2].Position -= (m1 / (m1 + m2)) * dir * corr;
        }

        // Collision Handling
        for (auto& p : m_Particles) {
            // Ground
            if (p.Position.y < 0.0f) { p.Position.y = 0.0f; }
            
            // Sphere Collision
            glm::vec3 sDir = p.Position - spherePos;
            float sDist = glm::length(sDir);
            if (sDist < sphereRadius) {
                p.Position = spherePos + glm::normalize(sDir) * sphereRadius;
            }

            // Box Bounds (Space Arena)
            if (std::abs(p.Position.x) > 5.0f) p.Position.x = (p.Position.x > 0 ? 5.0f : -5.0f);
            if (std::abs(p.Position.z) > 5.0f) p.Position.z = (p.Position.z > 0 ? 5.0f : -5.0f);
        }
    }

    // 3. Finalize Velocity
    for (auto& p : m_Particles) {
        p.Velocity = (p.Position - p.OldPosition) / dt;
    }

    // Update VBO
    std::vector<glm::vec3> positions;
    for (auto& p : m_Particles) positions.push_back(p.Position);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());
}

void SoftBodySim::Render() {
    static auto shader = ResourceManager::Get().LoadShader("Standard3D", "assets/standard_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    shader->use();
    Camera& cam = SimulationManager::Get().GetCamera();
    
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    glm::mat4 view = cam.GetViewMatrix();

    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("viewPos", cam.GetPosition());
    shader->setVec3("lightPos", glm::vec3(5, 10, 5));

    // 1. Draw Collision Sphere (The Attractor)
    glm::mat4 sModel = glm::translate(glm::mat4(1.0f), m_LastSpherePos);
    sModel = glm::scale(sModel, glm::vec3(1.6f)); // Radius 0.8 -> Scale 1.6
    shader->setMat4("model", sModel);
    shader->setVec3("uColor", glm::vec3(1.0f, 0.1f, 0.2f)); // Glowing Red
    MeshLibrary::DrawCube();

    // 2. Draw Floor
    glm::mat4 fModel = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.05f, 0));
    fModel = glm::scale(fModel, glm::vec3(10.0f, 0.1f, 10.0f));
    shader->setMat4("model", fModel);
    shader->setVec3("uColor", glm::vec3(0.05f, 0.1f, 0.15f));
    MeshLibrary::DrawCube();

    // 3. Draw Softbody
    shader->setVec3("uColor", glm::vec3(0.0f, 1.0f, 0.8f)); // Cyan Jelly
    shader->setMat4("model", glm::mat4(1.0f));
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
}

void SoftBodySim::OnRuntimeUI() {
    ImGui::TextColored(ImVec4(0,1,1,1), "PBD MASTERPIECE ENGINE");
    ImGui::SliderFloat("Gravity", &m_Gravity, 0.0f, 20.0f);
}

void SoftBodySim::Restart() { Shutdown(); Initialize(); }
void SoftBodySim::Shutdown() { glDeleteVertexArrays(1, &m_VAO); }
