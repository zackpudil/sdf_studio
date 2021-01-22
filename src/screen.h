#include <program.h>
#include <texture.h>

#pragma once

class Screen {
public:
	void PrepareQuad();
	void DrawQuad();

	void PrepareCube();
	void DrawCube();
private:
	GLuint ebo, vbo, vao;
};