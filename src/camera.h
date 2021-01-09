
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#pragma once

class Camera {
public:
	Camera(glm::vec3, glm::vec3);

	glm::vec3 Position;
	glm::vec3 Direction;

	float Speed = 2.0f;
	float Sensitivity = 0.5f;
	float Fov = 1.97f;
	float Exposure = 0.5f;

	void HandleInput(GLFWwindow*);
	glm::mat3 GetViewMatrix();

	bool IsMoving = false;

private:
	const glm::vec3 CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
	const int FORWARD_KEY = GLFW_KEY_W;
	const int BACKWARD_KEY = GLFW_KEY_S;
	const int LEFT_KEY = GLFW_KEY_A;
	const int RIGHT_KEY = GLFW_KEY_D;
	const int UP_KEY = GLFW_KEY_Q;
	const int DOWN_KEY = GLFW_KEY_E;

	float lastFrame = 0.0f;
	float yaw = -90.0f;
	float pitch = 0.0f;

	double lastXPos = 0.0f;
	double lastYPos = 0.0f;

	void handleMouse(GLFWwindow *);
	void handleKeyboard(GLFWwindow *,float);

	void calculateDirection(double, double);
};
