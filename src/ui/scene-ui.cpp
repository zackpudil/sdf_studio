
#include "scene-ui.h"
#include <ranges>
#include <sstream>
#include <algorithm>
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <istream>
#include <fstream>
#include <algorithm>

SceneUI::SceneUI() {
	editor = new TextEditor();
	editor->SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
}

void SceneUI::HandleInput(GLFWwindow* window) {
	auto isCTLKeyDown = [&](int key) {
		return glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) && glfwGetKey(window, key);
	};

	auto isSHFTCTLKeyDown = [&](int key) {
		return glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)
			&& glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)
			&& glfwGetKey(window, key);
	};

	if (glfwGetKey(window, GLFW_KEY_F5) || isCTLKeyDown(GLFW_KEY_R)) {
		auto source = editor->GetText();
		Scene->SetShader(source);
		Scene->InitShader();
		hasBeenAlertedToError = false;
	}

	if (isCTLKeyDown(GLFW_KEY_P)) Scene->Pause = true;
	if (isSHFTCTLKeyDown(GLFW_KEY_P)) Scene->Pause = false;

	for (int i = 0; i <= 4; i++) {
		if (isCTLKeyDown(GLFW_KEY_0 + i)) {
			resScale = i;
			Scene->ResolutionScale = i;
			Scene->UpdateResolution();
		}
	}
}

void SceneUI::Render(bool editable) {
	ImGui::Begin("Config");

	renderResolutionScaler();
	ImGui::Separator();

	renderDebugConfig();
	ImGui::Separator();

	renderCompileErrors();

	ImGui::End();

	if (!Scene->ShaderSource.empty()) {
		ImGui::Begin("GLSL");

		renderMaterials();
		ImGui::Separator();

		renderUniforms();
		ImGui::Separator();

		editor->Render("Code");
		editor->SetReadOnly(!editable);
		ImGui::End();
	}
}

void SceneUI::UpdateText() {
	editor->SetText(Scene->ShaderSource);
	resScale = Scene->ResolutionScale;
}

void SceneUI::renderResolutionScaler() {
	if (ImGui::CollapsingHeader("Resolution")) {
		ImGui::SliderInt("Resolution Scale", &resScale, 0, 4);
		if (ImGui::Button("Update")) {
			Scene->ResolutionScale = resScale;
			Scene->UpdateResolution();
		}
		ImGui::Checkbox("Pause", &Scene->Pause);
	}
}

void SceneUI::renderDebugConfig() {
	if (ImGui::CollapsingHeader("Debug")) {
		ImGui::SliderFloat("Fudge Factor", &Scene->FudgeFactor, 0.25f, 1.0f);
		ImGui::SliderFloat("Max Distance", &Scene->MaxDistance, 10.0f, 100.0f);
		ImGui::SliderInt("Max Iterations", &Scene->MaxIterations, 100, 500);
		ImGui::Checkbox("Show Debug Plane", &Scene->UseDebugPlane);

		if (Scene->UseDebugPlane)
			ImGui::SliderFloat("Debug Plane", &Scene->DebugPlaneHeight, -10.0f, 10.0f);

		ImGui::Checkbox("Show Raymarch Amount", &Scene->ShowRayAmount);
	}
}

void SceneUI::renderCompileErrors() {
	if (!Scene->GetCompileError().empty() && !hasBeenAlertedToError) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), Scene->GetCompileError().c_str());
	}
}



void SceneUI::renderUniforms() {
	if(ImGui::CollapsingHeader("Uniforms")) {
		for (auto& v : *Scene->GetUniforms()) {
			auto name = v.name.c_str();
			switch (v.type) {
			case UniformType::Int:
				if (ImGui::TreeNode(name)) {
					ImGui::InputInt(name, v.valuesi);
					ImGui::TreePop();
				}
				break;
			case UniformType::Float:
				if (ImGui::TreeNode(name)) {
					ImGui::SliderFloat(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
					ImGui::InputFloat(std::string("Input###input" + v.name).c_str(), v.valuesf);
					ImGui::TreePop();
				}
				break;
			case UniformType::Vector2:
				if (ImGui::TreeNode(name)) {
					ImGui::SliderFloat2(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
					ImGui::InputFloat2(std::string("Input###input" + v.name).c_str(), v.valuesf);
					ImGui::TreePop();
				}
				break;
			case UniformType::Vector3:
				if (ImGui::TreeNode(name)) {
					ImGui::SliderFloat3(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
					ImGui::InputFloat3(std::string("Input###input" + v.name).c_str(), v.valuesf);
					ImGui::TreePop();
				}
				break;
			case UniformType::Vector4:
				if (ImGui::TreeNode(name)) {
					ImGui::SliderFloat4(std::string("Slide###slide" + v.name).c_str(), v.valuesf, v.min, v.max);
					ImGui::InputFloat4(std::string("Input###input" + v.name).c_str(), v.valuesf);
					ImGui::TreePop();
				}
				break;
			default:
				ImGui::Text("Huh?");
			}
		}
	}
}

void SceneUI::renderMaterials() {
	
	static char newMaterialName[25] = "";
	if (ImGui::CollapsingHeader("Materials")) {
		int i = 0;
		for (auto material : *Scene->GetMaterials()) {
			if (ImGui::TreeNode(material.name.c_str())) {
				
				if (material.albedo->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.albedo->TextureId, ImVec2(50, 50));
					ImGui::SameLine();
				}

				if (material.roughness->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.roughness->TextureId, ImVec2(50, 50));
					ImGui::SameLine();
				}

				if (material.metal->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.metal->TextureId, ImVec2(50, 50));
					ImGui::SameLine();
				}

				if (material.normal->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.normal->TextureId, ImVec2(50, 50));
					ImGui::SameLine();
				}

				if (material.ambientOcclusion->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.ambientOcclusion->TextureId, ImVec2(50, 50));
					ImGui::SameLine();
				}

				if (material.height->TextureId > 0) {
					ImGui::Image((void*)(intptr_t)material.height->TextureId, ImVec2(50, 50));
				}

				ImGui::TreePop();
				ImGui::SameLine();
				if (ImGui::Button(std::string("Delete##" + std::to_string(i)).c_str(), ImVec2(100, 50))) {
					Scene->GetMaterials()->erase(std::remove(Scene->GetMaterials()->begin(), Scene->GetMaterials()->end(), material));
				}
				i++;
			}
		}
		ImGui::InputText("Name##material", newMaterialName, 25);

		if (!std::string(newMaterialName).empty()) {
			if (ImGui::Button("Add Materials")) {
				igfd::ImGuiFileDialog::Instance()->OpenModal("ChooseFileDlgKey3", "Open Files", ".png,.jpg,.jpeg", "", 6);
			}
		}
	}
	
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

			const auto isType = [](std::string name, std::string type) {
				std::string lowerName = name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
				return lowerName.find(type) != std::string::npos;
			};

			for (auto const& file : files) {
				if (isType(file.first, "_albedo")) {
					material.albedo->LoadFromFile2D(file.second);
					material.albedoPath = file.second;
				} else if (isType(file.first, "_rough")) {
					material.roughness->LoadFromFile2D(file.second);
					material.roughnessPath = file.second;
				}  else if (isType(file.first, "_metal")) {
					material.metal->LoadFromFile2D(file.second);
					material.metalPath = file.second;
				} else if (isType(file.first, "_normal")) {
					material.normal->LoadFromFile2D(file.second);
					material.normalPath = file.second;
				} else if (isType(file.first, "_ao")) {
					material.ambientOcclusion->LoadFromFile2D(file.second);
					material.ambientOcclusionPath = file.second;
				} else if (isType(file.first, "_height")) {
					material.height->LoadFromFile2D(file.second);
					material.heightPath = file.second;
				}
			}

			Scene->GetMaterials()->push_back(material);
			newMaterialName[0] = '\0';
		}

		igfd::ImGuiFileDialog::Instance()->CloseDialog("ChooseFileDlgKey3");
	}
}
