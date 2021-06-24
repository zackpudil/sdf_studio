#include <program.h>
#include <glm\gtc\type_ptr.hpp>
#include <vector>

Program::~Program() {
	glUseProgram(0);
	glDeleteProgram(program);
	program = 0;
}

Program& Program::Reload() {
	glUseProgram(0);
	glDeleteProgram(program);

	for (GLuint shader : shaders) glDeleteShader(shader);
	shaders.clear();
	
	program = glCreateProgram();

	return *this;
}

Program& Program::Attach(std::string src, GLenum type) {
	auto shader = glCreateShader(type);
	auto source = src.c_str();

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	int status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

		glDeleteShader(shader);

		fprintf(stderr, "error compile shader: \n%s\n", &errorLog[0]);
		throw std::exception(errorLog.data());
	}

	shaders.push_back(shader);
	glAttachShader(program, shader);

	return *this;
}

Program& Program::Link() {
	glLinkProgram(program);

	GLint status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_FALSE) {
		int maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		std::vector<GLchar> errorLog(maxLength);
		glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);

		fprintf(stderr, "error linking: %s \n", &errorLog[0]);
		throw std::exception(errorLog.data());
	}

	return *this;
}

Program& Program::Activate() {
	glUseProgram(program);
	return *this;
}

void Program::bind(GLuint loc, int value) {
	glUniform1i(loc, value);
}

void Program::bind(GLuint loc, float value) {
	glUniform1f(loc, value);
}

void Program::bind(GLuint loc, glm::vec2 value) {
	glUniform2fv(loc, 1, glm::value_ptr(value));
}


void Program::bind(GLuint loc, glm::vec3 value) {
	glUniform3fv(loc, 1, glm::value_ptr(value));
}


void Program::bind(GLuint loc, glm::vec4 value) {
	glUniform4fv(loc, 1, glm::value_ptr(value));
}

void Program::bind(GLuint loc, glm::mat2 value) {
	glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}


void Program::bind(GLuint loc, glm::mat3 value) {
	glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}


void Program::bind(GLuint loc, glm::mat4 value) {
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}