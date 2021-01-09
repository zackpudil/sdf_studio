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

	std::string GetCompileError();
	std::string GetUniformErrors();

	std::vector<SceneUniform>* GetUniforms();
	std::vector<SceneMaterial>* GetMaterials();

	Texture* BrdfTexture;
	float DebugPlaneHeight = -10.0f;
	float FudgeFactor = 1.0f;
private:
	Program* program;
	Screen* screen;
	Camera* camera;
	Environment* environment;
	
	std::vector<SceneUniform> sceneUniforms;
	std::vector<SceneMaterial> sceneMaterials;

	std::string compileError;
	std::string uniformErrors;

	std::string vertSource;
	std::string fragSource;
	std::string brdfSource;
	std::string shaderSource;

	GLuint fbo, rbo;

	bool ready;

	void renderBrdf();
	void bindUniform(SceneUniform);
	void bindMaterial(SceneMaterial);

	void getUniformsFromSource();
	std::string addMaterialsToCode();

	SceneUniform createUniform(std::string, std::string, float, float);
};