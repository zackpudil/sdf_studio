
#include <scene.h>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <algorithm>

Scene::Scene(Camera *c, Environment *e) : camera(c), environment(e) {
	program = new Program();
	screen = new Screen();
	BrdfTexture = new Texture();

	screen->PrepareQuad();

	std::ifstream vertStream(std::string(PROJECT_SOURCE_DIR "/shaders/quad_vert.glsl"));
	vertSource = std::string(std::istreambuf_iterator<char>(vertStream), std::istreambuf_iterator<char>());

	std::ifstream fragStream(std::string(PROJECT_SOURCE_DIR "/shaders/realtime_renderer.glsl"));
	rendererSource = std::string(std::istreambuf_iterator<char>(fragStream), std::istreambuf_iterator<char>());

	std::ifstream hdfLibraryStream(std::string(PROJECT_SOURCE_DIR "/shaders/hg_sdf.glsl"));
	librarySource = std::string(std::istreambuf_iterator<char>(hdfLibraryStream), std::istreambuf_iterator<char>());

	std::ifstream brdfStream(std::string(PROJECT_SOURCE_DIR "/shaders/utils/precomputed_brdf.glsl"));
	brdfSource = std::string(std::istreambuf_iterator<char>(brdfStream), std::istreambuf_iterator<char>());

	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);

	ready = false;
	renderBrdf();
}

void Scene::Render() {
	if (ready) {
		program->Activate()
			.Bind("resolution", glm::vec2(1920, 1080))
			.Bind("camera", camera->GetViewMatrix())
			.Bind("eye", camera->Position)
			.Bind("fov", camera->Fov)
			.Bind("exposure", camera->Exposure)
			.Bind("fudge", FudgeFactor)
			.Bind("brdf", BrdfTexture->Use2D());

		environment->Use(program);

		for (auto u : sceneUniforms) bindUniform(u);
		for (auto t : sceneMaterials) bindMaterial(t);

	}

	screen->DrawQuad();
}

bool Scene::SetShader(std::string source) {
	try {
		std::ifstream fragStream(std::string(PROJECT_SOURCE_DIR "/shaders/realtime_renderer.glsl"));
		rendererSource = std::string(std::istreambuf_iterator<char>(fragStream), std::istreambuf_iterator<char>());

		shaderSource = source;

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
		std::string code = rendererSource;
		auto start_pos = code.find("<<HERE>>");
		code.replace(start_pos, 8, shaderSource);

		start_pos = code.find("<<SDF_HELPERS>>");
		code.replace(start_pos, 15, librarySource);

		start_pos = code.find("<<TEXTURES>>");
		code.replace(start_pos, 12, addMaterialsToCode());

		program->Reload()
			.Attach(vertSource, GL_VERTEX_SHADER)
			.Attach(code, GL_FRAGMENT_SHADER)
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

void Scene::renderBrdf() {
	BrdfTexture->Allocate2D();

	program->Reload()
		.Attach(vertSource, GL_VERTEX_SHADER)
		.Attach(brdfSource, GL_FRAGMENT_SHADER)
		.Link()
		.Activate();

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BrdfTexture->TextureId, 0);

	glViewport(0, 0, 512, 512);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	screen->DrawQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	program->Reload();

}

void Scene::bindUniform(SceneUniform uniform) {
	switch (uniform.type) {
	case UniformType::Int:
		program->Bind(uniform.name, uniform.valuei[0]);
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

void Scene::bindMaterial(SceneMaterial texture) {
	if (texture.albedo->TextureId > 0) program->Bind(texture.name + ".albedo", texture.albedo->Use2D());
	if (texture.roughness->TextureId > 0) program->Bind(texture.name + ".roughness", texture.roughness->Use2D());
	if (texture.metal->TextureId > 0) program->Bind(texture.name + ".metal", texture.metal->Use2D());
	if (texture.normal->TextureId > 0) program->Bind(texture.name + ".normal", texture.normal->Use2D());
	if (texture.ambientOcclusion->TextureId > 0) program->Bind(texture.name + ".ambientOcclusion", texture.ambientOcclusion->Use2D());
	if (texture.height->TextureId > 0) program->Bind(texture.name + ".height", texture.height->Use2D());
}

void Scene::getUniformsFromSource() {
	
	std::string line;
	std::istringstream sourceStream(shaderSource);

	std::vector<std::string> namesInSource;
	while (std::getline(sourceStream, line)) {
		if (line.find("SceneUniform") != std::string::npos) {
			char typeName[25], name[25];
			typeName[0] = '\0';
			name[0] = '\0';
			float min = -10.0, max = 10.0;
			
			std::istringstream lineS(line);

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
