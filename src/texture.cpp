#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int Texture::CurrentTextureUnit = 0;

Texture::Texture() {
	Width = 0;
	Height = 0;
	TextureId = -1;

	TextureUnit = CurrentTextureUnit;
	CurrentTextureUnit++;
}

void Texture::LoadHDRIFromFile2D(std::string file) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, nComps;
	float* data = stbi_loadf(file.c_str(), &width, &height, &nComps, 0);

	if (data) {
		glGenTextures(1, &TextureId);
		glBindTexture(GL_TEXTURE_2D, TextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Width = width;
		Height = height;
		stbi_image_free(data);
	} else {
		throw std::exception("Unable to load image");
	}
}

void Texture::LoadFromFile2D(std::string file) {
	if (file == "") return;

	int width, height, nrComps;

	unsigned char* data = stbi_load(file.c_str(), &width, &height, &nrComps, 0);
	if (data) {
		Width = width;
		Height = height;
		GLenum format = nrComps == 1 ? GL_RED : nrComps == 3 ? GL_RGB : GL_RGBA;

		glGenTextures(1, &TextureId);
		glBindTexture(GL_TEXTURE_2D, TextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	} else {
		throw std::exception(("Unable to read image: " + file).c_str());
	}
}

void Texture::AllocateCube(int width, int height, bool generateMipMap) {
	glGenTextures(1, &TextureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, TextureId);
	for (GLuint i = 0; i < 6; i++) 
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (!generateMipMap) {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::Allocate2D(int width, int height, bool rg) {
	glGenTextures(1, &TextureId);
	glBindTexture(GL_TEXTURE_2D, TextureId);
	if (rg) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RG, GL_FLOAT, 0);
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, 0);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Width = width;
	Height = height;
}

int Texture::Use2D() {
	glActiveTexture(GL_TEXTURE0 + TextureUnit);
	glBindTexture(GL_TEXTURE_2D, TextureId);
	return TextureUnit;
}

int Texture::UseCube() {
	glActiveTexture(GL_TEXTURE0 + TextureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, TextureId);
	return TextureUnit;
}

void Texture::GenerateMipmap() {
	glBindTexture(GL_TEXTURE_CUBE_MAP, TextureId);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}
