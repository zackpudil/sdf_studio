
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


void checkPressedAndReleased(GLFWwindow*, int, bool*, bool*);

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

	bool showUI = true;

	bool hidePressed = false;
	bool renderPressed = false;
	bool pausePressed = false;

	while (!glfwWindowShouldClose(window)) {
		if (project.ProjectCamera->IsMoving || statsUI.KeepRunning || (projectUI.Offline && !project.ProjectScene->Pause)) {
			glfwPollEvents();
		} else {
			glfwWaitEvents();
		}

		checkPressedAndReleased(window, GLFW_KEY_F11, &hidePressed, &showUI);
		checkPressedAndReleased(window, GLFW_KEY_F10, &renderPressed, &projectUI.Offline);
		checkPressedAndReleased(window, GLFW_KEY_F9, &pausePressed, &(project.ProjectScene->Pause));


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

		if (showUI) {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			cameraUI.Render();
			environmentUI.Render();
			sceneUI.Render(!project.ProjectCamera->IsMoving);
			statsUI.Render();

			projectUI.Render();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();

	return 0;
}

void checkPressedAndReleased(GLFWwindow* window, int key, bool* pressed, bool* toggle) {
	if(glfwGetKey(window, key) == GLFW_PRESS) {
		*pressed = true;
	}

	if (glfwGetKey(window, key) == GLFW_RELEASE && *pressed) {
		*toggle = !(*toggle);
		*pressed = false;
	}
}