#include <string>
#include <glad/glad.h>

#pragma once

class Texture {
public:
	Texture();

	void LoadHDRIFromFile2D(std::string);
	void LoadFromFile2D(std::string);
	void Allocate2D(int width=512, int height=512, bool rg = true);
	void AllocateCube(int width, int height, bool generateMipMap = false);

	int Use2D();
	int UseCube();

	void GenerateMipmap();

	void DeleteTexture();

	GLuint TextureId;
	int TextureUnit;

	int Width;
	int Height;
private:
	static int CurrentTextureUnit;
};