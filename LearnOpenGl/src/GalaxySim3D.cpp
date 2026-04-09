#include "GalaxySim3D.h"
#include "MeshLibrary.h"
#include "Engine/ResourceManager.h"
#include "SimulationManager.h"
#include "imgui/imgui.h"
#include "Engine/Renderer.h"
#include <glad/glad.h>
#include <malloc.h>
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <immintrin.h>
#include "Engine/Renderer.h"

GalaxySim3D::GalaxySim3D() : m_PosX(nullptr), m_PosY(nullptr), m_PosZ(nullptr),
                           m_VelX(nullptr), m_VelY(nullptr), m_VelZ(nullptr),
                           m_ColorR(nullptr), m_ColorG(nullptr), m_ColorB(nullptr) {
    m_Count = 100000; // Masterpiece Count
}

GalaxySim3D::~GalaxySim3D() { Shutdown(); }

void GalaxySim3D::OnSetupUI() {
    ImGui::TextColored(ImVec4(0,1,1,1), "MASTERPIECE UNLEASHED");
    ImGui::Separator();
    if (ImGui::SliderInt("Star Count", &m_Count, 1000, 150000)) Restart();
    ImGui::SliderFloat("Universal G", &m_G, 0.0f, 0.002f);
}

void GalaxySim3D::Initialize() {
    int count = (m_Count + 7) & ~7;
    m_PosX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_PosY = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_PosZ = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelX = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelY = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_VelZ = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_ColorR = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_ColorG = (float*)_aligned_malloc(count * sizeof(float), 32);
    m_ColorB = (float*)_aligned_malloc(count * sizeof(float), 32);

    std::default_random_engine gen;
    std::uniform_real_distribution<float> distRadius(0.1f, 12.0f);
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * glm::pi<float>());

    int numArms = 5;
    float armTwist = 1.8f;

    for (int i = 0; i < m_Count; ++i) {
        float r = distRadius(gen);
        float angle = distAngle(gen);
        int armIndex = i % numArms;
        float armAngle = (armIndex * 2.0f * glm::pi<float>()) / numArms;
        float twist = r * armTwist;
        float spiralAngle = armAngle + twist;
        float spread = 1.0f / (r + 0.5f);
        
        m_PosX[i] = r * cos(spiralAngle) + (float)((rand() % 100) - 50) * 0.01f * spread;
        m_PosZ[i] = r * sin(spiralAngle) + (float)((rand() % 100) - 50) * 0.01f * spread;
        m_PosY[i] = (float)((rand() % 100) - 50) * 0.001f * (15.0f - r);

        float v = sqrt(2.8f / (r + 0.3f));
        m_VelX[i] = -sin(spiralAngle) * v;
        m_VelZ[i] = cos(spiralAngle) * v;
        m_VelY[i] = 0.0f;

        if (r < 1.0f) {
            m_ColorR[i] = 1.0f; m_ColorG[i] = 0.95f; m_ColorB[i] = 0.8f;
        } else {
            float mix = (float)(armIndex) / numArms;
            m_ColorR[i] = 0.3f + mix * 0.5f;
            m_ColorG[i] = 0.2f + (1.0f-mix) * 0.3f;
            m_ColorB[i] = 0.6f + mix * 0.4f;
        }
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_InstPosVBO);
    glGenBuffers(1, &m_InstColorVBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void GalaxySim3D::Update(float dt) {
    if (dt > 0.02f) dt = 0.02f;
    UpdateSIMD(dt);

    static std::vector<float> posData;
    static std::vector<float> colorData;
    if (posData.size() != m_Count * 3) {
        posData.resize(m_Count * 3);
        colorData.resize(m_Count * 3);
    }

    for (int i = 0; i < m_Count; ++i) {
        posData[i * 3] = m_PosX[i];
        posData[i * 3 + 1] = m_PosY[i];
        posData[i * 3 + 2] = m_PosZ[i];
        colorData[i * 3] = m_ColorR[i];
        colorData[i * 3 + 1] = m_ColorG[i];
        colorData[i * 3 + 2] = m_ColorB[i];
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 3 * sizeof(float), posData.data());
    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 3 * sizeof(float), colorData.data());
}

void GalaxySim3D::UpdateSIMD(float dt) {
    __m256 dtV = _mm256_set1_ps(dt);
    __m256 gV = _mm256_set1_ps(m_G);
    __m256 softeningV = _mm256_set1_ps(m_Softening + 0.05f);
    __m256 centralMassV = _mm256_set1_ps(1200.0f); // Ultra Massive Core

    for (int i = 0; i < m_Count; i += 8) {
        __m256 px = _mm256_load_ps(&m_PosX[i]);
        __m256 py = _mm256_load_ps(&m_PosY[i]);
        __m256 pz = _mm256_load_ps(&m_PosZ[i]);
        __m256 vx = _mm256_load_ps(&m_VelX[i]);
        __m256 vy = _mm256_load_ps(&m_VelY[i]);
        __m256 vz = _mm256_load_ps(&m_VelZ[i]);

        __m256 d2 = _mm256_add_ps(_mm256_mul_ps(px, px), _mm256_add_ps(_mm256_mul_ps(py, py), _mm256_mul_ps(pz, pz)));
        __m256 invD = _mm256_rsqrt_ps(_mm256_add_ps(d2, softeningV));
        __m256 invD3 = _mm256_mul_ps(invD, _mm256_mul_ps(invD, invD));
        __m256 f = _mm256_mul_ps(_mm256_mul_ps(gV, centralMassV), invD3);

        vx = _mm256_add_ps(vx, _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), px), _mm256_mul_ps(f, dtV)));
        vz = _mm256_add_ps(vz, _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), pz), _mm256_mul_ps(f, dtV)));
        vy = _mm256_add_ps(vy, _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), py), _mm256_mul_ps(f, dtV)));

        px = _mm256_add_ps(px, _mm256_mul_ps(vx, dtV));
        py = _mm256_add_ps(py, _mm256_mul_ps(vy, dtV));
        pz = _mm256_add_ps(pz, _mm256_mul_ps(vz, dtV));

        _mm256_store_ps(&m_PosX[i], px); _mm256_store_ps(&m_PosY[i], py); _mm256_store_ps(&m_PosZ[i], pz);
        _mm256_store_ps(&m_VelX[i], vx); _mm256_store_ps(&m_VelY[i], vy); _mm256_store_ps(&m_VelZ[i], vz);
    }

    // Dynamic Star Re-spawning & Coloring
    for (int i = 0; i < m_Count; ++i) {
        float r2 = m_PosX[i]*m_PosX[i] + m_PosZ[i]*m_PosZ[i];
        if (r2 < 0.04f) { // Swallowed by Black Hole
            float radius = 9.0f + (rand() % 10) * 0.1f;
            float angle = (rand() % 628) * 0.01f;
            m_PosX[i] = radius * cos(angle);
            m_PosZ[i] = radius * sin(angle);
            m_PosY[i] = (rand() % 100 - 50) * 0.005f;
            m_VelX[i] = -sin(angle) * 1.5f;
            m_VelZ[i] = cos(angle) * 1.5f;
            // Flare color when re-spawning
            m_ColorR[i] = 1.0f; m_ColorG[i] = 0.8f; m_ColorB[i] = 0.5f;
        } else {
            // Velocity-based heating (blue shift for fast stars)
            float vel = sqrt(m_VelX[i]*m_VelX[i] + m_VelZ[i]*m_VelZ[i]);
            m_ColorG[i] = 0.4f + std::min(vel * 0.2f, 0.6f);
        }
    }
}

void GalaxySim3D::Render() {
    static auto shader = ResourceManager::Get().LoadShader("StarSprite", "assets/star_sprite_vertex.glsl", "assets/star_sprite_fragment.glsl");
    shader->use();
    Camera& cam = SimulationManager::Get().GetCamera();
    
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    
    shader->setMat4("view", cam.GetViewMatrix());
    shader->setMat4("projection", projection);
    shader->setFloat("scale", m_Scale);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for glows
    glDepthMask(GL_FALSE); // Stars shouldn't block each other's glow
    
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_POINTS, 0, m_Count);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void GalaxySim3D::OnRuntimeUI() {
    ImGui::TextColored(ImVec4(1,1,0,1), "GOD-LEVEL ENGINE (500K)");
    ImGui::SliderFloat("Gravity", &m_G, 0.0f, 0.005f);
    ImGui::SliderFloat("Star Glow", &m_Scale, 0.001f, 0.2f);
}

void GalaxySim3D::Restart() { Shutdown(); Initialize(); }
void GalaxySim3D::Shutdown() {
    if (m_PosX) { 
        _aligned_free(m_PosX); _aligned_free(m_PosY); _aligned_free(m_PosZ);
        _aligned_free(m_VelX); _aligned_free(m_VelY); _aligned_free(m_VelZ);
        _aligned_free(m_ColorR); _aligned_free(m_ColorG); _aligned_free(m_ColorB);
        m_PosX = nullptr;
    }
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_InstPosVBO);
    glDeleteBuffers(1, &m_InstColorVBO);
}
