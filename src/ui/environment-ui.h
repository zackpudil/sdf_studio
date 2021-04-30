#include <environment.h>
#pragma once

class EnvironmentUI {
public:
	void Render();

	Environment* Environment;
private:
	std::string filePath;

	int newLightType;
	float newLightPosition[3] = { 0.0f, 0.0f, 0.0f };
	float newLightColor[3] = { 0.0f, 0.0f, 0.0f };
};