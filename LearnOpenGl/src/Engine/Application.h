#pragma once
#include <memory>
#include <vector>
#include "Window.h"
#include "../SimulationManager.h"

class Application {
public:
    Application();
    virtual ~Application();

    static Application& Get() { return *s_Instance; }

    void Run();
    void Close();

    Window& GetWindow() { return *m_Window; }

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};
