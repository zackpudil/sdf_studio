#include <string>
#include <texture.h>
#include <screen.h>
#include <camera.h>
#include <vector>

#pragma once

enum class LightType {
	Sunlight = 0,
	Pointlight = 1
};

struct Light {
	LightType type;
	glm::vec3 position;
	glm::vec3 color;

	bool hasShadow;
	float shadowPenumbra;
};

class Environment {
public:
	Environment();

	void SetHDRI(std::string);
	void PreRender();
	void Use(Program *);

	void RemoveLight(int);
	std::vector<Light>* GetLights();

	Texture* HdriTexture;
private:
	Texture* cubeMap;
	Texture* irradianceMap;
	Texture* prefilterMap;
	Texture* brdfTexture;

	Screen* cubeScreen;
	Screen* quadScreen;
	Program* program;

	std::vector<Light> lights;

	GLuint fbo;
	GLuint rbo;

	std::string cubeVertSource;

	std::string convertSource;
	std::string irradianceSource;
	std::string prefilterSource;

	void convertHdriToCubeMap(glm::mat4, glm::mat4[6]);
	void calcIrradianceCubeMap(glm::mat4, glm::mat4[6]);
	void calcPrefilterCubeMap(glm::mat4, glm::mat4[6]);
};