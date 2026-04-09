#include "Application.h"
#include "Renderer.h"
#include "../Editor/EditorLayer.h"
#include <GLFW/glfw3.h>
#include <iostream>

Application* Application::s_Instance = nullptr;
static std::unique_ptr<EditorLayer> s_EditorLayer;

Application::Application() {
    s_Instance = this;
    m_Window = std::make_unique<Window>(WindowProps("Physics Studio Pro - The Metamorphosis"));
    
    Renderer::Init();
    s_EditorLayer = std::make_unique<EditorLayer>();
}

Application::~Application() {
}

void Application::Run() {
    while (m_Running) {
        float time = (float)glfwGetTime();
        float deltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;

        glfwPollEvents();

        // 1. Physics / Simulation Update
        if (SimulationManager::Get().GetActive() && SimulationManager::Get().IsRunning() && !SimulationManager::Get().IsPaused()) {
            SimulationManager::Get().GetActive()->Update(deltaTime);
        }

        // 2. Rendering
        Renderer::BeginFrame();
        if (auto active = SimulationManager::Get().GetActive()) {
            active->Render();
        }
        Renderer::EndFrame();

        // 3. UI
        s_EditorLayer->Begin();
        s_EditorLayer->OnImGuiRender();
        s_EditorLayer->End();

        m_Window->OnUpdate();
        
        if (m_Window->ShouldClose()) {
            m_Running = false;
        }
    }
    
    Renderer::Shutdown();
}

void Application::Close() {
    m_Running = false;
}
