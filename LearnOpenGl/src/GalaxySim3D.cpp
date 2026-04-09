#include "GalaxySim3D.h"
#include "MeshLibrary.h"
#include "Shader.h"
#include "SimulationManager.h"
#include <glad/glad.h>
#include "imgui/imgui.h"
#include <malloc.h>
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <immintrin.h>

GalaxySim3D::GalaxySim3D() : m_PosX(nullptr), m_PosY(nullptr), m_PosZ(nullptr),
                           m_VelX(nullptr), m_VelY(nullptr), m_VelZ(nullptr),
                           m_ColorR(nullptr), m_ColorG(nullptr), m_ColorB(nullptr) {}

GalaxySim3D::~GalaxySim3D() { Shutdown(); }

void GalaxySim3D::OnSetupUI() {
    ImGui::Text("Galaksi Yapılandırması");
    if (ImGui::SliderInt("Yıldız Sayısı", &m_Count, 1000, 100000)) {
        Restart();
    }
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
    std::uniform_real_distribution<float> distRadius(0.5f, 5.0f);
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * glm::pi<float>());

    for (int i = 0; i < m_Count; ++i) {
        float r = distRadius(gen);
        float a = distAngle(gen);
        m_PosX[i] = r * cos(a);
        m_PosZ[i] = r * sin(a);
        m_PosY[i] = (float)((rand() % 100) - 50) * 0.005f;

        // Orbital velocity (rough approximation)
        float v = sqrt(0.5f / r);
        m_VelX[i] = -sin(a) * v;
        m_VelZ[i] = cos(a) * v;
        m_VelY[i] = 0.0f;

        m_ColorR[i] = 0.5f + (r * 0.1f);
        m_ColorG[i] = 0.7f;
        m_ColorB[i] = 1.0f - (r * 0.1f);
    }

    // Mesh Setup
    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices;
    MeshLibrary::GetSphere(meshVertices, meshIndices, 8);
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
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

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
    UpdateSIMD(dt);

    std::vector<float> posData(m_Count * 3);
    std::vector<float> colorData(m_Count * 3);
    for (int i = 0; i < m_Count; ++i) {
        posData[i * 3] = m_PosX[i];
        posData[i * 3 + 1] = m_PosY[i];
        posData[i * 3 + 2] = m_PosZ[i];

        // Heat map based on speed
        float v2 = m_VelX[i] * m_VelX[i] + m_VelY[i] * m_VelY[i] + m_VelZ[i] * m_VelZ[i];
        float speed = sqrt(v2) * 5.0f; // Scale for visual impact
        
        colorData[i * 3] = 0.5f + speed * 0.5f;     // R
        colorData[i * 3 + 1] = 0.7f + speed * 0.3f; // G
        colorData[i * 3 + 2] = 1.0f;                // B (Always blueish-white)
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstPosVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 3 * sizeof(float), posData.data());
    glBindBuffer(GL_ARRAY_BUFFER, m_InstColorVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_Count * 3 * sizeof(float), colorData.data());
}

void GalaxySim3D::UpdateSIMD(float dt) {
    __m256 dtV = _mm256_set1_ps(dt);
    __m256 gV = _mm256_set1_ps(m_G);
    __m256 softeningV = _mm256_set1_ps(m_Softening);

    // Optimized Galaxy Physics: Focus on central mass + self-velocity
    for (int i = 0; i < m_Count; i += 8) {
        __m256 px = _mm256_load_ps(&m_PosX[i]);
        __m256 py = _mm256_load_ps(&m_PosY[i]);
        __m256 pz = _mm256_load_ps(&m_PosZ[i]);
        __m256 vx = _mm256_load_ps(&m_VelX[i]);
        __m256 vy = _mm256_load_ps(&m_VelY[i]);
        __m256 vz = _mm256_load_ps(&m_VelZ[i]);

        // Central Force toward (0,0,0)
        __m256 distSq = _mm256_add_ps(_mm256_mul_ps(px, px), _mm256_add_ps(_mm256_mul_ps(py, py), _mm256_mul_ps(pz, pz)));
        distSq = _mm256_add_ps(distSq, softeningV);
        __m256 invDist = _mm256_rsqrt_ps(distSq);
        __m256 invDist3 = _mm256_mul_ps(invDist, _mm256_mul_ps(invDist, invDist));
        __m256 massForce = _mm256_mul_ps(gV, _mm256_set1_ps(100.0f)); // Big central mass
        __m256 f = _mm256_mul_ps(massForce, invDist3);

        __m256 ax = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), px), f);
        __m256 ay = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), py), f);
        __m256 az = _mm256_mul_ps(_mm256_sub_ps(_mm256_setzero_ps(), pz), f);

        vx = _mm256_add_ps(vx, _mm256_mul_ps(ax, dtV));
        vy = _mm256_add_ps(vy, _mm256_mul_ps(ay, dtV));
        vz = _mm256_add_ps(vz, _mm256_mul_ps(az, dtV));

        px = _mm256_add_ps(px, _mm256_mul_ps(vx, dtV));
        py = _mm256_add_ps(py, _mm256_mul_ps(vy, dtV));
        pz = _mm256_add_ps(pz, _mm256_mul_ps(vz, dtV));

        _mm256_store_ps(&m_PosX[i], px);
        _mm256_store_ps(&m_PosY[i], py);
        _mm256_store_ps(&m_PosZ[i], pz);
        _mm256_store_ps(&m_VelX[i], vx);
        _mm256_store_ps(&m_VelY[i], vy);
        _mm256_store_ps(&m_VelZ[i], vz);
    }
}

void GalaxySim3D::Render() {
    static Shader starShader("assets/instance_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");
    starShader.use();
    
    Camera& cam = SimulationManager::Get().GetCamera();
    glm::mat4 view = cam.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
    
    starShader.setMat4("view", view);
    starShader.setMat4("projection", projection);
    starShader.setVec3("viewPos", cam.GetPosition());
    starShader.setVec3("lightPos", glm::vec3(0, 10, 0));
    starShader.setFloat("scale", m_Scale);

    glBindVertexArray(m_VAO);
    glDrawElementsInstanced(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0, m_Count);
}

void GalaxySim3D::OnRuntimeUI() {
    ImGui::SliderFloat("Yerçekimi (G)", &m_G, 0.0f, 0.001f);
    ImGui::SliderFloat("Yıldız Boyutu", &m_Scale, 0.001f, 0.1f);
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
