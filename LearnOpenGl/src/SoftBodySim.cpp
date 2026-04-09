#include "SoftBodySim.h"
#include "MeshLibrary.h"
#include "Shader.h"
#include "SimulationManager.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>

SoftBodySim::SoftBodySim() {}
SoftBodySim::~SoftBodySim() { Shutdown(); }

void SoftBodySim::OnSetupUI() {
    ImGui::Text("Yumuşak Cisim Ayarları");
    if (ImGui::SliderInt("Boyut (GxYxD)", &m_Dim, 2, 10)) Restart();
    ImGui::SliderFloat("Sertlik (Stiffness)", &m_Stiffness, 10.0f, 2000.0f);
    ImGui::SliderFloat("Sönümleme (Damping)", &m_Damping, 0.1f, 10.0f);
}

void SoftBodySim::Initialize() {
    CreateCube();

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
    glBufferData(GL_ARRAY_BUFFER, m_Particles.size() * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribDivisor(2, 1);
}

void SoftBodySim::CreateCube() {
    m_Particles.clear();
    m_Springs.clear();
    float spacing = 0.5f;

    for (int z = 0; z < m_Dim; z++) {
        for (int y = 0; y < m_Dim; y++) {
            for (int x = 0; x < m_Dim; x++) {
                SoftParticle p;
                p.Position = glm::vec3(x * spacing - 1.0f, y * spacing + 2.0f, z * spacing - 1.0f);
                p.OldPosition = p.Position;
                p.Velocity = glm::vec3(0);
                p.Acceleration = glm::vec3(0);
                p.Mass = 1.0f;
                m_Particles.push_back(p);
            }
        }
    }

    // Connect Springs
    for (int z = 0; z < m_Dim; z++) {
        for (int y = 0; y < m_Dim; y++) {
            for (int x = 0; x < m_Dim; x++) {
                int i = x + y * m_Dim + z * m_Dim * m_Dim;
                // Structural
                if (x < m_Dim - 1) AddSpring(i, i + 1, m_Stiffness);
                if (y < m_Dim - 1) AddSpring(i, i + m_Dim, m_Stiffness);
                if (z < m_Dim - 1) AddSpring(i, i + m_Dim * m_Dim, m_Stiffness);
                // Shear
                if (x < m_Dim - 1 && y < m_Dim - 1) AddSpring(i, i + 1 + m_Dim, m_Stiffness * 0.5f);
                if (y < m_Dim - 1 && z < m_Dim - 1) AddSpring(i, i + m_Dim + m_Dim * m_Dim, m_Stiffness * 0.5f);
            }
        }
    }
}

void SoftBodySim::AddSpring(int i, int j, float stiffness) {
    Spring s;
    s.p1 = i; s.p2 = j;
    s.restLength = glm::distance(m_Particles[i].Position, m_Particles[j].Position);
    s.stiffness = stiffness;
    s.damping = m_Damping;
    m_Springs.push_back(s);
}

void SoftBodySim::Update(float dt) {
    // 1. Solve Springs (Forces)
    for (int step = 0; step < 5; step++) { // Sub-stepping for stability
        float subDt = dt / 5.0f;
        
        for (auto& s : m_Springs) {
            glm::vec3 dir = m_Particles[s.p2].Position - m_Particles[s.p1].Position;
            float dist = glm::length(dir);
            if (dist > 0.0001f) {
                glm::vec3 force = glm::normalize(dir) * (dist - s.restLength) * s.stiffness;
                m_Particles[s.p1].Acceleration += force / m_Particles[s.p1].Mass;
                m_Particles[s.p2].Acceleration -= force / m_Particles[s.p2].Mass;
            }
        }

        // 2. Integration & Constraints
        for (auto& p : m_Particles) {
            p.Acceleration += glm::vec3(0, -m_Gravity, 0);
            
            glm::vec3 nextPos = p.Position + (p.Position - p.OldPosition) * 0.98f + p.Acceleration * subDt * subDt;
            p.OldPosition = p.Position;
            p.Position = nextPos;
            p.Acceleration = glm::vec3(0);

            // Ground Collision
            if (p.Position.y < -1.0f) {
                p.Position.y = -1.0f;
                p.OldPosition.y = p.Position.y; // Zero velocity on impact for stability
            }
        }
    }

    // Update GPU Buffer
    std::vector<float> posData;
    for (auto& p : m_Particles) {
        posData.push_back(p.Position.x);
        posData.push_back(p.Position.y);
        posData.push_back(p.Position.z);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferData(GL_ARRAY_BUFFER, posData.size() * sizeof(float), posData.data(), GL_STREAM_DRAW);
}

void SoftBodySim::Render() {
    static Shader softShader("assets/instance_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    softShader.use();
    Camera& cam = SimulationManager::Get().GetCamera();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
    
    softShader.setMat4("view", view);
    softShader.setMat4("projection", projection);
    softShader.setVec3("viewPos", cam.GetPosition());
    softShader.setVec3("lightPos", glm::vec3(0, 10, 5));
    softShader.setFloat("scale", 0.15f);

    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0, (int)m_Particles.size());
}

void SoftBodySim::OnRuntimeUI() {
    ImGui::SliderFloat("Yerçekimi", &m_Gravity, 0.0f, 20.0f);
    ImGui::SliderFloat("Sertlik", &m_Stiffness, 10.0f, 2000.0f);
}

void SoftBodySim::Restart() { Shutdown(); Initialize(); }
void SoftBodySim::Shutdown() { glDeleteVertexArrays(1, &m_VAO); }
