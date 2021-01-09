#include "environment-ui.h"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <thread>
#include <glm\gtc\type_ptr.hpp>

EnvironmentUI::EnvironmentUI(Environment* e) : environment(e) {
}

void EnvironmentUI::Render() {
	ImGui::Begin("Environment");
	if (filePath.empty()) {
		ImGui::Text("No File Selected");
	}
	else {
		ImGui::Text(filePath.c_str());
	}

	ImGui::SameLine();
	if (ImGui::SmallButton("Open File###2")) {
		igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey2", "Choose a File", ".hdr", "");
	}


	if (igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey2", ImGuiWindowFlags_NoCollapse, ImVec2(800, 400), ImVec2(800, 400))) {
		if (igfd::ImGuiFileDialog::Instance()->IsOk) {
			filePath = igfd::ImGuiFileDialog::Instance()->GetFilePathName();
			environment->SetHDRI(filePath);
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey2");
	}

	if (!filePath.empty()) {
		ImGui::Image((void*)(intptr_t)environment->HdriTexture->TextureId, ImVec2(200, 100));

		if (ImGui::Button("Create IRR and PC maps")) {
			environment->PreRender();
		}
	}

	ImGui::Text("Lights");
	ImGui::Separator();

	const char* types[] = { "Sun", "Point" };
	
	int id = 0;
	int idToRemove = -1;
	for (auto& light : *environment->GetLights()) {
		auto idstr = std::to_string(id);
		const char* currentItem = light.type == LightType::Pointlight ? types[1] : types[0];

		if (ImGui::BeginCombo(std::string("Light Type###type" + idstr).c_str(), currentItem)) {
			for (int i = 0; i < IM_ARRAYSIZE(types); i++) {
				if (ImGui::Selectable(types[i], false)) {
					light.type = (LightType)i;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::InputFloat3(std::string("Position/Direction###position" + idstr).c_str(), &light.position.x);

		std::string popupId = "my_picker###picker" + idstr;

		ImVec4 colorForButton = ImVec4(light.color.x, light.color.y, light.color.z, 1.0);
		if (ImGui::ColorButton(std::string("Color###color" + idstr).c_str(), colorForButton, 0, ImVec2(50, 25))) {
			ImGui::OpenPopup(popupId.c_str());
		}

		if (ImGui::BeginPopup(popupId.c_str())) {
			ImGui::ColorPicker3(std::string("Color###picker_color" + idstr).c_str(), &light.color.x);
			ImGui::EndPopup();
		}

		ImGui::Checkbox(std::string("Cast Shadow###shadow" + idstr).c_str(), &light.hasShadow);

		if (light.hasShadow) {
			ImGui::SliderFloat(std::string("Shadow Penumbra###shadowPenumbra" + idstr).c_str(), &light.shadowPenumbra, 12.0f, 128.0f);
		}

		if (ImGui::Button(std::string("Remove##remove" + std::to_string(id)).c_str())) {
			idToRemove = id;
		}

		id++;
		ImGui::Separator();
	}

	if (idToRemove >= 0) environment->RemoveLight(idToRemove);

	if (ImGui::Button("Add Light")) {
		environment->GetLights()->push_back(Light{
			LightType::Sunlight,
			glm::vec3(0.0f, 1.0f, 0.0),
			glm::vec3(1.0f),
			false,
			32.0f
		});
	}

	ImGui::End();
}