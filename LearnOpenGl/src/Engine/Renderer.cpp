#include "Renderer.h"
#include "ResourceManager.h"
#include <glad/glad.h>

std::shared_ptr<Framebuffer> Renderer::s_ViewportFB = nullptr;
std::shared_ptr<Shader> Renderer::s_PostProcessShader = nullptr;
unsigned int Renderer::s_QuadVAO = 0;
unsigned int Renderer::s_QuadVBO = 0;

void Renderer::Init() {
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

    s_PostProcessShader = ResourceManager::Get().LoadShader("PostProcess", "assets/post_process.glsl", "assets/post_process.glsl");
}

void Renderer::Shutdown() {
    glDeleteVertexArrays(1, &s_QuadVAO);
    glDeleteBuffers(1, &s_QuadVBO);
}

void Renderer::BeginFrame() {
    s_ViewportFB->Bind();
    glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::EndFrame() {
    s_ViewportFB->Unbind();
}

void Renderer::RenderToViewport() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    s_PostProcessShader->use();
    s_PostProcessShader->setInt("screenTexture", 0);
    s_PostProcessShader->setBool("bloom", true);
    s_PostProcessShader->setFloat("exposure", 1.0f);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_ViewportFB->GetTexture());
    glBindVertexArray(s_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
