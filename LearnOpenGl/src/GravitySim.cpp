#include "GravitySim.h"
#include "PhysicsUtility.h"
#include "Shader.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <malloc.h>
#include <random>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GravitySim::GravitySim() : m_PosX(nullptr), m_PosY(nullptr), m_VelX(nullptr), m_VelY(nullptr) {}

GravitySim::~GravitySim() { Shutdown(); }

void GravitySim::OnSetupUI() {
    ImGui::Text("Setup Gravity Simulation");
    if (ImGui::SliderInt("Particle Count", &m_Count, 1000, 500000)) {
        Restart(); // Auto-restart on count change
    }
}

void GravitySim::Initialize() {
    int count = (m_Count + 7) & ~7;
    m_PosX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_PosY = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelY = (float*)_aligned_malloc(count * sizeof(float), 32);

    std::default_random_engine gen;
    std::uniform_real_distribution<float> distPos(-0.5f, 0.5f);
    std::uniform_real_distribution<float> distVel(-1.0f, 1.0f);

    for (int i = 0; i < m_Count; ++i) {
        m_PosX[i] = distPos(gen);
        m_PosY[i] = distPos(gen);
        m_VelX[i] = distVel(gen);
        m_VelY[i] = distVel(gen);
    }

    m_InterleavedPos.resize(m_Count * 2);

    // OpenGL Setup
    float particleQuad[] = { -0.005f, 0.005f, -0.005f, -0.005f, 0.005f, 0.005f, 0.005f, -0.005f };
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_InstanceVBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particleQuad), particleQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 2 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1);
}

void GravitySim::Update(float dt) {
    if (m_UseSIMD) UpdateSIMD(dt); else UpdateScalar(dt);

    for (int i = 0; i < m_Count; ++i) {
        m_InterleavedPos[i * 2] = m_PosX[i];
        m_InterleavedPos[i * 2 + 1] = m_PosY[i];
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 2 * sizeof(float), m_InterleavedPos.data());
}

void GravitySim::UpdateScalar(float dt) {
    for (int i = 0; i < m_Count; ++i) {
        m_VelY[i] -= m_Gravity * dt;
        m_VelX[i] *= m_Friction;
        m_VelY[i] *= m_Friction;
        m_PosX[i] += m_VelX[i] * dt;
        m_PosY[i] += m_VelY[i] * dt;

        if (m_PosY[i] < -1.0f) { m_PosY[i] = -1.0f; m_VelY[i] *= -m_Bounciness; }
        if (m_PosX[i] < -1.0f) { m_PosX[i] = -1.0f; m_VelX[i] *= -m_Bounciness; }
        if (m_PosX[i] > 1.0f) { m_PosX[i] = 1.0f; m_VelX[i] *= -m_Bounciness; }
    }
}

void GravitySim::UpdateSIMD(float dt) {
    __m256 dtV = _mm256_set1_ps(dt);
    __m256 gravityV = _mm256_set1_ps(m_Gravity);
    __m256 frictionV = _mm256_set1_ps(m_Friction);
    __m256 negOneV = _mm256_set1_ps(-1.0f);
    __m256 oneV = _mm256_set1_ps(1.0f);
    __m256 bV = _mm256_set1_ps(-m_Bounciness);

    for (int i = 0; i < m_Count; i += 8) {
        __m256 px = _mm256_load_ps(&m_PosX[i]);
        __m256 py = _mm256_load_ps(&m_PosY[i]);
        __m256 vx = _mm256_load_ps(&m_VelX[i]);
        __m256 vy = _mm256_load_ps(&m_VelY[i]);

        vy = _mm256_sub_ps(vy, _mm256_mul_ps(gravityV, dtV));
        vx = _mm256_mul_ps(vx, frictionV);
        vy = _mm256_mul_ps(vy, frictionV);
        px = _mm256_add_ps(px, _mm256_mul_ps(vx, dtV));
        py = _mm256_add_ps(py, _mm256_mul_ps(vy, dtV));

        __m256 bMask = _mm256_cmp_ps(py, negOneV, _CMP_LT_OQ);
        py = _mm256_blendv_ps(py, negOneV, bMask);
        vy = _mm256_blendv_ps(vy, _mm256_mul_ps(vy, bV), bMask);

        __m256 lMask = _mm256_cmp_ps(px, negOneV, _CMP_LT_OQ);
        __m256 rMask = _mm256_cmp_ps(px, oneV, _CMP_GT_OQ);
        px = _mm256_blendv_ps(px, negOneV, lMask);
        px = _mm256_blendv_ps(px, oneV, rMask);
        vx = _mm256_blendv_ps(vx, _mm256_mul_ps(vx, bV), _mm256_or_ps(lMask, rMask));

        _mm256_store_ps(&m_PosX[i], px);
        _mm256_store_ps(&m_PosY[i], py);
        _mm256_store_ps(&m_VelX[i], vx);
        _mm256_store_ps(&m_VelY[i], vy);
    }
}

void GravitySim::Render() {
    static Shader particleShader("assets/particle_vertex.glsl", "assets/particle_fragment.glsl");
    particleShader.use();
    particleShader.setFloat("particleSize", m_ParticleSize);
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    particleShader.setMat4("projection", projection);
    glBindVertexArray(m_VAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_Count);
}

void GravitySim::OnRuntimeUI() {
    ImGui::Text("Fizik Ayarları");
    ImGui::SliderFloat("Yerçekimi", &m_Gravity, 0.0f, 20.0f);
    ImGui::SliderFloat("Sürtünme", &m_Friction, 0.9f, 1.0f);
    ImGui::SliderFloat("Bounciness", &m_Bounciness, 0.0f, 1.0f);
    ImGui::SliderFloat("Boyut", &m_ParticleSize, 0.1f, 10.0f);
    ImGui::Checkbox("SIMD Kullan", &m_UseSIMD);
    if (ImGui::Button("Sıfırla")) { Restart(); }
}

void GravitySim::Restart() {
    Shutdown();
    Initialize();
}

void GravitySim::Shutdown() {
    if (m_PosX) { _aligned_free(m_PosX); m_PosX = nullptr; }
    if (m_PosY) { _aligned_free(m_PosY); m_PosY = nullptr; }
    if (m_VelX) { _aligned_free(m_VelX); m_VelX = nullptr; }
    if (m_VelY) { _aligned_free(m_VelY); m_VelY = nullptr; }
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_InstanceVBO);
}
