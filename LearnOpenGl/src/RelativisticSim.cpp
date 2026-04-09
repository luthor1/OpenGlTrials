#include "RelativisticSim.h"
#include "Engine/Renderer.h"
#include "Engine/ResourceManager.h"
#include "SimulationManager.h"
#include "Camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

void RelativisticSim::Initialize() {
    m_Shader = ResourceManager::Get().LoadShader("RelativisticBH_33", "assets/post_process_vertex.glsl", "assets/relativistic_black_hole_fragment.glsl");
    CreateQuad();
}

void RelativisticSim::Update(float dt) {}

void RelativisticSim::Render() {
    Camera& cam = SimulationManager::Get().GetCamera();
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    
    glViewport(0, 0, Renderer::GetViewportWidth(), Renderer::GetViewportHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    m_Shader->use();
    m_Shader->setVec3("uCamPos", cam.GetPosition());
    m_Shader->setMat4("uInvView", glm::inverse(cam.GetViewMatrix()));
    m_Shader->setMat4("uInvProj", glm::inverse(projection));
    m_Shader->setFloat("uTime", (float)glfwGetTime());

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void RelativisticSim::OnRuntimeUI() {
    ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "RELATIVISTIC ENGINE (OpenGL 3.3)");
    ImGui::Text("Resolution: %d x %d", Renderer::GetViewportWidth(), Renderer::GetViewportHeight());
    ImGui::Text("Jeodezikler Fragment Shader uzerinde hesaplaniyor.");
    
    ImGui::Separator();
    ImGui::Text("Physics Studio Engine V2.0");
    ImGui::Text("RK4 Integration Active");
}

void RelativisticSim::Restart() { Shutdown(); Initialize(); }

void RelativisticSim::Shutdown() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void RelativisticSim::CreateQuad() {
    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}
