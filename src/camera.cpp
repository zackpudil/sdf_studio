#include <camera.h>

Camera::Camera(glm::vec3 p, glm::vec3 d) :
	Position(p),
	Direction(d)
{
	
}

void Camera::HandleInput(GLFWwindow* window) {
	float currentFrame = glfwGetTime();

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
		handleMouse(window);
		handleKeyboard(window, currentFrame - lastFrame);
	} else {
		IsMoving = false;
	}

	lastFrame = currentFrame;
}

glm::mat3 Camera::GetViewMatrix() {
	glm::vec3 right = glm::normalize(glm::cross(Direction, CAMERA_UP));
	glm::vec3 up = glm::normalize(glm::cross(right, Direction));

	return glm::mat3(right, up, Direction);
}

void Camera::handleMouse(GLFWwindow *window) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	if (!IsMoving) {
		lastXPos = xpos;
		lastYPos = ypos;
		IsMoving = true;
	}

	calculateDirection(xpos - lastXPos, lastYPos - ypos);
	lastXPos = xpos;
	lastYPos = ypos;
}

void Camera::handleKeyboard(GLFWwindow *window, float deltaTime) {
	float frameSpeed = Speed * deltaTime;

	glm::vec3 right = glm::normalize(glm::cross(Direction, CAMERA_UP));
	glm::vec3 up = glm::normalize(glm::cross(right, Direction));

	if (glfwGetKey(window, FORWARD_KEY) == GLFW_PRESS)
		Position += Direction * frameSpeed;

	if (glfwGetKey(window, BACKWARD_KEY) == GLFW_PRESS)
		Position -= Direction * frameSpeed;

	if (glfwGetKey(window, RIGHT_KEY) == GLFW_PRESS)
		Position += right * frameSpeed;

	if (glfwGetKey(window, LEFT_KEY) == GLFW_PRESS)
		Position -= right * frameSpeed;

	if (glfwGetKey(window, UP_KEY) == GLFW_PRESS)
		Position += up * frameSpeed;

	if (glfwGetKey(window, DOWN_KEY) == GLFW_PRESS)
		Position -= up * frameSpeed;
}

void Camera::calculateDirection(double xoffset, double yoffset) {
	yaw += (float)(xoffset * Sensitivity);
	pitch += (float)(yoffset * Sensitivity);

	pitch = fmax(fmin(pitch, 89.0f), -89.0f);

	Direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	Direction.y = sin(glm::radians(pitch));
	Direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	Direction = glm::normalize(Direction);
}