#include <string>
#include <glad\glad.h>
#include <glm\glm.hpp>
#include <map>
#include <vector>

#pragma once

class Program {
public:
	~Program();

	Program& Attach(std::string , GLenum);
	Program& Reload();

	Program& Link();
	Program& Activate();

	template <typename T> Program& Bind(std::string const& name, T&& value) {
		int loc = glGetUniformLocation(program, name.c_str());
		if (loc != -1)
			bind(loc, std::forward<T>(value));
		else
			fprintf(stderr, "Unable to find uniform %s\n", name.c_str());

		return *this;
	}

private:
	GLuint program;
	std::vector<GLuint> shaders;

	void bind(GLuint, int);
	void bind(GLuint, float);
	void bind(GLuint, glm::vec2);
	void bind(GLuint, glm::vec3);
	void bind(GLuint, glm::vec4);
	void bind(GLuint, glm::mat2);
	void bind(GLuint, glm::mat3);
	void bind(GLuint, glm::mat4);
};