
#include <stdlib.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends\imgui_impl_opengl3.h>
#include <backends\imgui_impl_glfw.h>
#include <ui/scene-ui.h>
#include <screen.h>
#include <glm/glm.hpp>
#include <environment.h>
#include <ui/camera-ui.h>
#include <ui/environment-ui.h>
#include <ui\stats-ui.h>

#include "main.h"
#include <project.h>
#include <ImGuiFileDialog.h>
#include <ui\project-ui.h>

int main() {

	if (!glfwInit()) {
		std::cout << "Unable to initialize Window\n";
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "SDF Studio", NULL, NULL);
	glfwMakeContextCurrent(window);

	if (window == nullptr) {
		std::cout << "Unable to create window\n";
		exit(1);
	}

	if (!gladLoadGL()) {
		std::cout << "FUUUCK, fucking glad fucking sucks.";
		exit(1);
	}

	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Failed to load opengl " << glGetError() << std::endl;
		exit(1);
	}

	std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui_ImplGlfw_InitForOpenGL(window, true);

	Project project;

	project.NewScene();

	CameraUI cameraUI;
	cameraUI.Camera = project.ProjectCamera;

	SceneUI sceneUI;
	sceneUI.Scene = project.ProjectScene;
	sceneUI.UpdateText();

	EnvironmentUI environmentUI;
	environmentUI.Environment = project.ProjectEnvironment;

	StatsUI statsUI;
	ProjectUI projectUI(&project, &sceneUI, &environmentUI, &cameraUI);

	while (!glfwWindowShouldClose(window)) {
		if (project.ProjectCamera->IsMoving || statsUI.KeepRunning || (projectUI.Offline && !project.ProjectScene->Pause)) {
			glfwPollEvents();
		} else {
			glfwWaitEvents();
		}

		project.ProjectCamera->HandleInput(window);
		sceneUI.HandleInput(window);

		if (projectUI.Offline) {
			project.ProjectScene->OfflineRender();
			project.ProjectScene->OfflineDisplay(WIDTH, HEIGHT);
		}
		else {
			project.ProjectScene->Render();
			project.ProjectScene->Display(WIDTH, HEIGHT);
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		cameraUI.Render();
		environmentUI.Render();
		sceneUI.Render();
		statsUI.Render();

		projectUI.Render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();

	return 0;
}