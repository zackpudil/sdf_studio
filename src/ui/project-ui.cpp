#include "project-ui.h"
#include <imgui.cpp>
#include <ImGuiFileDialog.h>

ProjectUI::ProjectUI(Project* p, SceneUI* s, EnvironmentUI* e, CameraUI* c) :
	project(p), sceneUI(s), environmentUI(e), cameraUI(c)
{
}

void ProjectUI::Render() {
	ImGui::Begin("Project");
	if (ImGui::Button("Save")) {
		if (!project->SaveScene()) {
			igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey4", "Open Files", ".txt", "");
		}
	}

	if (igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey4", ImGuiWindowFlags_NoCollapse, ImVec2(800, 400), ImVec2(800, 400))) {
		if (igfd::ImGuiFileDialog::Instance()->IsOk) {
			project->SavePath = igfd::ImGuiFileDialog::Instance()->GetFilePathName();
			project->SaveScene();
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey4");
	}

	ImGui::SameLine();
	if (ImGui::Button("Open")) {
		igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey5", "Open Files", ".txt", "");
	}

	if (igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey5", ImGuiWindowFlags_NoCollapse, ImVec2(800, 400), ImVec2(800, 400))) {
		if (igfd::ImGuiFileDialog::Instance()->IsOk) {
			project->LoadScene(igfd::ImGuiFileDialog::Instance()->GetFilePathName());
			sceneUI->Scene = project->ProjectScene;
			environmentUI->Environment = project->ProjectEnvironment;
			cameraUI->Camera = project->ProjectCamera;
			sceneUI->UpdateText();
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey5");
	}

	ImGui::SameLine();
	if (ImGui::Button("New")) {
		project->NewScene();
		sceneUI->Scene = project->ProjectScene;
		environmentUI->Environment = project->ProjectEnvironment;
		cameraUI->Camera = project->ProjectCamera;
		sceneUI->UpdateText();
	}

	if (!project->SavePath.empty()) {
		ImGui::Text(project->SavePath.c_str());
	}
	ImGui::End();
}