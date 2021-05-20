#include "project.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

void Project::NewScene() {
	SavePath = "";
	ProjectCamera = new Camera(glm::vec3(0, 0, -3), glm::vec3(0, 0, 1));
	ProjectEnvironment = new Environment();
	ProjectScene = new Scene(ProjectCamera, ProjectEnvironment);

	ProjectEnvironment->GetLights()->push_back({
		LightType::Sunlight,
		glm::vec3(0.7, 0.8, -0.6),
		glm::vec3(1),
		true,
		64
	});

	ProjectScene->ShaderSource = R""""(
uniform float radius; // SceneUniform 0.5,2

float de(vec3 p, out int mid) {
	float s = length(p) - radius;
	float pl = p.y + 1.0;
	mid = s < pl ? 1 : 2;

	return min(s, pl);
}

Material getMaterial(vec3 p, inout vec3 n, int mid) {
	if(mid == 1) {
		return createHardMaterial(vec3(1.0, 0.3, 0.3), 0.3, 0);
	}

	return createHardMaterial(vec3(1), 0.4, 0);
}

SubSurfaceMaterial getSubsurfaceMaterial(Material m, int mid) {
	return SubSurfaceMaterial(vec3(0), 0, 0, 0, 0);
}
	)"""";

	ProjectScene->SetShader(ProjectScene->ShaderSource);
	ProjectScene->InitShader();
	(*ProjectScene->GetUniforms())[0].valuesf[0] = 1.0;
}

bool Project::SaveScene() {
	if (SavePath.empty()) return false;

	std::ofstream fileData;
	fileData.open(SavePath, std::ios_base::trunc | std::ios_base::in | std::ios_base::out);

	fileData << ProjectScene->ShaderSource << std::endl;
	fileData << "END CODE" << std::endl;

	fileData << ProjectScene->FudgeFactor << " " << ProjectScene->MaxDistance << " " << ProjectScene->ResolutionScale << std::endl;
	fileData << "END DEBUG" << std::endl;

	for (auto& material : *ProjectScene->GetMaterials()) {
		fileData << material.name
			<< " " << material.albedoPath
			<< " " << material.normalPath
			<< " " << material.roughnessPath
			<< " " << material.metalPath
			<< " " << material.ambientOcclusionPath
			<< " " << material.heightPath
			<< std::endl;
	}
	fileData << "END MATERIALS" << std::endl;

	for (auto &uniform : *ProjectScene->GetUniforms()) {
		fileData << uniform.name;
		for (auto value : uniform.valuesf) {
			fileData << " " << value;
		}

		for (auto value : uniform.valuesi) {
			fileData << " " << value;
		}

		fileData << std::endl;
	}
	fileData << "END UNIFORMS" << std::endl;

	if (!ProjectEnvironment->HdriPath.empty()) {
		fileData << ProjectEnvironment->HdriPath << " " << ProjectEnvironment->LightPathExposure << " " << ProjectEnvironment->UseIrradianceForBackground << std::endl;
	}

	fileData << "END ENV" << std::endl;

	auto outGLM = [&](glm::vec3 v) {
		return std::string(std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z));
	};
	
	for (auto &light : *ProjectEnvironment->GetLights()) {
		fileData 
			<< (int)light.type << " " 
			<< outGLM(light.color) << " " 
			<< outGLM(light.position) << " " 
			<< light.hasShadow << " " 
			<< light.shadowPenumbra << std::endl;
	}

	fileData << "END LIGHTS" << std::endl;

	fileData
		<< ProjectCamera->Fov << " "
		<< ProjectCamera->Exposure << " "
		<< outGLM(ProjectCamera->Position) << " "
		<< outGLM(ProjectCamera->Direction) << " "
		<< ProjectCamera->DepthOfField << std::endl;

	fileData << "END CAMERA";

	fileData << std::endl;
	fileData.close();

	return true;
}

enum class ReadMode {
	Code = 0,
	Uniforms = 1,
	Debug,
	Materials,
	HdirPath,
	Lights,
	Camera
};

void Project::LoadScene(std::string filePath) {
	SavePath = filePath;

	std::fstream fileData;
	ProjectCamera = new Camera(glm::vec3(0, 0, -3), glm::vec3(0, 0, 1));
	ProjectEnvironment = new Environment();
	ProjectScene = new Scene(ProjectCamera, ProjectEnvironment);

	ReadMode CurrentReadMode = ReadMode::Code;

	fileData.open(SavePath);
	if (fileData.is_open())
	{
		std::string line;
		while (std::getline(fileData, line)) {
			std::stringstream ss(line);
			auto inGLM = [&]() {
				glm::vec3 v;
				ss >> v.x >> v.y >> v.z;
				return v;
			};
			switch (CurrentReadMode) {
			case ReadMode::Code:
				if (line == "END CODE") {
					CurrentReadMode = ReadMode::Debug;
				} else {
					ProjectScene->ShaderSource += line + "\n";
				}
				break;
			case ReadMode::Debug:
				if (line == "END DEBUG") {
					CurrentReadMode = ReadMode::Materials;
					ProjectScene->UpdateResolution();
				} else {
					ss >> ProjectScene->FudgeFactor >> ProjectScene->MaxDistance >> ProjectScene->ResolutionScale;
				}
				break;
			case ReadMode::Materials:
				if (line == "END MATERIALS") {
					CurrentReadMode = ReadMode::Uniforms;
					ProjectScene->SetShader(ProjectScene->ShaderSource);
					ProjectScene->InitShader();
				}
				else {
					std::string name, paths[6];
					ss >> name >> paths[0] >> paths[1] >> paths[2] >> paths[3] >> paths[4] >> paths[5];
					std::vector<std::string> pathsV(paths, paths + 6);

					const auto findByType = [&](const std::string type) {
						auto it = std::find_if(
							pathsV.begin(),
							pathsV.end(),
							[&](std::string s) {
								std::string lowerName = s;
								std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
								return lowerName.find(type) != std::string::npos;
							}
						);

						if (it != pathsV.end()) return *it;

						return std::string("");
					};

					SceneMaterial material;
					material.name = name;

					material.albedo = new Texture();
					material.albedoPath = findByType("_albedo");
					material.albedo->LoadFromFile2D(material.albedoPath);

					material.roughness = new Texture();
					material.roughnessPath = findByType("_rough");
					material.roughness->LoadFromFile2D(material.roughnessPath);

					material.metal = new Texture();
					material.metalPath = findByType("_metal");
					material.metal->LoadFromFile2D(material.metalPath);

					material.normal = new Texture();
					material.normalPath = findByType("_normal");
					material.normal->LoadFromFile2D(material.normalPath);

					material.ambientOcclusion = new Texture();
					material.ambientOcclusionPath = findByType("_ao");
					material.ambientOcclusion->LoadFromFile2D(material.ambientOcclusionPath);

					material.height = new Texture();
					material.heightPath = findByType("_height");
					material.height->LoadFromFile2D(material.heightPath);

					ProjectScene->GetMaterials()->push_back(material);

				}
				break;
			case ReadMode::Uniforms:
				if (line == "END UNIFORMS") {
					CurrentReadMode = ReadMode::HdirPath;
				} else {
					std::string name;
					float valuesf[16];
					int valuesi[16];

					ss >> name;
					for (int i = 0; i < 16; i++) ss >> valuesf[i];
					for (int i = 0; i < 16; i++) ss >> valuesi[i];

					auto findUniform = std::find_if(
						ProjectScene->GetUniforms()->begin(),
						ProjectScene->GetUniforms()->end(),
						[=](SceneUniform& s) {
							if (s.name.compare(name) == 0) {
								for (int i = 0; i < 16; i++) s.valuesf[i] = valuesf[i];
								for (int i = 0; i < 16; i++) s.valuesi[i] = valuesi[i];

								return true;
							}
							return false;
						}
					);
				}
				break;
			case ReadMode::HdirPath:
				if (line == "END ENV") {
					CurrentReadMode = ReadMode::Lights;
				}
				else
				{
					std::string path;
					float exposure;
					bool useIrr;

					ss >> path >> exposure >> useIrr;
					ProjectEnvironment->LightPathExposure = exposure;
					ProjectEnvironment->UseIrradianceForBackground = useIrr;
					ProjectEnvironment->SetHDRI(path);
					ProjectEnvironment->PreRender();
				}
				
				break;
			case ReadMode::Lights:
				if (line == "END LIGHTS") {
					CurrentReadMode = ReadMode::Camera;
				} else {
					int lightType;
					glm::vec3 position;
					glm::vec3 color;
					bool hasShadow;
					float shadowPenumbra;

					ss >> lightType;
					color = inGLM();
					position = inGLM();
					ss >> hasShadow >> shadowPenumbra;

					ProjectEnvironment->GetLights()->push_back({
						(LightType)lightType,
						position,
						color,
						hasShadow,
						shadowPenumbra
					});

				}
				break;
			case ReadMode::Camera:
				if (line != "END CAMERA") {
					ss >> ProjectCamera->Fov >> ProjectCamera->Exposure;
					ProjectCamera->Position = inGLM();
					ProjectCamera->Direction = inGLM();
					ss >> ProjectCamera->DepthOfField;
				}
				break;
			}
		}
	}

}
