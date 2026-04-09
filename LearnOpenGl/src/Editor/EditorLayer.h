#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

class EditorLayer {
public:
    EditorLayer();
    ~EditorLayer();

    void Begin();
    void End();
    void OnImGuiRender();

private:
    void SetDarkThemeColors();
};
