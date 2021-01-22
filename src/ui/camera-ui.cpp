#include "camera-ui.h"
#include <imgui.h>
#include <string>

CameraUI::CameraUI(Camera *c) : camera(c) { }

void CameraUI::Render() {
	ImGui::Begin("Camera");

	ImGui::SliderFloat("FOV", &camera->Fov, 0.5, 2.0);
	ImGui::SliderFloat("Speed", &camera->Speed, 1.0, 5.0);
	ImGui::SliderFloat("Sensitivity", &camera->Sensitivity, 0.1, 1.0);
	ImGui::SliderFloat("Exposure", &camera->Exposure, 0.1, 2.0);

	if (ImGui::Button("Reset")) {
		camera->Position = glm::vec3(0, 0, 3);
		camera->Direction = glm::vec3(0, 0, -1);
	}

	ImGui::Text(std::string(std::to_string(camera->Position.x) + "," + std::to_string(camera->Position.y) + "," + std::to_string(camera->Position.z)).c_str());
	ImGui::Text(std::string(std::to_string(camera->Direction.x) + "," + std::to_string(camera->Direction.y) + "," + std::to_string(camera->Direction.z)).c_str());

	ImGui::End();
}
