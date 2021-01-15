
#include "scene-ui.h"
#include <ranges>
#include <sstream>
#include <algorithm>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <istream>
#include <fstream>
#include <algorithm>

SceneUI::SceneUI(Scene* scene) : scene(scene) {
	sceneFilePath = "";
	editor = new TextEditor();
	editor->SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
}

void SceneUI::Render() {
	ImGui::Begin("Scene");

	if (ImGui::CollapsingHeader("Debug")) {
		ImGui::SliderFloat("Fudge Factor", &scene->FudgeFactor, 0.25f, 1.0f);
	}

	ImGui::Separator();

	renderUniforms();
	ImGui::Separator();

	renderSceneChooser();
	ImGui::Separator();

	ImGui::End();

	if (!sceneFilePath.empty()) {
		renderCodeAndMaterials();
	}
}

void SceneUI::HandleInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_F5)) {
		auto source = editor->GetText();
		scene->SetShader(source);
		scene->InitShader();
		hasBeenAlertedToError = false;
	}
}

void SceneUI::renderUniforms() {
	for (auto& v : *scene->GetUniforms()) {
		auto name = v.name.c_str();
		switch (v.type) {
		case UniformType::Int:
			if (ImGui::CollapsingHeader(name)) {
				ImGui::InputInt(name, v.valuei);
			}
			break;
		case UniformType::Float:
			if (ImGui::CollapsingHeader(name)) {
				ImGui::SliderFloat(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
				ImGui::InputFloat(std::string("Input###input" + v.name).c_str(), v.valuesf);
			}
			break;
		case UniformType::Vector2:
			if (ImGui::CollapsingHeader(name)) {
				ImGui::SliderFloat2(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
				ImGui::InputFloat2(std::string("Input###input" + v.name).c_str(), v.valuesf);
			}
			break;
		case UniformType::Vector3:
			if (ImGui::CollapsingHeader(name)) {
				ImGui::SliderFloat3(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
				ImGui::InputFloat3(std::string("Input###input" + v.name).c_str(), v.valuesf);
			}
			break;
		case UniformType::Vector4:
			if (ImGui::CollapsingHeader(name)) {
				ImGui::SliderFloat4(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
				ImGui::InputFloat4(std::string("Input###input" + v.name).c_str(), v.valuesf);
			}
			break;
		default:
			ImGui::Text("Huh?");
		}
		ImGui::SameLine();

		ImGui::Separator();
	}
}

void SceneUI::renderSceneChooser() {
	if (sceneFilePath.empty()) {
		ImGui::Text("No File Selected");
	}
	else {
		ImGui::Text(sceneFilePath.c_str());
	}

	ImGui::SameLine();
	if (ImGui::SmallButton("Open File")) {
		igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey", "Choose a File", ".glsl,.gl,.vert,.frag", "");
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Reload")) {
		std::ifstream shaderStream(sceneFilePath);
		auto shaderSource = std::string(std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>());

		scene->SetShader(shaderSource);
		scene->InitShader();
		hasBeenAlertedToError = false;
	}

	if (igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(800, 400), ImVec2(800, 400))) {
		if (igfd::ImGuiFileDialog::Instance()->IsOk) {
			sceneFilePath = igfd::ImGuiFileDialog::Instance()->GetFilePathName();
			std::ifstream shaderStream(sceneFilePath);
			auto shaderSource = std::string(std::istreambuf_iterator<char>(shaderStream), std::istreambuf_iterator<char>());

			editor->SetText(shaderSource);
			scene->SetShader(shaderSource);
			scene->InitShader();
			hasBeenAlertedToError = false;
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey");
	}

	if (!scene->GetCompileError().empty() && !hasBeenAlertedToError) {
		ImGui::OpenPopup("Compile/Link Error");
	}

	if (ImGui::BeginPopup("Compile/Link Error")) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), scene->GetCompileError().c_str());

		if (ImGui::Button("Close")) {
			hasBeenAlertedToError = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void SceneUI::renderCodeAndMaterials() {
	ImGui::Begin("GLSL");

	static char newMaterialName[25] = "";
	if (ImGui::CollapsingHeader("Materials")) {
		for (auto material : *scene->GetMaterials()) {
			if (ImGui::TreeNode(material.name.c_str())) {

				ImGui::Image((void*)(intptr_t)material.albedo->TextureId, ImVec2(50, 50));
				ImGui::SameLine();

				ImGui::Image((void*)(intptr_t)material.roughness->TextureId, ImVec2(50, 50));
				ImGui::SameLine();

				ImGui::Image((void*)(intptr_t)material.metal->TextureId, ImVec2(50, 50));
				ImGui::SameLine();

				ImGui::Image((void*)(intptr_t)material.normal->TextureId, ImVec2(50, 50));
				ImGui::SameLine();

				ImGui::Image((void*)(intptr_t)material.ambientOcclusion->TextureId, ImVec2(50, 50));
				ImGui::SameLine();

				ImGui::Image((void*)(intptr_t)material.height->TextureId, ImVec2(50, 50));

				ImGui::TreePop();
			}
		}
		ImGui::InputText("Name##material", newMaterialName, 25);

		if (!std::string(newMaterialName).empty()) {
			if (ImGui::Button("Add Materials")) {
				igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey3", "Open Files", ".png,.jpg,.jpeg", "", 6);
			}
		}
	}

	editor->Render("Code");
	ImGui::End();

	if (igfd::ImGuiFileDialog::Instance()->FileDialog("ChooseFileDlgKey3", ImGuiWindowFlags_NoCollapse, ImVec2(800, 400), ImVec2(800, 400))) {
		if (igfd::ImGuiFileDialog::Instance()->IsOk) {
			auto files = igfd::ImGuiFileDialog::Instance()->GetSelection();

			SceneMaterial material;
			material.name = std::string(newMaterialName);
			material.albedo = new Texture();
			material.roughness = new Texture();
			material.metal = new Texture();
			material.normal = new Texture();
			material.ambientOcclusion = new Texture();
			material.height = new Texture();

			auto isType = [](std::string name, std::string type) {
				std::string lowerName = name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
				return lowerName.find(type) != std::string::npos;
			};

			for (auto const& file : files) {
				if (isType(file.first, "_albedo")) material.albedo->LoadFromFile2D(file.second);
				else if (isType(file.first, "_rough")) material.roughness->LoadFromFile2D(file.second);
				else if (isType(file.first, "_metal")) material.metal->LoadFromFile2D(file.second);
				else if (isType(file.first, "_normal")) material.normal->LoadFromFile2D(file.second);
				else if (isType(file.first, "_ao"))  material.ambientOcclusion->LoadFromFile2D(file.second);
				else if (isType(file.first, "_height")) material.height->LoadFromFile2D(file.second);
			}

			scene->GetMaterials()->push_back(material);
			newMaterialName[0] = '\0';
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey3");
	}
}
