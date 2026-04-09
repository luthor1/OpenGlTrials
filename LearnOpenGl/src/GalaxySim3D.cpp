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
    std::uniform_real_distribution<float> distRadius(0.1f, 10.0f);
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * glm::pi<float>());

    int numArms = 4;
    float armTwist = 1.2f;

    for (int i = 0; i < m_Count; ++i) {
        float r = distRadius(gen);
        float angle = distAngle(gen);
        
        // Quantize angle to arms
        int armIndex = i % numArms;
        float armAngle = (armIndex * 2.0f * glm::pi<float>()) / numArms;
        float twist = r * armTwist;
        
        // Spiral arm core
        float spiralAngle = armAngle + twist;
        
        // Add spread/dispersion
        float spread = 0.8f / (r + 0.5f);
        float offsetX = (float)((rand() % 100) - 50) * 0.01f * spread;
        float offsetZ = (float)((rand() % 100) - 50) * 0.01f * spread;
        
        m_PosX[i] = r * cos(spiralAngle) + offsetX;
        m_PosZ[i] = r * sin(spiralAngle) + offsetZ;
        m_PosY[i] = (float)((rand() % 100) - 50) * 0.001f * (12.0f - r);

        // Orbital velocity
        float v = sqrt(2.5f / (r + 0.2f));
        m_VelX[i] = -sin(spiralAngle) * v;
        m_VelZ[i] = cos(spiralAngle) * v;
        m_VelY[i] = 0.0f;

        // Masterpiece Colors
        if (r < 1.5f) {
            // Hot Core: Blue-White
            m_ColorR[i] = 0.8f + (rand() % 20) * 0.01f;
            m_ColorG[i] = 0.9f;
            m_ColorB[i] = 1.0f;
        } else {
            // Outer Arms: Pink/Blue/Orange mix
            float mix = (float)(armIndex) / numArms;
            m_ColorR[i] = 0.4f + mix * 0.4f;
            m_ColorG[i] = 0.3f + (1.0f-mix) * 0.2f;
            m_ColorB[i] = 0.7f + mix * 0.3f;
        }
    }

    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices;
    MeshLibrary::GetSphere(meshVertices, meshIndices, 4); // Low-poly spheres for 100k stars
    m_IndexCount = (int)meshIndices.size();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_IBO);
    glGenBuffers(1, &m_InstPosVBO);
    glGenBuffers(1, &m_InstColorVBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(Vertex), meshVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferData(GL_ARRAY_BUFFER, m_Count * 3 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribDivisor(3, 1);
}

void GalaxySim3D::Update(float dt) {
    if (dt > 0.02f) dt = 0.02f;
    UpdateSIMD(dt);

    std::vector<float> posData(m_Count * 3);
    std::vector<float> colorData(m_Count * 3);
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
    __m256 softeningV = _mm256_set1_ps(m_Softening + 0.1f);
    __m256 centralMassV = _mm256_set1_ps(500.0f);

    for (int i = 0; i < m_Count; i += 8) {
        __m256 px = _mm256_load_ps(&m_PosX[i]);
        __m256 py = _mm256_load_ps(&m_PosY[i]);
        __m256 pz = _mm256_load_ps(&m_PosZ[i]);
        __m256 vx = _mm256_load_ps(&m_VelX[i]);
        __m256 vy = _mm256_load_ps(&m_VelY[i]);
        __m256 vz = _mm256_load_ps(&m_VelZ[i]);

        __m256 d2 = _mm256_add_ps(_mm256_mul_ps(px, px), _mm256_add_ps(_mm256_mul_ps(py, py), _mm256_mul_ps(pz, pz)));
        d2 = _mm256_add_ps(d2, softeningV);
        __m256 invD = _mm256_rsqrt_ps(d2);
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
}

void GalaxySim3D::Render() {
    static auto shader = ResourceManager::Get().LoadShader("StarShader", "assets/star_vertex.glsl", "assets/star_fragment.glsl");
    shader->use();
    Camera& cam = SimulationManager::Get().GetCamera();
    
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    
    shader->setMat4("view", cam.GetViewMatrix());
    shader->setMat4("projection", projection);
    shader->setFloat("scale", m_Scale);

    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0, m_Count);
}

void GalaxySim3D::OnRuntimeUI() {
    ImGui::Text("Active Stars: %d", m_Count);
    ImGui::SliderFloat("Gravity", &m_G, 0.0f, 0.005f);
    ImGui::SliderFloat("Star Scale", &m_Scale, 0.001f, 0.05f);
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
}
