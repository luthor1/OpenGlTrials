#include <iostream>
#include <vector>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "SimulationManager.h"
#include "GravitySim.h"
#include "FallSim.h"
#include "GalaxySim3D.h"
#include "FluidSimSPH.h"
#include "Framebuffer.h"
#include "SoftBodySim.h"
#include "Octree.h"
#include "Picker.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void updateFPSCounter(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Global state for camera control
bool firstMouse = true;
double lastX = 640, lastY = 360;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Physics Studio - The Metamorphosis", NULL, NULL);
	if (window == NULL) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

	glViewport(0, 0, 1280, 720);
	glEnable(GL_DEPTH_TEST);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Register Simulations (High-fidelity physics suite)
	SimulationManager::Get().RegisterSimulation(std::make_unique<SoftBodySim>());
	SimulationManager::Get().RegisterSimulation(std::make_unique<FluidSimSPH>());
	SimulationManager::Get().RegisterSimulation(std::make_unique<GalaxySim3D>());
	SimulationManager::Get().RegisterSimulation(std::make_unique<GravitySim>());
	SimulationManager::Get().RegisterSimulation(std::make_unique<FallSim>());

	// Post-processing foundations
	bool useBloom = true;
	float exposure = 1.0f;
	float lastFrame = 0.0f;

	Framebuffer mainFB(1280, 720);
	float quadVertices[] = { -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f };
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	
	Shader postShader("assets/post_process.glsl", "assets/post_process.glsl");

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = (float)glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		processInput(window);

		// 1. Render to Framebuffer (HDR Pass)
		mainFB.Bind();
		glClearColor(0.005f, 0.005f, 0.015f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (SimulationManager::Get().GetActive())
		{
			if (SimulationManager::Get().IsRunning() && !SimulationManager::Get().IsPaused())
				SimulationManager::Get().GetActive()->Update(deltaTime);
			
			SimulationManager::Get().GetActive()->Render();
		}
		mainFB.Unbind();

		// 2. Post-processing Pass (Tonemapping & Bloom)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		postShader.use();
		postShader.setInt("screenTexture", 0);
		postShader.setBool("bloom", useBloom);
		postShader.setFloat("exposure", exposure);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mainFB.GetTexture());
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// 3. UI Layer (Professional Studio Layout)
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Main Studio Workspace
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(300, 720));
		ImGui::Begin("Cosmic Studio Browser", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "THE MASTERPIECE FRAMEWORK");
		ImGui::Separator();
		
		auto& sims = SimulationManager::Get().GetSimulations();
		if (ImGui::TreeNodeEx("Simülasyon Kütüphanesi", ImGuiTreeNodeFlags_DefaultOpen)) {
			for (int i = 0; i < sims.size(); ++i) {
				bool isSelected = (SimulationManager::Get().GetActiveIndex() == i);
				if (ImGui::Selectable(sims[i]->GetName().c_str(), isSelected)) {
					SimulationManager::Get().SwitchTo(i);
				}
			}
			ImGui::TreePop();
		}

		if (SimulationManager::Get().GetActive() && !SimulationManager::Get().IsRunning()) {
			ImGui::Dummy(ImVec2(0, 10));
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "LAB AYARLARI");
			SimulationManager::Get().GetActive()->OnSetupUI();
			if (ImGui::Button("SİMÜLASYONU BAŞLAT", ImVec2(-1, 50))) SimulationManager::Get().Start();
		}
		ImGui::End();

		// Right Panel: Inspector
		ImGui::SetNextWindowPos(ImVec2(980, 0));
		ImGui::SetNextWindowSize(ImVec2(300, 720));
		ImGui::Begin("Fizik Müfettişi (Inspector)", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		if (SimulationManager::Get().IsRunning()) {
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "SIM: %s", SimulationManager::Get().IsPaused() ? "DURAKLATILDI" : "ÇALIŞIYOR");
			if (ImGui::Button("PLAY/PAUSE", ImVec2(-1, 35))) SimulationManager::Get().TogglePause();
			if (ImGui::Button("STOP", ImVec2(-1, 35))) SimulationManager::Get().Stop();
			ImGui::Separator();
			SimulationManager::Get().GetActive()->OnRuntimeUI();
		} else {
			ImGui::Text("Simülasyon başlatılmadı.");
		}
		ImGui::End();

		// Bottom Panel: Telemetry & Visuals
		ImGui::SetNextWindowPos(ImVec2(300, 570));
		ImGui::SetNextWindowSize(ImVec2(680, 150));
		ImGui::Begin("Telemetri & Görsel Ayarlar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::Columns(2);
		ImGui::Text("Performans:");
		ImGui::Text("FPS: %.1f", io.Framerate);
		ImGui::Text("Frame: %.3f ms", 1000.0f / io.Framerate);
		ImGui::NextColumn();
		ImGui::Text("Görsel Çıktı:");
		ImGui::Checkbox("Bloom Effekti (Post-Process)", &useBloom);
		ImGui::SliderFloat("Exposure", &exposure, 0.1f, 3.0f);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Rendering
		if (SimulationManager::Get().GetActive())
		{
			if (SimulationManager::Get().IsRunning() && !SimulationManager::Get().IsPaused())
			{
				SimulationManager::Get().GetActive()->Update(deltaTime);
			}
			SimulationManager::Get().GetActive()->Render();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		updateFPSCounter(window);
	}

	SimulationManager::Get().Stop();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Mouse rotation only if NOT hovering UI
	if (!ImGui::GetIO().WantCaptureMouse) {
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			if (firstMouse) {
				lastX = xpos; lastY = ypos; firstMouse = false;
			}
			float xoffset = (float)(xpos - lastX);
			float yoffset = (float)(lastY - ypos); 
			lastX = xpos; lastY = ypos;
			SimulationManager::Get().GetCamera().ProcessMouseMovement(xoffset, yoffset);
		} else {
			firstMouse = true;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (!ImGui::GetIO().WantCaptureMouse) {
		SimulationManager::Get().GetCamera().ProcessMouseScroll((float)yoffset);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void updateFPSCounter(GLFWwindow* window) {
	static double previousTime = 0.0;
	static int frameCount = 0;
	double currentTime = glfwGetTime();
	frameCount++;
	if (currentTime - previousTime >= 1.0) {
		std::ostringstream ss;
		ss << "Physics Studio Pro | FPS: " << frameCount;
		glfwSetWindowTitle(window, ss.str().c_str());
		frameCount = 0; previousTime = currentTime;
	}
}
