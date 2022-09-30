#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class cam {
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

	cam(glm::vec3 pos, glm::vec3 up = glm::vec3(0, 1, 0), float mouseSens = 0.1)
		: TrueUp(up), Position(pos), Front(glm::vec3(0, 0, -1)), Up(up), sensitivity(mouseSens) { cameraVectors(); };

	// Returns view matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
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
