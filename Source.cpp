#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <string>

#include "Shader.h"

#include "Camera Class.h"
#include "vertices.h"
#include "namespace.h"

using namespace functions;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool ImGui_ImplOpenGL3_Init(const char* glsl_version);

int vectorSize = 0;
const int SCRN_WIDTH = 1200, SCRN_HEIGHT = 1000;
bool firstMouse = true;
float lastX = 0, lastY = 0;
float simDepth = -20, radius = 1;
CameraClass camera(glm::vec3(0, 0, 2));

float deltaTime, lastFrame = 0, currentFrame;
glm::vec3 lightPosition = glm::vec3(0, 0, simDepth), lightColour = glm::vec3(1, 0.95, 0.5);

std::vector<planet> planets;
std::vector<line>shadowLines;

// input variables
bool selectingPlanet = false;

int main() {
	// Setup
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Remove frap cap
	glfwSwapInterval(0);

	// Create window object
	GLFWwindow* window = glfwCreateWindow(SCRN_WIDTH, SCRN_HEIGHT, "ALPHA", NULL, NULL);
	if (window == NULL) {
		std::cout << "Falid to create glfw object\n";
		glfwTerminate();
		return -1;
	}

	// Generate new triangles
	for (int i = 0; i < 36; i += 9) {
		std::vector<float>temp;

		temp.push_back(vertexData[i]);
		temp.push_back(vertexData[i + 1]);
		temp.push_back(vertexData[i + 2]);
		temp.push_back(vertexData[i + 3]);
		temp.push_back(vertexData[i + 4]);
		temp.push_back(vertexData[i + 5]);
		temp.push_back(vertexData[i + 6]);
		temp.push_back(vertexData[i + 7]);
		temp.push_back(vertexData[i + 8]);

		triangleGeneration(temp, vertices, 4);
	}

	circleGeneration(vertices);

	glfwMakeContextCurrent(window);
	// Input functions
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	// Hides cursor
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Loads Glad.c
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to create glfw window";
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	Shader shaders("vertex.GLSL", "fragment.GLSL");

	unsigned int VBO, lineVBO, VAO, lineVAO;
	glGenVertexArrays(1, &VAO);
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &lineVBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	planet p;
	p.pos = glm::vec3(0, 1, -20);
	planets.push_back(p);
		
	// Matrices
	glm::mat4 projection = glm::perspective(glm::radians(camera.ZOOM), (float)SCRN_WIDTH / (float)SCRN_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// imgui boilerplate
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	while (!glfwWindowShouldClose(window)) {
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		camera.keyboardInput(window, deltaTime);
		
		// Set background colour
		glClearColor(0, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glm::mat4 model = glm::mat4(1.0);

		shaders.use();

		// Light source
		model = glm::translate(model, lightPosition);

		shaderInputs(shaders, model, projection, view, camera.Position, lightColour, lightPosition, lightColour, true, shadowLines);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		// Objects
		for (int i = 0; i < 1; i++) {
			model = glm::mat4(1.0);
			model = glm::translate(model, planets[i].pos);

			
			shadowSolver(lightPosition, planets[i].pos, shadowLines, radius, false);
			
			shaderInputs(shaders, model, projection, view, camera.Position, lightColour, lightPosition, glm::vec3(1, 1, 1), false, shadowLines);
			
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}

		// draw ui to screen
		planet tempPlanet;
		tempPlanet.mass = 0;
		// tell imgui its new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Create Planet");

		ImGui::InputText("Planet name", tempPlanet.name, 512);
		//ImGui::InputFloat("Planet Radius", &tempPlanet.radius);
		//ImGui::InputInt("Planet Mass", &tempPlanet.mass);
		ImGui::Checkbox("Light Source", &tempPlanet.lightSource);
		//ImGui::ColorPicker3("Planet colour", tempPlanet.colour);

		ImGui::Button("Add planet");

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	// cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.mouseInput(xoffset, yoffset);
}