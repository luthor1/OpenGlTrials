#pragma once
#include <memory>
#include "../Framebuffer.h"
#include "../Shader.h"

class Renderer {
public:
    static void Init();
    static void Shutdown();

    static void BeginFrame();
    static void EndFrame();

    static void RenderToViewport();
    static void RenderSkybox(const glm::mat4& view, const glm::mat4& projection);
    static void Resize(int w, int h);
    
    static unsigned int GetViewportTexture() { return s_ViewportFB->GetTexture(); }
    static int GetViewportWidth() { return s_IntermediateFB ? s_IntermediateFB->GetWidth() : 1280; }
    static int GetViewportHeight() { return s_IntermediateFB ? s_IntermediateFB->GetHeight() : 720; }

private:
    static std::shared_ptr<Framebuffer> s_MultisampleFB;
    static std::shared_ptr<Framebuffer> s_IntermediateFB;
    static std::shared_ptr<Framebuffer> s_ViewportFB;
    
    static std::shared_ptr<Shader> s_ViewportShader;
    static std::shared_ptr<Shader> s_SkyboxShader;
    static unsigned int s_QuadVAO, s_QuadVBO;
    static glm::vec3 s_BlackHolePos; // For lensing
};
