#include <program.h>
#include <vector>
#include <camera.h>
#include <screen.h>
#include <environment.h>
#include <map>

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
	void OfflineRender();

	void Display(int, int);
	void OfflineDisplay(int, int);

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
	int OfflineRenderAmounts = 0;
private:
	Program* renderProgram;
	Program* displayProgram;
	Program* offlineRenderProgram;
	Program* offlineDisplayProgram;

	Screen* screen;
	Camera* camera;
	Environment* environment;

	Texture* mainImage;
	Texture* offlineRender;
	
	std::vector<SceneUniform> sceneUniforms;
	std::vector<SceneMaterial> sceneMaterials;

	std::string compileError;
	std::string uniformErrors;

	std::string vertSource;
	std::string rendererSource;
	std::string offlineRenderSource;
	std::string brdfSource;

	std::map<std::string, std::string> librarySources;

	GLuint fbo, offlineFbo;

	bool ready;

	void renderBrdf();

	void bindUniform(SceneUniform, Program *);
	void bindMaterial(SceneMaterial, Program *);

	void getUniformsFromSource();
	std::string addMaterialsToCode();

	SceneUniform createUniform(std::string, std::string, float, float);

	glm::vec2 getResolution();

	std::string getShaderSource(std::string);

	std::string updateSourceToCode(std::string);
};