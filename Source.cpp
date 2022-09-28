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

#define _simWidth 15
#define _simHeight 8.4

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
bool ImGui_ImplOpenGL3_Init(const char* glsl_version);

int scrnX, scrnY;
const int SCRN_WIDTH = 1920, SCRN_HEIGHT = 1080;
bool firstMouse = true, addPlanetClicked;
float lastX = 0, lastY = 0;
float radius = 1;
CameraClass camera(glm::vec3(0, 0, 1));
glm::vec2 normPlacedPosition;

float deltaTime, lastFrame = 0, currentFrame;
bool planetsExist = false;
float gravity = 0.05;
float mouseGradient;

std::vector<planet>planets;
std::vector<line>shadowLines;

// gui variables
bool choosePlanetPos = false, positionChoosen = false;
// mouse position
double mxpos, mypos;
// stores state of pause of sim
bool paused = false;
// speed of sim
float timeStep = 1;

// function to add planets
void addPlanet(std::vector<planet>&p, planet toAdd) {
	// temporary planet
	planet t;
	// initiate values for temporary planet
	t.mass = toAdd.mass;
	t.colour[0] = toAdd.colour[0]; t.colour[1] = toAdd.colour[1]; t.colour[2] = toAdd.colour[2];
	t.pos = toAdd.pos;
	// add temporary planet to vector
	p.push_back(t);
}

int main() {
	// Setup
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// disable window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

	std::cout << glGetError << std::endl;

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	Shader shaders("vertex.GLSL", "fragment.GLSL");
	Shader arrowShader("vertex.GLSL", "fragment.GLSL");

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int VAO2, VBO2;
	glGenVertexArrays(1, &VAO2);
	glGenBuffers(1, &VBO2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(arrowVertices), &arrowVertices, GL_STATIC_DRAW);

	glBindVertexArray(VAO2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);	

	// imgui boilerplate
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// stores variables from ui input
	planet temporaryPlanet;
	// center light source
	planet sun;
	sun.colour[0] = 1; sun.colour[1] = 0.9; sun.colour[2] = 0.1;
	sun.mass = 10; sun.pos = glm::vec3(0, 0, 0);

	// Matrices
	glm::mat4 projection = glm::perspective(glm::radians(camera.ZOOM), (float)SCRN_WIDTH / (float)SCRN_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	while (!glfwWindowShouldClose(window)) {
		// calculate delta time
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// only call physics if simulation isnt paused
		if (!paused)
			physics(planets, sun, deltaTime, gravity, timeStep);

		camera.keyboardInput(window, deltaTime);
		
		// Background is slightly lit by light source
		glClearColor(sun.colour[0] * 0.08, sun.colour[1] * 0.08, sun.colour[2] * 0.08, 1);
		//glClearColor(0.1, 0.1, 0.1, 0.1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glm::mat4 model = glm::mat4(1.0);

		shaders.use();

		// draw light in middle
		model = glm::scale(model, glm::vec3(0.05));

		shaderInputs(shaders, model, view, projection, 
			glm::vec4(sun.colour[0], sun.colour[1], sun.colour[2], 1),
			glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
			true);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size() * sizeof(float));

		// call addPlanet to get how many objects there are
		for (int i = 0; i < planets.size(); i++) {
			model = glm::mat4(1.0);
			model = glm::scale(model, glm::vec3(0.05));
			model = glm::translate(model, planets[i].pos);

			shaderInputs(shaders, model, view, projection,
				glm::vec4(planets[i].colour[0], planets[i].colour[1], planets[i].colour[2], 1),
				glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
				true);

			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		}

		// draw ui to screen
		// tell imgui its new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ui elements
		ImGui::Begin("Menu");
		ImGui::Text("---PLANET CREATE---");
		ImGui::Text("Planet Colour");
		ImGui::ColorPicker3("", temporaryPlanet.colour);
		//ImGui::InputText("Planet name", temporaryPlanet.name, 512);
		ImGui::Text("Planet Mass");
		ImGui::SliderFloat("1 - 10", &temporaryPlanet.mass, 1, 10);

		if (ImGui::Button("Choose Planet Position")) {
			choosePlanetPos = true;
			positionChoosen = false;
		}
		addPlanetClicked = false;
		if (ImGui::Button("Add Planet"))
			addPlanetClicked = true;

		//ImGui::Text(" ");
		ImGui::Text("---SIMULATION VARIABLES---");
		// if not paused display paused button
		if (!paused)
			if (ImGui::Button("Pause Simulation"))
				paused = true;
		// if paused display continue button
		if (paused)
			if (ImGui::Button("Play Simulation"))
				paused = false;

		ImGui::Text("Strength Of Gravity");
		ImGui::SliderFloat("0.005 - 1", &gravity, 0.005, 1);
		ImGui::Text("Timestep Of Simulation");
		ImGui::SliderFloat("0.25 - 3", &timeStep, 0.25, 3);
		ImGui::Text("Light Colour");
		ImGui::ColorPicker3(" ", sun.colour);


		glfwGetCursorPos(window, &mxpos, &mypos);
		glfwGetWindowSize(window, &scrnX, &scrnY);
		
		// normalized mouse coords
		mxpos /= scrnX; mypos /= scrnY; 
		// between -0.5 and 0.5
		mxpos -= 0.5; mypos -= 0.5;
		// *2 to scale from -1 to 1
		mxpos *= 2; mypos *= -2;
		// gradient of mouse coordinates
		mouseGradient = mypos / mxpos;

		printf("%f\n", glm::degrees(tan(mxpos/mypos));
		
		// render semi transparent planet if user is choosing position of new planet
		if (choosePlanetPos) {
			shaders.use();

			// check to place planet
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && !positionChoosen) {
				positionChoosen = true;
				temporaryPlanet.pos = glm::vec3(mxpos * _simWidth, mypos * _simHeight, 0);
				// coordinate from -1 to 1 of placed planets position
				normPlacedPosition.x = mxpos; normPlacedPosition.y = mypos;
			}

			model = glm::mat4(1);
			model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
			// translate to mouse pos if position isnt choosen
			if (!positionChoosen)
				model = glm::translate(model, glm::vec3(mxpos * _simWidth, mypos * _simHeight, 0));
			else
				model = glm::translate(model, temporaryPlanet.pos);
			
			shaderInputs(shaders, model, view, projection, 
				glm::vec4(temporaryPlanet.colour[0], temporaryPlanet.colour[1], temporaryPlanet.colour[2], 1),
				glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
				true);

			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			// once position is chossen draw arrow showing starting velocity
			if (positionChoosen) {
				// if Add Planet is clicked and all planet values are filled add the planet
				if (addPlanetClicked && checkPlanet(temporaryPlanet)) {
					choosePlanetPos = false;
					positionChoosen = false;
					// add new planet to planet vector
					addPlanet(planets, temporaryPlanet);
					// set flag to render planets as true
					planetsExist = true;
					// reset temporary planets values to default
					temporaryPlanet.reset();
				}

				arrowShader.use();

				model = glm::mat4(1);
				model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
				//model = glm::translate(model, temporaryPlanet.pos);
				model = glm::rotate(model, generateAngle(mxpos, mypos), glm::vec3(0, 0, -1));

				shaderInputs(arrowShader, model, view, projection,
					glm::vec4(0.8, 0.8, 0.8, 0.75), 
					glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
					false);

				glBindVertexArray(VAO2);
				glDrawArrays(GL_TRIANGLES, 0, sizeof(arrowVertices));
			}
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

	//camera.mouseInput(xoffset, yoffset);
}
