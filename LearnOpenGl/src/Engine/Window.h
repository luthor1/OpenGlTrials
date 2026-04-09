#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct WindowProps {
    std::string Title;
    uint32_t Width;
    uint32_t Height;

    WindowProps(const std::string& title = "Cosmic Engine",
                uint32_t width = 1280,
                uint32_t height = 720)
        : Title(title), Width(width), Height(height) {}
};

class Window {
public:
    Window(const WindowProps& props);
    ~Window();

    void OnUpdate();

    uint32_t GetWidth() const { return m_Data.Width; }
    uint32_t GetHeight() const { return m_Data.Height; }

    bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }
    GLFWwindow* GetNativeWindow() const { return m_Window; }

private:
    void Init(const WindowProps& props);
    void Shutdown();

    GLFWwindow* m_Window;

    struct WindowData {
        std::string Title;
        uint32_t Width, Height;
    };

    WindowData m_Data;
};
