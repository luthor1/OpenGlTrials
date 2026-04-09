#pragma once
#include <glad/glad.h>
#include <iostream>

class Framebuffer {
public:
    Framebuffer(int width, int height, int samples = 1);
    ~Framebuffer();

    void Bind();
    void Unbind();
    void Resize(int width, int height);
    void ResolveTo(unsigned int targetFBO);
    
    unsigned int GetTexture() { return m_ColorTexture; }
    unsigned int GetFBO() { return m_FBO; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

private:
    unsigned int m_FBO;
    unsigned int m_ColorTexture;
    unsigned int m_DepthStencilRBO;
    int m_Width, m_Height, m_Samples;

    void Invalidate();
};
