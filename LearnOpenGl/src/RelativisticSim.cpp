#include "RelativisticSim.h"
#include "Engine/Renderer.h"
#include "Engine/ResourceManager.h"
#include "SimulationManager.h"
#include "Camera.h"
#include "MeshLibrary.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

// UBO Layout structs matching geodesic.frag std140
struct CameraUBOData {
    alignas(16) glm::vec3 pos;
    float tanHalfFov;
    alignas(16) glm::vec3 right;
    float aspect;
    alignas(16) glm::vec3 up;
    int moving;
    alignas(16) glm::vec3 forward;
    float uTime;
};

struct DiskUBOData {
    float r1, r2, num, thickness;
};

struct ObjectsUBOData {
    int numObjects;
    float _pad0, _pad1, _pad2;
    glm::vec4 posRadius[16];   // xyz: pos, w: radius
    glm::vec4 colorMass[16];   // rgb: color, a: mass
};

void RelativisticSim::Initialize() {
    // 1. Shaders
    // We use the new geodesic.frag for cinematic raymarching in GL 3.3
    m_Shader = ResourceManager::Get().LoadShader("RelativisticRaytrace", "assets/post_process_vertex.glsl", "assets/geodesic.frag");
    m_GridShader = ResourceManager::Get().LoadShader("WarpedGrid", "assets/grid_vertex.glsl", "assets/grid_fragment.glsl");
    m_PlanetShader = ResourceManager::Get().LoadShader("PlanetShader", "assets/standard_3d_vertex.glsl", "assets/instance_3d_fragment.glsl");

    if (!m_Shader || !m_GridShader || !m_PlanetShader) {
        std::cerr << "ERROR: Shaders failed to load in Phase 29!" << std::endl;
    } else {
        // Manually link Uniform Blocks to Binding Points for GL 3.3 (Phase 32)
        unsigned int camIndex = glGetUniformBlockIndex(m_Shader->ID, "Camera");
        if (camIndex != GL_INVALID_INDEX) glUniformBlockBinding(m_Shader->ID, camIndex, 1);

        unsigned int diskIndex = glGetUniformBlockIndex(m_Shader->ID, "Disk");
        if (diskIndex != GL_INVALID_INDEX) glUniformBlockBinding(m_Shader->ID, diskIndex, 2);

        unsigned int objIndex = glGetUniformBlockIndex(m_Shader->ID, "Objects");
        if (objIndex != GL_INVALID_INDEX) glUniformBlockBinding(m_Shader->ID, objIndex, 3);
    }

    // 2. Physics & Scaling
    const double scale = 1e9;
    m_Rs = (2.0 * G * M_SagA / (c * c)) / scale; 

    Camera& cam = SimulationManager::Get().GetCamera();
    cam.Position = glm::vec3(0.0f, 40.0f, 300.0f);
    cam.MovementSpeed = 50.0f; 

    // 3. UBOs (Available in GL 3.3)
    glGenBuffers(1, &m_CameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraUBOData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_CameraUBO);

    glGenBuffers(1, &m_DiskUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_DiskUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(DiskUBOData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_DiskUBO);

    glGenBuffers(1, &m_ObjectsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ObjectsUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ObjectsUBOData), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, m_ObjectsUBO);

    m_Objects.clear();

    CreateQuad();
    GenerateGrid();
    CreatePlanetMesh();
    m_CamSpeed = 50.0f;
    m_TimeScale = 1000.0f;
}

void RelativisticSim::Update(float dt) {
    if (dt > 0.1f) dt = 0.1f; 

    GLFWwindow* window = glfwGetCurrentContext();
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!m_FKeyPressed) {
            RelObject planet;
            float r = 150.0f + (rand() % 50); 
            float angle = (rand() % 360) * 0.0174f;
            planet.pos = glm::vec3(cos(angle) * r, 10.0f, sin(angle) * r);
            float v_si = (float)sqrt(G * M_SagA / (double(r) * 1e9)) * 0.4f; 
            planet.vel = glm::vec3(-sin(angle) * v_si / 1e9f, 0, cos(angle) * v_si / 1e9f);
            planet.mass = 5.972e24f * 100000.0f; 
            planet.radius = 40.0f;
            planet.color = glm::vec3(0.3f, 0.6f, 1.0f);
            planet.name = "Celestial Impact Body";
            m_Objects.push_back(planet);
            m_FKeyPressed = true;
        }
    } else {
        m_FKeyPressed = false;
    }

    // Camera Speed Hack (Acceleration)
    Camera& cam = SimulationManager::Get().GetCamera();
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        cam.MovementSpeed = m_CamSpeed * 3.0f;
    } else {
        cam.MovementSpeed = m_CamSpeed;
    }

    UpdatePhysics(dt);
    GenerateGrid();
}

void RelativisticSim::UpdatePhysics(float dt) {
    const double scale = 1e9;
    double totalTimeStep = (double)dt * m_TimeScale;
    const int subSteps = 20;
    double subDt = totalTimeStep / subSteps;

    auto get_accel_si = [&](const glm::dvec3& pos_scaled) -> glm::dvec3 {
        double r_scaled = glm::length(pos_scaled);
        if (r_scaled < m_Rs * 0.1) return glm::dvec3(0.0); 
        double r_si = r_scaled * scale;
        glm::dvec3 dir = -glm::normalize(pos_scaled);
        double accel_si = (G * M_SagA) / (r_si * r_si);
        return dir * accel_si;
    };

    for (auto it = m_Objects.begin(); it != m_Objects.end(); ) {
        bool absorbed = false;
        for (int step = 0; step < subSteps; ++step) {
            glm::dvec3 p0 = glm::dvec3(it->pos);
            glm::dvec3 v0 = glm::dvec3(it->vel) * scale;

            // RK4 Step
            glm::dvec3 k1_p = v0;
            glm::dvec3 k1_v = get_accel_si(p0);
            glm::dvec3 k2_p = v0 + 0.5 * subDt * k1_v;
            glm::dvec3 k2_v = get_accel_si(p0 + 0.5 * (subDt / scale) * k1_p);
            glm::dvec3 k3_p = v0 + 0.5 * subDt * k2_v;
            glm::dvec3 k3_v = get_accel_si(p0 + 0.5 * (subDt / scale) * k2_p);
            glm::dvec3 k4_p = v0 + subDt * k3_v;
            glm::dvec3 k4_v = get_accel_si(p0 + (subDt / scale) * k3_p);

            glm::dvec3 p_final = p0 + (subDt / (6.0 * scale)) * (k1_p + 2.0*k2_p + 2.0*k3_p + k4_p);
            glm::dvec3 v_final = v0 + (subDt / 6.0) * (k1_v + 2.0*k2_v + 2.0*k3_v + k4_v);

            // CCD Intersection
            glm::dvec3 L = p_final - p0;
            double a = glm::dot(L, L);
            double b = 2.0 * glm::dot(p0, L);
            double r_tol = m_Rs * 1.01;
            double c = glm::dot(p0, p0) - (r_tol * r_tol);
            double disc = b*b - 4.0*a*c;
            if (disc >= 0) {
                double t1 = (-b - sqrt(disc)) / (2.0 * a);
                if (t1 >= 0.0 && t1 <= 1.0) { absorbed = true; break; }
            }
            if (glm::length(p_final) < r_tol) { absorbed = true; break; }

            it->pos = glm::vec3(p_final);
            it->vel = glm::vec3(v_final / scale);
        }

        if (absorbed) it = m_Objects.erase(it);
        else ++it;
    }
}

void RelativisticSim::SyncUBOs() {
    Camera& cam = SimulationManager::Get().GetCamera();
    
    CameraUBOData cData;
    cData.pos = cam.Position;
    cData.tanHalfFov = tan(glm::radians(60.0f * 0.5f));
    cData.forward = cam.Front;
    cData.right = cam.Right;
    cData.up = cam.Up;
    cData.aspect = (float)Renderer::GetViewportWidth() / Renderer::GetViewportHeight();
    
    // Check if camera is actually moving or rotating for optimization
    static glm::vec3 lastPos = glm::vec3(0);
    static glm::vec3 lastFront = glm::vec3(0);
    bool isMoving = (glm::distance(cam.Position, lastPos) > 0.001f);
    bool isRotating = (glm::distance(cam.Front, lastFront) > 0.001f);
    
    cData.moving = (isMoving || isRotating) ? 1 : 0;
    
    lastPos = cam.Position;
    lastFront = cam.Front;
    
    cData.uTime = (float)glfwGetTime();

    glBindBuffer(GL_UNIFORM_BUFFER, m_CameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraUBOData), &cData);

    DiskUBOData dData;
    dData.r1 = (float)m_Rs * 2.2f;
    dData.r2 = (float)m_Rs * 5.2f;
    dData.num = 2.0f;
    dData.thickness = 1.0f;
    glBindBuffer(GL_UNIFORM_BUFFER, m_DiskUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(DiskUBOData), &dData);

    ObjectsUBOData oData;
    oData.numObjects = std::min((int)m_Objects.size(), 16);
    for (int i = 0; i < oData.numObjects; ++i) {
        oData.posRadius[i] = glm::vec4(m_Objects[i].pos, m_Objects[i].radius);
        oData.colorMass[i] = glm::vec4(m_Objects[i].color, m_Objects[i].mass);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, m_ObjectsUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ObjectsUBOData), &oData);
}

void RelativisticSim::Render() {
    int vw = Renderer::GetViewportWidth();
    int vh = Renderer::GetViewportHeight();

    SyncUBOs();

    // 1. Raymarch Black Hole via Full-Screen Quad (GL 3.3 Fragment approach)
    glViewport(0, 0, vw, vh);
    glDepthMask(GL_FALSE);
    if (m_Shader) {
        m_Shader->use();
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    glDepthMask(GL_TRUE);

    // 2. 3D Overlay (Planets & Grid)
    Camera& cam = SimulationManager::Get().GetCamera();
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)vw/vh, 0.1f, 20000.0f);
    glm::mat4 view = cam.GetViewMatrix();

    if (m_PlanetShader) {
        m_PlanetShader->use();
        m_PlanetShader->setMat4("view", view);
        m_PlanetShader->setMat4("projection", projection);
        glBindVertexArray(m_PlanetVAO);
        for (const auto& obj : m_Objects) {
            float dist = glm::length(obj.pos);
            // Fix: glm::clamp template ambiguity
            float S = glm::clamp((float)pow(m_Rs / std::max((double)dist, 1.0), 3.0), 0.0f, 40.0f) * 15.0f;
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.pos);
            glm::vec3 dir = -glm::normalize(obj.pos);
            glm::vec3 worldUp = (abs(dir.y) > 0.9f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
            glm::vec3 r = glm::normalize(glm::cross(worldUp, dir));
            glm::vec3 u = glm::cross(dir, r);
            
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(r, 0); rot[1] = glm::vec4(u, 0); rot[2] = glm::vec4(dir, 0);
            model *= rot;
            
            float stretch = 1.0f + S;
            float squeeze = 1.0f / sqrt(stretch);
            model = glm::scale(model, glm::vec3(squeeze * obj.radius, squeeze * obj.radius, stretch * obj.radius));
            
            m_PlanetShader->setMat4("model", model);
            m_PlanetShader->setVec3("uColor", obj.color);
            m_PlanetShader->setVec3("uVelocity", obj.vel);
            m_PlanetShader->setVec3("uHolePos", glm::vec3(0.0f));
            glDrawElements(GL_TRIANGLES, m_PlanetIndexCount, GL_UNSIGNED_INT, 0);
        }
    }

    if (m_GridShader) {
        m_GridShader->use();
        m_GridShader->setMat4("viewProj", projection * view);
        glBindVertexArray(m_GridVAO);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDrawElements(GL_LINES, m_GridIndexCount, GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);
    }
}

void RelativisticSim::OnRuntimeUI() {
    ImGui::TextColored(ImVec4(1, 0.2f, 0, 1), "RELATIVISTIC RAYTRACE ENGINE v3.1");
    ImGui::Text("Architecture: GLAD 3.3 (Original Motor Sync)");
    ImGui::Separator();
    
    ImGui::SliderFloat("Cam Fly Speed", &m_CamSpeed, 1.0f, 500.0f);
    ImGui::SliderFloat("Physics Time Scale", &m_TimeScale, 1.0f, 20000.0f);
    
    ImGui::Separator();
    ImGui::Text("Celestial Bodies: %d", (int)m_Objects.size());
    for (const auto& obj : m_Objects) {
        float r = glm::length(obj.pos);
        ImGui::Text("- %s (%.1fx Rs)", obj.name.c_str(), r / m_Rs);
    }
}

void RelativisticSim::Restart() { Shutdown(); Initialize(); }

void RelativisticSim::Shutdown() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_GridVAO) glDeleteVertexArrays(1, &m_GridVAO);
    if (m_PlanetVAO) glDeleteVertexArrays(1, &m_PlanetVAO);
    if (m_CameraUBO) glDeleteBuffers(1, &m_CameraUBO);
    if (m_DiskUBO) glDeleteBuffers(1, &m_DiskUBO);
    if (m_ObjectsUBO) glDeleteBuffers(1, &m_ObjectsUBO);
}

void RelativisticSim::CreateQuad() {
    float vertices[] = { -1.0f, 1.0f, 0, 1, -1.0f, -1.0f, 0, 0, 1.0f, 1.0f, 1, 1, 1.0f, -1.0f, 1, 0 };
    glGenVertexArrays(1, &m_VAO); glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO); glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void RelativisticSim::CreatePlanetMesh() {
    std::vector<Vertex> vertices; std::vector<unsigned int> indices;
    MeshLibrary::GetSphere(vertices, indices, 32);
    m_PlanetIndexCount = (int)indices.size();
    glGenVertexArrays(1, &m_PlanetVAO); glGenBuffers(1, &m_PlanetVBO); glGenBuffers(1, &m_PlanetEBO);
    glBindVertexArray(m_PlanetVAO); glBindBuffer(GL_ARRAY_BUFFER, m_PlanetVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_PlanetEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
}

void RelativisticSim::GenerateGrid() {
    const int gridSize = 42; const float spacing = 15.0f; 
    std::vector<glm::vec3> vertices; std::vector<unsigned int> indices;
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            float worldX = (x - gridSize / 2) * spacing; float worldZ = (z - gridSize / 2) * spacing;
            float r_bh = sqrt(worldX * worldX + worldZ * worldZ);
            float y = (r_bh > m_Rs) ? (float)(-3.5 * sqrt(m_Rs * (r_bh - m_Rs))) : (float)(-3.5 * m_Rs);
            vertices.push_back(glm::vec3(worldX, y, worldZ));
        }
    }
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int i = z * (gridSize + 1) + x;
            indices.push_back(i); indices.push_back(i + 1); indices.push_back(i); indices.push_back(i + gridSize + 1);
        }
    }
    if (m_GridVAO == 0) glGenVertexArrays(1, &m_GridVAO);
    if (m_GridVBO == 0) glGenBuffers(1, &m_GridVBO);
    if (m_GridEBO == 0) glGenBuffers(1, &m_GridEBO);
    glBindVertexArray(m_GridVAO); glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_GridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    m_GridIndexCount = (int)indices.size();
}