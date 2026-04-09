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
#include "ParticleSystem.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void updateFPSCounter(GLFWwindow* window);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "SIMD Particle Simulator", NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

	glViewport(0, 0, 1280, 720);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Setup Dear ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	Shader particleShader("assets/particle_vertex.glsl", "assets/particle_fragment.glsl");

	// Particle System Init
	const int MAX_PARTICLES = 100000;
	ParticleSystem ps(MAX_PARTICLES);

	// Geometry for a single particle (a small quad)
	float particleQuad[] = {
		-0.005f,  0.005f,
		-0.005f, -0.005f,
		 0.005f,  0.005f,
		 0.005f, -0.005f,
	};

	unsigned int VAO, VBO, instanceVBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(VAO);
	
	// Base geometry
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(particleQuad), particleQuad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	// Instance data (Offsets)
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	// We will update this every frame with posX and posY interleaved or just use two buffers
	// For simplicity, let's interleave them in a temporary buffer or use two separate attributes
	// Actually, let's use two separate attributes for posX and posY to avoid interleaving on CPU
	
	// Pre-allocate buffer for instance data (2 floats per particle: X and Y)
	glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 2 * sizeof(float), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glVertexAttribDivisor(1, 1); // This tells OpenGL this is an instanced attribute

	float lastFrame = 0.0f;
	std::vector<float> interleavedPos(MAX_PARTICLES * 2);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = (float)glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		processInput(window);

		// Update Particles
		ps.Update(deltaTime);

		// Prepare data for GPU
		const float* px = ps.GetPositionsX();
		const float* py = ps.GetPositionsY();
		for (int i = 0; i < MAX_PARTICLES; ++i) {
			interleavedPos[i * 2] = px[i];
			interleavedPos[i * 2 + 1] = py[i];
		}

		// Update instance buffer
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, MAX_PARTICLES * 2 * sizeof(float), interleavedPos.data());

		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui UI
		{
			ImGui::Begin("Particle Simulator Settings");
			ImGui::Text("Particle Count: %d", MAX_PARTICLES);
			ImGui::Separator();
			ImGui::SliderFloat("Gravity", &ps.Gravity, 0.0f, 20.0f);
			ImGui::SliderFloat("Friction", &ps.Friction, 0.9f, 1.0f);
			ImGui::SliderFloat("Bounciness", &ps.Bounciness, 0.0f, 1.0f);
			ImGui::SliderFloat("Particle Size", &ps.ParticleSize, 0.1f, 10.0f);
			ImGui::Checkbox("Use SIMD (AVX2)", &ps.UseSIMD);
			if (ImGui::Button("Reset Simulation")) ps.Reset();
			ImGui::Separator();
			ImGui::Text("Performance: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// Draw Particles
		particleShader.use();
		particleShader.setFloat("particleSize", ps.ParticleSize);
		
		// Projection matrix (simple 2D)
		glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
		particleShader.setMat4("projection", projection);

		glBindVertexArray(VAO);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MAX_PARTICLES);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		updateFPSCounter(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &instanceVBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void updateFPSCounter(GLFWwindow* window)
{
	static double previousTime = 0.0;
	static int frameCount = 0;
	double currentTime = glfwGetTime();
	frameCount++;
	if (currentTime - previousTime >= 1.0) {
		std::ostringstream ss;
		ss << "OpenGL FPS: " << frameCount;
		glfwSetWindowTitle(window, ss.str().c_str());
		frameCount = 0;
		previousTime = currentTime;
	}
}
