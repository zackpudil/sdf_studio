#include "environment.h"

#include <fstream>
#include <streambuf>
#include <sstream>
#include <glm/glm.hpp>
#include <glm\ext\matrix_clip_space.hpp>
#include <glm\ext\matrix_transform.hpp>

Environment::Environment() {
	cubeScreen = new Screen();
	quadScreen = new Screen();
	program = new Program();

	HdriTexture = new Texture();
	brdfTexture = new Texture();
	cubeMap = new Texture();
	irradianceMap = new Texture();
	prefilterMap = new Texture();

	std::ifstream vertStream(std::string(PROJECT_SOURCE_DIR "/shaders/model_vert.glsl"));
	cubeVertSource = std::string(std::istreambuf_iterator<char>(vertStream), std::istreambuf_iterator<char>());

	std::ifstream hdriStream(std::string(PROJECT_SOURCE_DIR "/shaders/cubemap/gen_from_hdri.glsl"));
	convertSource = std::string(std::istreambuf_iterator<char>(hdriStream), std::istreambuf_iterator<char>());

	std::ifstream irrStream(std::string(PROJECT_SOURCE_DIR "/shaders/cubemap/irradiance_convolution.glsl"));
	irradianceSource = std::string(std::istreambuf_iterator<char>(irrStream), std::istreambuf_iterator<char>());

	std::ifstream prefilterStream(std::string(PROJECT_SOURCE_DIR "/shaders/cubemap/prefilter.glsl"));
	prefilterSource = std::string(std::istreambuf_iterator<char>(prefilterStream), std::istreambuf_iterator<char>());

	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	hasEnvMap = false;
}

void Environment::SetHDRI(std::string filename) {

	if (filename.empty()) {
		HdriPath = "";
		HdriTexture->Allocate2D(1, 1, false);
	} else {
		HdriPath = filename;
		HdriTexture->LoadHDRIFromFile2D(HdriPath);
	}
	cubeMap->AllocateCube(1024, 1024, true);
	irradianceMap->AllocateCube(64, 64);
	prefilterMap->AllocateCube(512, 512, true);
	brdfTexture->Allocate2D();
	hasEnvMap = false;
}

void Environment::PreRender() {
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	cubeScreen->PrepareCube();

	convertHdriToCubeMap(captureProjection, captureViews);
	calcIrradianceCubeMap(captureProjection, captureViews);
	calcPrefilterCubeMap(captureProjection, captureViews);
	hasEnvMap = true;
}

void Environment::Use(Program *program, bool offline) {
	program->Bind("irr", irradianceMap->UseCube())
		.Bind("prefilter", prefilterMap->UseCube())
		.Bind("numberOfLights", (int)lights.size())
		.Bind("useIrr", UseIrradianceForBackground ? 1 : 0);

	if (offline) {
		program->Bind("hasEnvMap", hasEnvMap ? 1 : 0)
			.Bind("envExp", LightPathExposure);
	}


	for(int i = 0; i < lights.size(); i++) {
		program->Bind("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
		program->Bind("lights[" + std::to_string(i) + "].position", lights[i].position);
		program->Bind("lights[" + std::to_string(i) + "].color", lights[i].color);
		if (!offline) {
			program->Bind("lights[" + std::to_string(i) + "].hasShadow", lights[i].hasShadow ? 1 : 0);
			program->Bind("lights[" + std::to_string(i) + "].shadowPenumbra", lights[i].shadowPenumbra);
		}
	}
}

std::vector<Light>* Environment::GetLights() {
	return &lights;
}

void Environment::RemoveLight(int i) {
	lights.erase(lights.begin() + i);
}

void Environment::convertHdriToCubeMap(glm::mat4 captureProjection, glm::mat4 captureViews[6]) {
	
	program->Reload()
		.Attach(cubeVertSource, GL_VERTEX_SHADER)
		.Attach(convertSource, GL_FRAGMENT_SHADER)
		.Link()
		.Activate()
		.Bind("equirectangularMap", HdriTexture->Use2D())
		.Bind("projection", captureProjection);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

	glViewport(0, 0, 1024, 1024);
	
	for (unsigned int i = 0; i < 6; i++) {
		program->Bind("view", captureViews[i]);

		// TODO: Move this code to somewhere.
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap->TextureId, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cubeScreen->DrawCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	cubeMap->GenerateMipmap();

}

void Environment::calcIrradianceCubeMap(glm::mat4 captureProjection, glm::mat4 captureViews[6]) {
	program->Reload()
		.Attach(cubeVertSource, GL_VERTEX_SHADER)
		.Attach(irradianceSource, GL_FRAGMENT_SHADER)
		.Link()
		.Activate()
		.Bind("projection", captureProjection)
		.Bind("environmentMap", cubeMap->UseCube());

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 64, 64);
	glViewport(0, 0, 64, 64);
	
	for (unsigned int i = 0; i < 6; i++) {
		program->Bind("view", captureViews[i]);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap->TextureId, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cubeScreen->DrawCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Environment::calcPrefilterCubeMap(glm::mat4 captureProjection, glm::mat4 captureViews[6]) {
	prefilterMap->GenerateMipmap();

	program->Reload()
		.Attach(cubeVertSource, GL_VERTEX_SHADER)
		.Attach(prefilterSource, GL_FRAGMENT_SHADER)
		.Link()
		.Activate()
		.Bind("projection", captureProjection)
		.Bind("environmentMap", cubeMap->UseCube());

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (unsigned int mip = 0; mip < 5; ++mip) {
		GLuint mipWidth = 512 * std::pow(0.5, mip);
		GLuint mipHeight = 512 * std::pow(0.5, mip);

		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / 4.0;
		program->Bind("roughness", roughness);
		for (unsigned int i = 0; i < 6; i++) {
			program->Bind("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap->TextureId, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			cubeScreen->DrawCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}