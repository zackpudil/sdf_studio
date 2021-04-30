#include <program.h>
#include <vector>
#include <camera.h>
#include <screen.h>
#include <environment.h>

#pragma once

enum class UniformType {
	Int = 0,
	Float,
	Vector2,
	Vector3,
	Vector4,
	Matrix2,
	Matrix3,
	Matrix4
};

struct SceneUniform {
	std::string name;
	UniformType type;

	float min;
	float max;

	int valuesi[16];
	float valuesf[16];
};

struct SceneMaterial {
	std::string name;

	std::string albedoPath;
	Texture* albedo;

	std::string roughnessPath;
	Texture* roughness;

	std::string metalPath;
	Texture* metal;

	std::string normalPath;
	Texture* normal;

	std::string ambientOcclusionPath;
	Texture* ambientOcclusion;

	std::string heightPath;
	Texture* height;
};

class Scene {
public:
	Scene(Camera *, Environment *);

	bool SetShader(std::string);
	void SetEnvironment(std::string);

	bool InitShader();
	void Render();
	void Display();

	std::string GetCompileError();
	std::string GetUniformErrors();

	std::vector<SceneUniform>* GetUniforms();
	std::vector<SceneMaterial>* GetMaterials();

	void UpdateResolution();

	Texture* BrdfTexture;
	std::string ShaderSource = "";

	float DebugPlaneHeight = -10.0f;
	bool UseDebugPlane = false;
	float FudgeFactor = 1.0f;
	float MaxDistance = 50.0f;
	int ResolutionScale;
	int MaxIterations = 300;
	bool ShowRayAmount = false;
	bool Pause = false;
private:
	Program* renderProgram;
	Program* displayProgram;

	Screen* screen;
	Camera* camera;
	Environment* environment;

	Texture* mainImage;
	
	std::vector<SceneUniform> sceneUniforms;
	std::vector<SceneMaterial> sceneMaterials;

	std::string compileError;
	std::string uniformErrors;

	std::string vertSource;
	std::string rendererSource;
	std::string librarySource;
	std::string brdfSource;

	GLuint fbo, rbo;

	bool ready;

	void renderBrdf();

	void bindUniform(SceneUniform);
	void bindMaterial(SceneMaterial);

	void getUniformsFromSource();
	std::string addMaterialsToCode();

	SceneUniform createUniform(std::string, std::string, float, float);

	glm::vec2 getResolution();
};