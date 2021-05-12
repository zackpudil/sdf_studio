#include <vector>
#include <string>
#include <program.h>
#include <scene.h>
#include <TextEditor.h>

#pragma once

class SceneUI {
public:
	SceneUI();

	Scene* Scene;
	void Render();
	void HandleInput(GLFWwindow*);
	void UpdateText();
private:

	TextEditor* editor;
	bool hasBeenAlertedToError = false;
	int resScale = 0;

	void renderResolutionScaler();
	void renderDebugConfig();
	void renderCompileErrors();

	void renderUniforms();
	void renderMaterials();
};