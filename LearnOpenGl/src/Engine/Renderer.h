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
    static void Resize(int w, int h);
    
    static unsigned int GetViewportTexture() { return s_ViewportFB->GetTexture(); }
    static int GetViewportWidth() { return s_IntermediateFB ? s_IntermediateFB->GetWidth() : 1280; }
    static int GetViewportHeight() { return s_IntermediateFB ? s_IntermediateFB->GetHeight() : 720; }

private:
    static std::shared_ptr<Framebuffer> s_MultisampleFB;
    static std::shared_ptr<Framebuffer> s_IntermediateFB;
    static std::shared_ptr<Framebuffer> s_ViewportFB;
    
    static std::shared_ptr<Shader> s_PostProcessShader;
    static unsigned int s_QuadVAO, s_QuadVBO;
};
