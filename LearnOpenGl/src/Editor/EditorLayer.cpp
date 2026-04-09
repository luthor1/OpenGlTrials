#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "EditorLayer.h"
#include "../Engine/Application.h"
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
    float winW = (float)Application::Get().GetWindow().GetWidth();
    float winH = (float)Application::Get().GetWindow().GetHeight();

    // Setup a classic sidebar-based layout
    float sidebarW = 320.0f;
    float consoleH = 180.0f;

    // 1. Simulation Library (Left Sidebar Top)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(sidebarW, winH * 0.4f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Simulation Library");
        ImGui::TextDisabled("Available Blueprints");
        ImGui::Separator();
        auto& sims = SimulationManager::Get().GetSimulations();
        for (int i = 0; i < (int)sims.size(); ++i) {
            bool isSelected = (SimulationManager::Get().GetActiveIndex() == i);
            char label[128];
            sprintf(label, "[%s] %s", isSelected ? "*" : " ", sims[i]->GetName().c_str());
            if (ImGui::Selectable(label, isSelected)) {
                SimulationManager::Get().SwitchTo(i);
                AddLog("Switched to simulation: " + sims[i]->GetName());
            }
        }
        ImGui::End();
    }

    // 2. Inspector Panel (Left Sidebar Bottom)
    {
        ImGui::SetNextWindowPos(ImVec2(10, winH * 0.4f + 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(sidebarW, winH - (winH * 0.4f + 30)), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector Panel");
        if (auto active = SimulationManager::Get().GetActive()) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "PROPERTIES: %s", active->GetName().c_str());
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 5));

            if (SimulationManager::Get().IsRunning()) {
                active->OnRuntimeUI();
                ImGui::Dummy(ImVec2(0, 10));
                if (ImGui::Button("STOP SESSION", ImVec2(-1, 35))) {
                    SimulationManager::Get().Stop();
                    AddLog("Simulation session stopped.");
                }
            } else {
                active->OnSetupUI();
                ImGui::Dummy(ImVec2(0, 10));
                if (ImGui::Button("START ENGINE", ImVec2(-1, 45))) {
                    SimulationManager::Get().Start();
                    AddLog("Simulation engine started: " + active->GetName());
                }
            }
        } else {
            ImGui::Text("No simulation selected.");
        }
        ImGui::End();
    }

    // 3. Telemetry / Performance Graph (Right Sidebar Top)
    {
        ImGui::SetNextWindowPos(ImVec2(winW - sidebarW - 10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(sidebarW, 160), ImGuiCond_FirstUseEver);
        ImGui::Begin("Performance & Telemetry");
        float currentSPF = 1000.0f / io.Framerate;
        s_FrameTimes.push_back(currentSPF);
        if (s_FrameTimes.size() > MAX_FRAME_HISTORY) s_FrameTimes.pop_front();

        std::vector<float> data(s_FrameTimes.begin(), s_FrameTimes.end());
        ImGui::PlotLines("SPF (ms)", data.data(), (int)data.size(), 0, nullptr, 0.0f, 33.0f, ImVec2(0, 80));
        
        ImGui::Columns(2, "stats");
        ImGui::Text("Current FPS:"); ImGui::NextColumn(); ImGui::TextColored(ImVec4(0,1,0,1), "%.1f", io.Framerate); ImGui::NextColumn();
        ImGui::Text("Frame Time:"); ImGui::NextColumn(); ImGui::TextColored(ImVec4(0,1,1,1), "%.3f ms", currentSPF); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::End();
    }

    // 4. Console (Bottom Center-ish)
    {
        ImGui::SetNextWindowPos(ImVec2(sidebarW + 20, winH - consoleH - 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(winW - sidebarW * 2 - 30, consoleH), ImGuiCond_FirstUseEver);
        ImGui::Begin("Engine Console");
        if (ImGui::Button("Clear")) s_ConsoleLogs.clear();
        ImGui::Separator();
        ImGui::BeginChild("LogScroll");
        for (const auto& log : s_ConsoleLogs) {
            ImGui::TextUnformatted(log.c_str());
        }
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
