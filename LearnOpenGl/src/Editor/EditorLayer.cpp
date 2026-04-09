#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "EditorLayer.h"
#include "../Engine/Application.h"
#include "../Engine/Renderer.h"
#include "../SimulationManager.h"
#include <iostream>
#include <vector>
#include <deque>

static std::deque<float> s_FrameTimes;
static const int MAX_FRAME_HISTORY = 100;
static std::vector<std::string> s_ConsoleLogs;

void AddLog(const std::string& msg) {
    if (s_ConsoleLogs.size() > 50) s_ConsoleLogs.erase(s_ConsoleLogs.begin());
    s_ConsoleLogs.push_back(msg);
}

EditorLayer::EditorLayer() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Removed for Master branch compatibility
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Removed for Master branch compatibility

    ImGui::StyleColorsDark();
    SetDarkThemeColors();

    GLFWwindow* window = Application::Get().GetWindow().GetNativeWindow();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

EditorLayer::~EditorLayer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorLayer::End() {
    ImGuiIO& io = ImGui::GetIO();
    Application& app = Application::Get();
    io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    /* Viewports not supported in Master branch
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    */
}

void EditorLayer::OnImGuiRender() {
    ImGuiIO& io = ImGui::GetIO();
    float winW = io.DisplaySize.x;
    float winH = io.DisplaySize.y;

    float sidebarWidth = winW * 0.20f;
    if (sidebarWidth < 260.0f) sidebarWidth = 260.0f;
    
    float consoleHeight = winH * 0.22f;
    if (consoleHeight < 150.0f) consoleHeight = 150.0f;

    float mainColumnWidth = winW - sidebarWidth;

    // 1. Sidebar (Explorer & Inspector)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, winH));
        ImGui::Begin("Engine Explorer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        ImGui::TextDisabled("ACTIVE PROJECTS");
        ImGui::Separator();
        auto& sims = SimulationManager::Get().GetSimulations();
        for (int i = 0; i < (int)sims.size(); ++i) {
            bool isSelected = (SimulationManager::Get().GetActiveIndex() == i);
            if (ImGui::Selectable(sims[i]->GetName().c_str(), isSelected)) {
                SimulationManager::Get().SwitchTo(i);
                AddLog("Project: Loaded " + sims[i]->GetName());
            }
        }
        
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Separator();
        ImGui::TextDisabled("ENGINE INSPECTOR");
        
        if (auto active = SimulationManager::Get().GetActive()) {
            ImGui::TextColored(ImVec4(0, 1, 0.8f, 1), "Target: %s", active->GetName().c_str());
            if (SimulationManager::Get().IsRunning()) {
                active->OnRuntimeUI();
                if (ImGui::Button("KILL PROCESS", ImVec2(-1, 35))) SimulationManager::Get().Stop();
            } else {
                active->OnSetupUI();
                if (ImGui::Button("LAUNCH ENGINE", ImVec2(-1, 40))) {
                    SimulationManager::Get().Start();
                    AddLog("Engine: Launching " + active->GetName());
                }
            }
        }
        
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Separator();
        ImGui::TextDisabled("TELEMETRY");
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
        
        ImGui::End();
    }

    // 2. Viewport (Main Center Workspace)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::SetNextWindowPos(ImVec2(sidebarWidth, 0));
        ImGui::SetNextWindowSize(ImVec2(mainColumnWidth, winH - consoleHeight));
        ImGui::Begin("Simulation Workspace", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
        
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Renderer")) {
                static float exposure = 1.1f;
                if (ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f)) { /* master logic */ }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x > 0 && viewportSize.y > 0) {
            // SYNCED RESIZE: Happens BEFORE Main Render in Application cycle
            Renderer::Resize((int)viewportSize.x, (int)viewportSize.y);
            ImGui::Image((void*)(intptr_t)Renderer::GetViewportTexture(), viewportSize, ImVec2(0, 1), ImVec2(1, 0));
        }

        // Unlocked Camera Interaction (Fly-Cam)
        // No longer strictly requires window focus, just hovering or active mouse usage
        if (ImGui::IsWindowHovered() || ImGui::IsWindowFocused()) {
            Camera& cam = SimulationManager::Get().GetCamera();
            float dt = io.DeltaTime;

            if (ImGui::IsKeyDown(ImGuiKey_W)) cam.ProcessKeyboard(Camera::FORWARD, dt);
            if (ImGui::IsKeyDown(ImGuiKey_S)) cam.ProcessKeyboard(Camera::BACKWARD, dt);
            if (ImGui::IsKeyDown(ImGuiKey_A)) cam.ProcessKeyboard(Camera::LEFT, dt);
            if (ImGui::IsKeyDown(ImGuiKey_D)) cam.ProcessKeyboard(Camera::RIGHT, dt);
            if (ImGui::IsKeyDown(ImGuiKey_Space)) cam.ProcessKeyboard(Camera::UP, dt);
            if (ImGui::IsKeyDown(ImGuiKey_E)) cam.ProcessKeyboard(Camera::UP, dt); // 'E' also for UP
            if (ImGui::IsKeyDown(ImGuiKey_Q)) cam.ProcessKeyboard(Camera::DOWN, dt);

            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                cam.ProcessMouseMovement(io.MouseDelta.x, -io.MouseDelta.y);
            }
            if (io.MouseWheel != 0) {
                cam.ProcessMouseScroll(io.MouseWheel);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // 3. Console (Output Panel)
    {
        ImGui::SetNextWindowPos(ImVec2(sidebarWidth, winH - consoleHeight));
        ImGui::SetNextWindowSize(ImVec2(mainColumnWidth, consoleHeight));
        ImGui::Begin("Engine Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        if (ImGui::Button("Clear Logs")) s_ConsoleLogs.clear();
        ImGui::Separator();
        ImGui::BeginChild("LogScroll");
        for (const auto& log : s_ConsoleLogs) ImGui::TextUnformatted(log.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        ImGui::End();
    }
}

void EditorLayer::SetDarkThemeColors() {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 12.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    auto& colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.08f, 0.08f, 0.09f, 0.95f };
    colors[ImGuiCol_Header] = ImVec4{ 0.15f, 0.15f, 0.16f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.25f, 0.25f, 0.27f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.12f, 0.12f, 0.13f, 1.0f };
    colors[ImGuiCol_Button] = ImVec4{ 0.15f, 0.15f, 0.16f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.25f, 0.25f, 0.27f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.12f, 0.12f, 0.13f, 1.0f };
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.12f, 0.12f, 0.13f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.18f, 0.18f, 0.19f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.09f, 0.09f, 0.10f, 1.0f };
    colors[ImGuiCol_Tab] = ImVec4{ 0.11f, 0.11f, 0.12f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.35f, 0.35f, 0.37f, 1.0f };
    colors[ImGuiCol_TabActive] = ImVec4{ 0.24f, 0.24f, 0.26f, 1.0f };
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.1f, 0.1f, 0.11f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.1f, 0.1f, 0.11f, 1.0f };
    colors[ImGuiCol_Separator] = ImVec4{ 0.2f, 0.2f, 0.21f, 1.0f };
    colors[ImGuiCol_CheckMark] = ImVec4{ 0.0f, 0.7f, 1.0f, 1.0f };
    colors[ImGuiCol_SliderGrab] = ImVec4{ 0.0f, 0.7f, 1.0f, 1.0f };
    colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.0f, 0.8f, 1.0f, 1.0f };
}
