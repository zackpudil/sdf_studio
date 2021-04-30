#include <scene.h>
#pragma once

class Project {
public:
	Scene* ProjectScene;
	Environment* ProjectEnvironment;
	Camera* ProjectCamera;

	std::string SavePath;

	void NewScene();
	bool SaveScene();
	void LoadScene(std::string);
};