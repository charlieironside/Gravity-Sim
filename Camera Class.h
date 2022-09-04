#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class CameraClass {
public:
	glm::vec3 Position;
	glm::vec3 LookAt;
	glm::vec3 Right;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 TrueUp;

	float yaw = -90, pitch = 0, sensitivity;
	float movementSpeed = 10;
	const float ZOOM = 45.0f;

	CameraClass(glm::vec3 pos, glm::vec3 up = glm::vec3(0, 1, 0), float mouseSens = 0.1)
		: TrueUp(up), Position(pos), Front(glm::vec3(0, 0, -1)), Up(up), sensitivity(mouseSens) { cameraVectors(); };

	// Returns view matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	void keyboardInput(GLFWwindow* window, float deltaTime) {
		float speed = movementSpeed * deltaTime;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			Position += Front * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			Position -= Front * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			Position -= Right * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			Position += Right * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			Position += TrueUp * speed;
		}
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			Position -= TrueUp * speed;
		}
	}

	void mouseInput(float xOffset, float yOffset, bool constrainPitch = true) {
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		// Add to 
		yaw += xOffset;
		pitch += yOffset;

		if (constrainPitch) {
			// Prevents camera flipping upside down
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		cameraVectors();
	}

private:
	void cameraVectors() {
		glm::vec3 notNormalizedFront;

		notNormalizedFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		notNormalizedFront.y = sin(glm::radians(pitch));
		notNormalizedFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		Front = glm::normalize(notNormalizedFront);

		Right = glm::normalize(glm::cross(Front, TrueUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};