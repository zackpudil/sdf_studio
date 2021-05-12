
#include <scene.h>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <algorithm>

Scene::Scene(Camera *c, Environment *e) : camera(c), environment(e) {
	renderProgram = new Program();
	displayProgram = new Program();

	offlineRenderProgram = new Program();
	offlineDisplayProgram = new Program();

	screen = new Screen();
	BrdfTexture = new Texture();
	mainImage = new Texture();
	offlineRender = new Texture();

	screen->PrepareQuad();

	vertSource = getShaderSource("quad_vert");
	rendererSource = getShaderSource("realtime_renderer");
	offlineRenderSource = getShaderSource("offline_renderer");
	brdfSource = getShaderSource("utils/precomputed_brdf");

	librarySources = {
		{ "<<NOISE>>", getShaderSource("library/noise") },
		{ "<<SDF_HELPERS>>", getShaderSource("library/sdf") },
		{ "<<RAY_TRACE>>", getShaderSource("library/ray_trace") },
		{ "<<MATERIALS>>", getShaderSource("library/materials") },
		{ "<<DEBUG>>", getShaderSource("library/debug") },
		{ "<<LIGHTING>>", getShaderSource("library/pbr_lighting") }
	};

	ready = false;
	renderBrdf();
	ResolutionScale = 0;
	UpdateResolution();
}

void Scene::Render() {
	if (ready && !Pause) {
		auto res = getResolution();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, res.x, res.y);
		glClear(GL_DEPTH_BUFFER_BIT);

		renderProgram->Activate()
			.Bind("resolution", res)
			.Bind("camera", camera->GetViewMatrix())
			.Bind("eye", camera->Position)
			.Bind("fov", camera->Fov)
			.Bind("exposure", camera->Exposure)
			.Bind("brdf", BrdfTexture->Use2D())
			.Bind("fudge", FudgeFactor)
			.Bind("maxDistance", MaxDistance)
			.Bind("maxIterations", MaxIterations)
			.Bind("useDebugPlane", UseDebugPlane ? 1 : 0)
			.Bind("debugPlaneHeight", DebugPlaneHeight)
			.Bind("showRayMarchAmount", ShowRayAmount ? 1 : 0);

		environment->Use(renderProgram);

		for (auto u : sceneUniforms) bindUniform(u, renderProgram);
		for (auto t : sceneMaterials) bindMaterial(t, renderProgram);
	}


	screen->DrawQuad();
}

void Scene::OfflineRender() {
	if (ready && !Pause) {
		auto res = getResolution();
		glBindFramebuffer(GL_FRAMEBUFFER, offlineFbo);
		glViewport(0, 0, res.x, res.y);
		glClear(GL_DEPTH_BUFFER_BIT);

		offlineRenderProgram->Activate()
			.Bind("resolution", res)
			.Bind("camera", camera->GetViewMatrix())
			.Bind("eye", camera->Position)
			.Bind("fov", camera->Fov)
			.Bind("fudge", FudgeFactor)
			.Bind("maxDistance", MaxDistance)
			.Bind("maxIterations", MaxIterations)
			.Bind("time", (float)glfwGetTime())
			.Bind("dof", camera->DepthOfField)
			.Bind("lastPass", offlineRender->Use2D())
			.Bind("shouldReset", camera->IsMoving ? 1 : 0);

		environment->Use(offlineRenderProgram, true);

		for (auto u : sceneUniforms) bindUniform(u, offlineRenderProgram);
		for (auto t : sceneMaterials) bindMaterial(t, offlineRenderProgram);

		screen->DrawQuad();
	}
}

void Scene::Display(int width, int height) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClear(GL_DEPTH_BUFFER_BIT);

	displayProgram->Activate().Bind("mainImage", mainImage->Use2D());

	screen->DrawQuad();
}

void Scene::OfflineDisplay(int width, int height) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClear(GL_DEPTH_BUFFER_BIT);

	offlineDisplayProgram->Activate()
		.Bind("lastPass", offlineRender->Use2D())
		.Bind("exposure", camera->Exposure);

	screen->DrawQuad();
}

bool Scene::SetShader(std::string source) {
	try {
		// TODO: Remove the refresh of the realtime_renderer once it is finalized.
		rendererSource = getShaderSource("realtime_renderer");
		offlineRenderSource = getShaderSource("offline_renderer");

		ShaderSource = source;
		UpdateResolution();

		getUniformsFromSource();
		return true;
	} catch (std::exception ex) {
		compileError = ex.what();
		ready = false;
	}

	return false;
}

bool Scene::InitShader() {
	try {
		std::string realTimeCode = updateSourceToCode(rendererSource);
		std::string offlineCode = updateSourceToCode(offlineRenderSource);

		renderProgram->Reload()
			.Attach(vertSource, GL_VERTEX_SHADER)
			.Attach(realTimeCode, GL_FRAGMENT_SHADER)
			.Link();

		offlineRenderProgram->Reload()
			.Attach(vertSource, GL_VERTEX_SHADER)
			.Attach(offlineCode, GL_FRAGMENT_SHADER)
			.Link();
		
		compileError.clear();
		ready = true;
		return true;
	} catch (std::exception ex) {
		compileError = ex.what();
		ready = false;
	}

	return false;
}

void Scene::SetEnvironment(std::string fileName) {
	environment->SetHDRI(fileName);
	environment->PreRender();
}

std::vector<SceneUniform>* Scene::GetUniforms() {
	return &sceneUniforms;
}

std::vector<SceneMaterial>* Scene::GetMaterials() {
	return &sceneMaterials;
}

std::string Scene::GetCompileError() {
	return compileError;
}

std::string Scene::GetUniformErrors() {
	return uniformErrors;
}

void Scene::UpdateResolution() {
	OfflineRenderAmounts = 0;
	auto res = getResolution();
	mainImage->Allocate2D(res.x, res.y, false);
	offlineRender->Allocate2D(res.x, res.y, false);

	auto source = getShaderSource("image_frag");
	displayProgram->Reload()
		.Attach(vertSource, GL_VERTEX_SHADER)
		.Attach(source, GL_FRAGMENT_SHADER)
		.Link();

	auto offlineDisplaySource = getShaderSource("offline_image");
	offlineDisplayProgram->Reload()
		.Attach(vertSource, GL_VERTEX_SHADER)
		.Attach(offlineDisplaySource, GL_FRAGMENT_SHADER)
		.Link();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainImage->TextureId, 0);

	glGenFramebuffers(1, &offlineFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, offlineFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, offlineRender->TextureId, 0);

}


void Scene::renderBrdf() {
	BrdfTexture->Allocate2D();

	renderProgram->Reload()
		.Attach(vertSource, GL_VERTEX_SHADER)
		.Attach(brdfSource, GL_FRAGMENT_SHADER)
		.Link()
		.Activate();


	glGenFramebuffers(1, &fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BrdfTexture->TextureId, 0);

	glViewport(0, 0, 512, 512);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	screen->DrawQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(1, &fbo);
	renderProgram->Reload();

}

void Scene::bindUniform(SceneUniform uniform, Program *program) {
	switch (uniform.type) {
	case UniformType::Int:
		program->Bind(uniform.name, uniform.valuesi[0]);
		break;
	case UniformType::Float:
		program->Bind(uniform.name, uniform.valuesf[0]);
		break;
	case UniformType::Vector2:
		program->Bind(uniform.name, glm::vec2(uniform.valuesf[0], uniform.valuesf[1]));
		break;
	case UniformType::Vector3:
		program->Bind(uniform.name, glm::vec3(uniform.valuesf[0], uniform.valuesf[1], uniform.valuesf[2]));
		break;
	case UniformType::Vector4:
		program->Bind(uniform.name, glm::vec4(uniform.valuesf[0], uniform.valuesf[1], uniform.valuesf[2], uniform.valuesf[3]));
		break;
	default:
		break;
	}
}

void Scene::bindMaterial(SceneMaterial texture, Program *program) {
	if (texture.albedo->TextureId > 0) program->Bind(texture.name + ".albedo", texture.albedo->Use2D());
	if (texture.roughness->TextureId > 0) program->Bind(texture.name + ".roughness", texture.roughness->Use2D());
	if (texture.metal->TextureId > 0) program->Bind(texture.name + ".metal", texture.metal->Use2D());
	if (texture.normal->TextureId > 0) program->Bind(texture.name + ".normal", texture.normal->Use2D());
	if (texture.ambientOcclusion->TextureId > 0) program->Bind(texture.name + ".ambientOcclusion", texture.ambientOcclusion->Use2D());
	if (texture.height->TextureId > 0) program->Bind(texture.name + ".height", texture.height->Use2D());
}

void Scene::getUniformsFromSource() {
	
	std::string line;
	std::istringstream sourceStream(ShaderSource);

	std::vector<std::string> namesInSource;
	while (std::getline(sourceStream, line)) {
		if (line.find("SceneUniform") != std::string::npos) {
			char typeName[25], name[25];
			typeName[0] = '\0';
			name[0] = '\0';
			float min = -10.0, max = 10.0;

			int res = sscanf(line.c_str(), "uniform %s %[^;]; // SceneUniform %f,%f", typeName, name, &min, &max);

			if (res < 4) {
				throw std::exception("Was not able to properly parse uniform variable\n");
			}

			namesInSource.push_back(name);

			auto findUniform = std::find_if(
				sceneUniforms.begin(),
				sceneUniforms.end(),
				[=](SceneUniform& s) {
					if (s.name.compare(name) == 0) {
						s.min = min;
						s.max = max;
						return true;
					}
					return false;
				}
			);

			if (findUniform == sceneUniforms.end()) {
				sceneUniforms.push_back(createUniform(std::string(typeName), std::string(name), min, max));
			}
		}
	}

	sceneUniforms.erase(
		std::remove_if(sceneUniforms.begin(), sceneUniforms.end(), [&](const SceneUniform s) {
			auto findInSource = std::find_if(
				namesInSource.begin(),
				namesInSource.end(),
				[=](const std::string name) {
					return s.name.compare(name) == 0; 
				}
			);

			return findInSource == namesInSource.end();
		}),
		sceneUniforms.end()
	);
}

std::string Scene::addMaterialsToCode() {
	std::string textureUniform = "";
	for (auto texture : sceneMaterials) {
		textureUniform += "uniform PBRTexture " + texture.name + ";\n";
	}

	return textureUniform;
}

SceneUniform Scene::createUniform(std::string typeName, std::string name, float min, float max) {
	UniformType type = UniformType::Float;

	if (typeName.compare("int") == 0) type = UniformType::Int;
	else if (typeName.compare("float") == 0) type = UniformType::Float;
	else if (typeName.compare("vec2") == 0) type = UniformType::Vector2;
	else if (typeName.compare("vec3") == 0) type = UniformType::Vector3;
	else if (typeName.compare("vec4") == 0) type = UniformType::Vector4;
	else type = UniformType::Matrix4;

	SceneUniform s = {
		name,
		type,
		min,
		max
	};

	return s;
}

glm::vec2 Scene::getResolution() {
	if (ResolutionScale == 0) return glm::vec2(1920, 1080);
	else if (ResolutionScale == 1) return glm::vec2(1600, 900);
	else if (ResolutionScale == 2) return glm::vec2(1526, 864);
	else if (ResolutionScale == 3) return glm::vec2(1280, 720);
	else if (ResolutionScale == 4) return glm::vec2(640, 360);
}

std::string Scene::getShaderSource(std::string shaderPath) {
	std::ifstream fragStream(std::string(PROJECT_SOURCE_DIR "/shaders/" + shaderPath + ".glsl"));
	return std::string(std::istreambuf_iterator<char>(fragStream), std::istreambuf_iterator<char>());
}

std::string Scene::updateSourceToCode(std::string code) {
	std::string newCode = code;
	auto start_pos = newCode.find("<<USER_CODE>>");
	newCode.replace(start_pos, 13, ShaderSource);

	start_pos = newCode.find("<<TEXTURES>>");
	newCode.replace(start_pos, 12, addMaterialsToCode());

	for (auto const& x : librarySources) {
		auto libStartPos = newCode.find(x.first);
		if (libStartPos != std::string::npos) {
			newCode.replace(libStartPos, x.first.length(), x.second);
		}
	}

	return newCode;
}