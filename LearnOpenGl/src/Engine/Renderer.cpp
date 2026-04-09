#include "Renderer.h"
#include "ResourceManager.h"
#include "SimulationManager.h"
#include "Camera.h"
#include <glad/glad.h>

std::shared_ptr<Framebuffer> Renderer::s_MultisampleFB = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_IntermediateFB = nullptr;
std::shared_ptr<Framebuffer> Renderer::s_ViewportFB = nullptr;
unsigned int Renderer::s_QuadVAO = 0;
unsigned int Renderer::s_QuadVBO = 0;
std::shared_ptr<Shader> Renderer::s_ViewportShader = nullptr;
std::shared_ptr<Shader> Renderer::s_SkyboxShader = nullptr;
glm::vec3 Renderer::s_BlackHolePos = glm::vec3(0,0,0);

void Renderer::Init() {
    s_MultisampleFB = std::make_shared<Framebuffer>(1280, 720, 4); // 4x MSAA
    s_IntermediateFB = std::make_shared<Framebuffer>(1280, 720);
    s_ViewportFB = std::make_shared<Framebuffer>(1280, 720);
    
    float quadVertices[] = { -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f };
    glGenVertexArrays(1, &s_QuadVAO);
    glGenBuffers(1, &s_QuadVBO);
    glBindVertexArray(s_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    s_ViewportShader = ResourceManager::Get().LoadShader("Viewport", "assets/post_process_vertex.glsl", "assets/post_process.glsl");
    s_SkyboxShader = ResourceManager::Get().LoadShader("Skybox", "assets/skybox_vertex.glsl", "assets/skybox_fragment.glsl");
}

void Renderer::Shutdown() {
    glDeleteVertexArrays(1, &s_QuadVAO);
    glDeleteBuffers(1, &s_QuadVBO);
}

void Renderer::BeginFrame() {
    s_MultisampleFB->Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Deep Space Black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE); // Optimize performance
}

void Renderer::EndFrame() {
    s_MultisampleFB->Unbind();
    // Resolve MSAA to intermediate
    s_MultisampleFB->ResolveTo(s_IntermediateFB->GetFBO());
}

void Renderer::RenderToViewport() {
    s_ViewportFB->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    s_ViewportShader->use();
    s_ViewportShader->setInt("screenTexture", 0);
    s_ViewportShader->setBool("bloom", true);
    s_ViewportShader->setFloat("exposure", 1.1f);
    
    // God-Level: Pass Matrices for Screenspace calculations (Lensing)
    Camera& cam = SimulationManager::Get().GetCamera();
    float aspect = (float)Renderer::GetViewportWidth() / (float)Renderer::GetViewportHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    
    s_ViewportShader->setMat4("view", cam.GetViewMatrix());
    s_ViewportShader->setMat4("projection", projection);
    s_ViewportShader->setVec3("blackHolePos", s_BlackHolePos); 

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_IntermediateFB->GetTexture());
    glBindVertexArray(s_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    s_ViewportFB->Unbind();
}

void Renderer::RenderSkybox(const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);
    s_SkyboxShader->use();
    s_SkyboxShader->setMat4("invView", glm::inverse(view));
    s_SkyboxShader->setMat4("invProjection", glm::inverse(projection));
    
    glBindVertexArray(s_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDepthFunc(GL_LESS);
}

void Renderer::Resize(int w, int h) {
    if (w > 0 && h > 0) {
        s_MultisampleFB->Resize(w, h);
        s_IntermediateFB->Resize(w, h);
        s_ViewportFB->Resize(w, h);
    }
}
