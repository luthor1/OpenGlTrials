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
    
    static std::shared_ptr<Framebuffer> GetViewportFramebuffer() { return s_ViewportFB; }

private:
    static std::shared_ptr<Framebuffer> s_ViewportFB;
    static std::shared_ptr<Shader> s_PostProcessShader;
    static unsigned int s_QuadVAO, s_QuadVBO;
};
