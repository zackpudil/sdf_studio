#include <string>
#include <glad/glad.h>

#pragma once

class Texture {
public:
	Texture();

	void LoadHDRIFromFile2D(std::string);
	void LoadFromFile2D(std::string);
	void Allocate2D();
	void AllocateCube(int width, int height, bool generateMipMap = false);

	int Use2D();
	int UseCube();

	void GenerateMipmap();

	GLuint TextureId;
	int TextureUnit;

	int Width;
	int Height;
private:
	static int CurrentTextureUnit;
};