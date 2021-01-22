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

	int valuei[16];
	float valuesf[16];
};

struct SceneMaterial {
	std::string name;
	Texture* albedo;
	Texture* roughness;
	Texture* metal;
	Texture* normal;
	Texture* ambientOcclusion;
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

	void UpdateResolution(int resScale);

	Texture* BrdfTexture;
	float DebugPlaneHeight = -10.0f;
	bool UseDebugPlane = false;
	float FudgeFactor = 1.0f;
	float MaxDistance = 50.0f;
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

	std::string shaderSource;

	GLuint fbo, rbo;

	bool ready;
	int resolutionScale;

	void renderBrdf();

	void bindUniform(SceneUniform);
	void bindMaterial(SceneMaterial);

	void getUniformsFromSource();
	std::string addMaterialsToCode();

	SceneUniform createUniform(std::string, std::string, float, float);

	glm::vec2 getResolution();
};