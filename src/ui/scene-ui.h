#include <vector>
#include <string>
#include <program.h>
#include <scene.h>
#include <TextEditor.h>

#pragma once

class SceneUI {
public:
	SceneUI(Scene *);

	std::string sceneFilePath;
	void Render();
	void HandleInput(GLFWwindow*);
private:
	Scene* scene;
	TextEditor* editor;

	bool hasBeenAlertedToError = false;

	void renderUniforms();
	void renderSceneChooser();
	void renderCodeAndMaterials();
};