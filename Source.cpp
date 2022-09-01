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

int scrnX, scrnY;
const int SCRN_WIDTH = 1920, SCRN_HEIGHT = 1080;
bool firstMouse = true;
float lastX = 0, lastY = 0;
float radius = 1;
CameraClass camera(glm::vec3(0, 0, 2));

float deltaTime, lastFrame = 0, currentFrame;
glm::vec3 lightPosition = glm::vec3(0, 0, 0), lightColour = glm::vec3(1, 0.95, 0.5);

planet* planets;
std::vector<line>shadowLines;

// gui variables
bool choosePlanetPos = false;
// mouse position
double mxpos, mypos;

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
	Shader selectionShaders("transV.GLSL", "transF.GLSL");

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
		
	// Matrices
	glm::mat4 projection = glm::perspective(glm::radians(camera.ZOOM), (float)SCRN_WIDTH / (float)SCRN_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	float simWidth = ((camera.Position.z) * tan(camera.ZOOM)) / 2;

	// imgui boilerplate
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	planets = new planet[1];

	planet p;
	p.pos = glm::vec3(0, 1, -2);

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
		model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));

		shaderInputs(shaders, model, projection, view, camera.Position, lightColour, lightPosition, lightColour, true, shadowLines);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		glm::vec3 colour; colour.x = p.colour[0]; colour[1] = p.colour[1]; colour[2] = p.colour[2];

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
		// tell imgui its new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Create Planet");

		ImGui::ColorPicker3("Planet colour", p.colour);
		ImGui::InputText("Planet name", p.name, 512);
		ImGui::InputFloat("Planet Radius", &p.radius);
		ImGui::InputInt("Planet Mass", &p.mass);
		ImGui::Checkbox("Light Source", &p.lightSource);

		ImGui::Button("Add planet");
		if (ImGui::Button("Choose Planet Position"))
			choosePlanetPos = true;
			
		// render transparent planet if user is choosing position of new planet
		if (choosePlanetPos) {
			//selectionShaders.use();
			model = glm::mat4(1);
			glfwGetCursorPos(window, &mxpos, &mypos);
			glfwGetWindowSize(window, &scrnX, &scrnY);
			float xyRatio = scrnX / scrnY;
			// normalized mouse coords								  // *2 to scale from -1 to 1
			mxpos /= scrnX; mypos /= scrnY; mxpos -= 0.5; mypos -= 0.5; mxpos *= 2; mypos *= 2;
			model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
			model = glm::translate(model, glm::vec3((mxpos * simWidth), (-mypos * simWidth) * xyRatio, 0));

			printf("%f %f\n", (float)mxpos, (float)mypos);

			shaders.setMat4("model", model);
			shaders.setMat4("view", view);
			shaders.setMat4("proj", projection);

			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}

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
	delete[] planets;

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