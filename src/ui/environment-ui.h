#include <environment.h>

class EnvironmentUI {
public:
	EnvironmentUI(Environment*);

	void Render();
private:
	Environment* environment;
	std::string filePath;

	int newLightType;
	float newLightPosition[3] = { 0.0f, 0.0f, 0.0f };
	float newLightColor[3] = { 0.0f, 0.0f, 0.0f };
};