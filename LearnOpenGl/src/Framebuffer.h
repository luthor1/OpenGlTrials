#pragma once
#include <glad/glad.h>
#include <iostream>

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();

    void Bind();
    void Unbind();
    void Resize(int width, int height);
    
    unsigned int GetTexture() { return m_Texture; }

private:
    unsigned int m_FBO;
    unsigned int m_Texture;
    unsigned int m_RBO;
    int m_Width, m_Height;
};
