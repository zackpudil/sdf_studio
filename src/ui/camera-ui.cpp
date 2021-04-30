#include "camera-ui.h"
#include <imgui.h>
#include <string>

void CameraUI::Render() {
	ImGui::Begin("Camera");

	ImGui::SliderFloat("FOV", &Camera->Fov, 0.5, 2.0);
	ImGui::SliderFloat("Speed", &Camera->Speed, 1.0, 5.0);
	ImGui::SliderFloat("Sensitivity", &Camera->Sensitivity, 0.1, 1.0);
	ImGui::SliderFloat("Exposure", &Camera->Exposure, 0.1, 2.0);

	if (ImGui::Button("Reset")) {
		Camera->Position = glm::vec3(0, 0, 3);
		Camera->Direction = glm::vec3(0, 0, -1);
	}

	ImGui::Text(std::string(std::to_string(Camera->Position.x) + "," + std::to_string(Camera->Position.y) + "," + std::to_string(Camera->Position.z)).c_str());
	ImGui::Text(std::string(std::to_string(Camera->Direction.x) + "," + std::to_string(Camera->Direction.y) + "," + std::to_string(Camera->Direction.z)).c_str());

	ImGui::End();
}
