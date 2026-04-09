#include "FallSim.h"
#include "Shader.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

FallSim::FallSim() : m_VAO(0), m_VBO(0) {}

FallSim::~FallSim() { Shutdown(); }

void FallSim::OnSetupUI() {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Fiziksel Parametreler");
    ImGui::SliderFloat("Başlangıç Yüksekliği (m)", &m_StartHeight, 1.0f, 1000.0f);
    ImGui::InputFloat("Yerçekimi İvmesi (m/s²)", &m_Gravity);
    ImGui::SliderFloat("Kütle (kg)", &m_Mass, 0.1f, 100.0f);
    
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Görsel Ölçek");
    ImGui::SliderFloat("Ölçek (m/birim)", &m_MeterScale, 1.0f, 100.0f);
    
    // Live update height on slider change
    Restart();
}

void FallSim::Initialize() {
    m_CurrentHeight = m_StartHeight;
    m_Velocity = 0.0f;
    m_Time = 0.0f;
    m_Impacted = false;
    m_ImpactTime = 0.0f;
    m_MaxVelocity = 0.0f;

    // Standard Quad Geometry
    float quad[] = { -0.05f, 0.05f, -0.05f, -0.05f, 0.05f, 0.05f, 0.05f, -0.05f };
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void FallSim::Update(float dt) {
    if (m_Impacted) return;

    m_Time += dt;
    // v = v0 + a*t -> a is -gravity
    m_Velocity += m_Gravity * dt;
    // h = h0 - v*dt
    m_CurrentHeight -= m_Velocity * dt;

    if (m_Velocity > m_MaxVelocity) m_MaxVelocity = m_Velocity;

    if (m_CurrentHeight <= 0.0f) {
        m_CurrentHeight = 0.0f;
        m_Impacted = true;
        m_ImpactTime = m_Time;
    }
}

void FallSim::Render() {
    DrawGround();
    DrawObject();
}

void FallSim::DrawGround() {
    static float line[] = { -1.0f, -0.9f, 1.0f, -0.9f };
    static unsigned int lVAO = 0, lVBO = 0;
    if (lVAO == 0) {
        glGenVertexArrays(1, &lVAO);
        glGenBuffers(1, &lVBO);
        glBindVertexArray(lVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    }

    static Shader lineShader("assets/particle_vertex.glsl", "assets/particle_fragment.glsl");
    lineShader.use();
    lineShader.setMat4("projection", glm::mat4(1.0));
    lineShader.setFloat("particleSize", 1.0f);
    
    glBindVertexArray(lVAO);
    glDrawArrays(GL_LINES, 0, 2);
}

void FallSim::DrawObject() {
    static Shader objShader("assets/particle_vertex.glsl", "assets/particle_fragment.glsl");
    objShader.use();
    
    // Map m_CurrentHeight to screen Y
    // 0m -> -0.9
    // m_StartHeight -> -0.9 + (m_StartHeight / m_MeterScale)
    float screenY = -0.9f + (m_CurrentHeight / m_MeterScale);
    
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    // Overload a bit: use projection * translation
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, screenY, 0.0f));
    
    objShader.setMat4("projection", projection * model);
    objShader.setFloat("particleSize", 1.0f); // Use base quad size

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void FallSim::OnRuntimeUI() {
    ImGui::Text("--- CANLI VERİLER ---");
    ImGui::Text("Süre: %.3f s", m_Time);
    ImGui::Text("Yükseklik: %.2f m", m_CurrentHeight);
    ImGui::Text("Hız: %.2f m/s (%.1f km/h)", m_Velocity, m_Velocity * 3.6f);
    
    if (m_Impacted) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "DÜŞÜŞ TAMAMLANDI");
        ImGui::Text("Çarpma Süresi: %.4f s", m_ImpactTime);
        ImGui::Text("Çarpma Hızı: %.2f m/s", m_Velocity);
        ImGui::Text("Teorik Süre: %.4f s", sqrt(2.0f * m_StartHeight / m_Gravity));
    }

    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("Tekrarla")) Restart();
}

void FallSim::Restart() {
    m_CurrentHeight = m_StartHeight;
    m_Velocity = 0.0f;
    m_Time = 0.0f;
    m_Impacted = false;
}

void FallSim::Shutdown() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        m_VAO = 0;
    }
}
