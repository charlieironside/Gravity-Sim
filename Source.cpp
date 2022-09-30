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

bool ImGui_ImplOpenGL3_Init(const char* glsl_version);

int scrnX, scrnY;
const int SCRN_WIDTH = 1920, SCRN_HEIGHT = 1080;
bool firstMouse = true, addPlanetClicked;
float lastX = 0, lastY = 0;
float radius = 1;
cam camera(glm::vec3(0, 0, 1));

float deltaTime, lastFrame = 0, currentFrame;
bool planetsExist = false;
bool showVectors = true;
float gravity = 0.05;
float mouseGradient;

std::vector<planet>planets;
std::vector<line>shadowLines;

// variable to hold state of user choosing planet position
bool choosePlanetPos = false, positionChoosen = false, velocityChoosen = false;
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
	t.velocity = toAdd.velocity;
	// add temporary planet to vector
	p.push_back(t);
}

int main() {
	// glfw setup
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// disable window resize
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	// remove frap cap
	glfwSwapInterval(0);

	// Create window object
	GLFWwindow* window = glfwCreateWindow(SCRN_WIDTH, SCRN_HEIGHT, "ALPHA", NULL, NULL);
	if (window == NULL) {
		std::cout << "Falid to create glfw object\n";
		glfwTerminate();
		return -1;
	}

	// vertexData are vertices for a tetrahedron
	// split each face into 4 smaller triangles using triangleGeneration()
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

	// with the tetrahedron having multiple triangles per face, normalize each vertex
	// which makes each vertice an equal distance from the center creating a circle
	circleGeneration(vertices);

	glfwMakeContextCurrent(window);
	// Input functions
	

	// Loads Glad.c
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to create glfw window";
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	Shader shaders("vertex.GLSL", "fragment.GLSL");
	Shader arrowShader("vertex.GLSL", "fragment.GLSL");

	// VBO is a buffer containing vertex data generated above to be sent to the GPU to render
	// VAO tells the GPU how to intepret/use that data
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// link the vbo to an array buffer
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	
	glBindVertexArray(VAO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// seperate VAO and VBO for rendering the vectors/arrows
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

	// temporaryPlanet is a variable that contains the information the user is
	// entering into the ui when the user presses "add planet" the data from temporaryPlanet 
	// is added to the planets array and temporaryPlanet's data is reset
	planet temporaryPlanet;
	// center light source
	planet sun;
	sun.colour[0] = 1; sun.colour[1] = 0.95; sun.colour[2] = 0.1;
	sun.mass = 50; sun.pos = glm::vec3(0, 0, 0);

	// matrices
	glm::mat4 projection = glm::perspective(glm::radians(camera.ZOOM), (float)SCRN_WIDTH / (float)SCRN_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// starting velocity of temporary planet
	glm::vec3 temporaryVector;
	// z component is vector magnitude
	temporaryVector.z = 0;

	// stores time after a click to compute a delay
	float timeAtClick;

	while (!glfwWindowShouldClose(window)) {
		// calculate delta time
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// only call physics if simulation isnt paused
		if (!paused)
			physics(planets, sun, deltaTime, gravity, timeStep);;
		
		// glClearColor sets the entire colour buffer to one colour
		// background is slightly lit by light source
		glClearColor(sun.colour[0] * 0.06 + 0.025, sun.colour[1] * 0.06 + 0.025, sun.colour[2] * 0.06 + 0.025, 1);

		// also clear the other buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// this matrix represents the translation, scale and rotation of what
		// ever is currently being drawn
		glm::mat4 model = glm::mat4(1.0);

		shaders.use();

		// draw light in middle
		model = glm::scale(model, glm::vec3(0.05));

		// send shader variables to GPU as "unifroms"
		shaderInputs(shaders, model, view, projection, 
			glm::vec4(sun.colour[0], sun.colour[1], sun.colour[2], 1),
			glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
			true);

		glBindVertexArray(VAO);
		// this is a draw call for openGL
		glDrawArrays(GL_TRIANGLES, 0, vertices.size() * sizeof(float));

		// loop through each planet in planet array
		for (int i = 0; i < planets.size(); i++) {
			// reset the model matrix
			model = glm::mat4(1.0);
			model = glm::scale(model, glm::vec3(0.05));
			model = glm::translate(model, planets[i].pos);

			shaderInputs(shaders, model, view, projection,
				glm::vec4(planets[i].colour[0], planets[i].colour[1], planets[i].colour[2], 1),
				glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
				true);

			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			if (showVectors) {
				// draw force vectors to other objects
				arrowShader.use();

				model = glm::mat4(1);
				// vector towards sun
				model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
				model = glm::translate(model, planets[i].pos);
				model = glm::rotate(model, generateAngle(-planets[i].pos.x, -planets[i].pos.y), glm::vec3(0, 0, -1));

				shaderInputs(shaders, model, view, projection,
					glm::vec4(planets[i].colour[0], planets[i].colour[1], planets[i].colour[2], 1),
					glm::vec3(1, 1, 1),
					false);

				glBindVertexArray(VAO2);
				glDrawArrays(GL_TRIANGLES, 0, vertices.size());

				// velocity vector of current planet
				model = glm::mat4(1);
				model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
				model = glm::translate(model, planets[i].pos);
				model = glm::rotate(model, generateAngle(planets[i].velocity.x, planets[i].velocity.y), glm::vec3(0, 0, -1));

				shaderInputs(shaders, model, view, projection,
					glm::vec4(planets[i].colour[0], planets[i].colour[1], planets[i].colour[2], 1),
					glm::vec3(1, 1, 1),
					false);

				glBindVertexArray(VAO2);
				glDrawArrays(GL_TRIANGLES, 0, vertices.size());

				// force vector from current vector towards other planets
				for (int j = 0; j < planets.size(); j++)
					if (j != i) {
						model = glm::mat4(1);
						model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
						model = glm::translate(model, planets[i].pos);
						model = glm::rotate(model, generateAngle(planets[j].pos.x - planets[i].pos.x, planets[j].pos.y - planets[i].pos.y), glm::vec3(0, 0, -1));

						shaderInputs(shaders, model, view, projection,
							glm::vec4(planets[i].colour[0], planets[i].colour[1], planets[i].colour[2], 1),
							glm::vec3(1, 1, 1),
							false);

						glBindVertexArray(VAO2);
						glDrawArrays(GL_TRIANGLES, 0, vertices.size());
				}
			}
		}

		// ui boiler plate code
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ui elements
		ImGui::Begin("Menu");
		ImGui::Text("---PLANET CREATE---");
		ImGui::Text("Planet Colour");
		ImGui::ColorPicker3("", temporaryPlanet.colour);
		ImGui::Text("Planet Mass");
		ImGui::SliderFloat("1 - 10", &temporaryPlanet.mass, 1, 10);
		ImGui::Text("Starting Velocity");
		ImGui::SliderFloat("0 - 10", &temporaryVector.z, 0, 10);

		if (ImGui::Button("Choose Planet Position")) {
			choosePlanetPos = true;
			positionChoosen = false;
		}
		if (ImGui::Button("Reselect Starting Velocity"))
			velocityChoosen = false;
		addPlanetClicked = false;
		if (ImGui::Button("Add Planet"))
			addPlanetClicked = true;

		//ImGui::Text(" ");
		ImGui::Text("---SIMULATION VARIABLES---");

		// if not paused, display paused button
		if (!paused && ImGui::Button("Pause Simulation"))
			paused = true;
		// if paused, display continue button
		if (paused && ImGui::Button("Play Simulation"))
			paused = false;

		if (ImGui::Button("Show Vectors"))
			showVectors = !showVectors;

		ImGui::Text("Strength Of Gravity");
		ImGui::SliderFloat("0.005 - 1", &gravity, 0.005, 1);
		ImGui::Text("Timestep Of Simulation");
		ImGui::SliderFloat("0.25 - 3", &timeStep, 0.25, 3);
		ImGui::Text("Sun Mass");
		ImGui::SliderFloat("10 - 100", &sun.mass, 10, 100);
		ImGui::Text("Light Colour");
		ImGui::ColorPicker3(" ", sun.colour);

		// return mouse position and screen size
		glfwGetCursorPos(window, &mxpos, &mypos);
		glfwGetWindowSize(window, &scrnX, &scrnY);
		
		// mxpos is mouse x position, mypos is mouse y position
		// this line makes mxpos and mypos a percentage
		mxpos /= scrnX; mypos /= scrnY; 
		// if mypos = 1, the mouse is at the top of the screen
		// if mypos = 0, the mouse is at the bottom of the screen

		// now mxpos and mypos go between -0.5 and 0.5
		mxpos -= 0.5; mypos -= 0.5;
		// now mxpos and mypos go from -1 to 1
		mxpos *= 2; mypos *= -2;
		
		// if the user is selecting the planet position, draw a planet at the mouse coordinates
		if (choosePlanetPos) {
			shaders.use();

			// if the user clicks set the mouse coordinates as the planet position
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && !positionChoosen) {
				positionChoosen = true;
				temporaryPlanet.pos = glm::vec3(mxpos * _simWidth, mypos * _simHeight, 0);
				// record what the time was when the user clicked
				timeAtClick = glfwGetTime();
			}

			model = glm::mat4(1);
			model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
			// move the temporary planet to mouse pos if position isnt choosen
			if (!positionChoosen)
				model = glm::translate(model, glm::vec3(mxpos * _simWidth, mypos * _simHeight, 0));
			// if the position is choosen dont move the temporary planet
			else
				model = glm::translate(model, temporaryPlanet.pos);
			
			shaderInputs(shaders, model, view, projection, 
				glm::vec4(temporaryPlanet.colour[0], temporaryPlanet.colour[1], temporaryPlanet.colour[2], 1),
				glm::vec3(sun.colour[0], sun.colour[1], sun.colour[2]),
				true);

			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			// once position is chossen draw arrow showing starting velocity the points towards users mouse
			if (positionChoosen) {
				// if "add planet" is clicked and all planet values are entered, add the planet
				if (addPlanetClicked && velocityChoosen && checkPlanet(temporaryPlanet)) {
					choosePlanetPos = false;
					positionChoosen = false;
					velocityChoosen = false;
					// this only normalizes the x and y component of vector
					normalize(temporaryVector);
					// add velocity to temporary planet
					// temporaryVector.z contains the scale of the vector
					temporaryPlanet.velocity.x = temporaryVector.x * temporaryVector.z;
					temporaryPlanet.velocity.y = temporaryVector.y * temporaryVector.z;

					// add new planet to planet vector
					addPlanet(planets, temporaryPlanet);
					// set flag to render planets as true
					planetsExist = true;
					// reset temporary planets values to default
					temporaryPlanet.reset();
				}

				if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS &&
					// add time delay to avoid the user having the mouse clicked for more than one frame
					// resulting in multiple clicks registered when the user only wanted to click once
					glfwGetTime() - timeAtClick > 0.25 &&
					!velocityChoosen) {
					velocityChoosen = true;
					temporaryVector.x = mxpos * _simWidth - temporaryPlanet.pos.x;
					temporaryVector.y = mypos * _simHeight - temporaryPlanet.pos.y;
				}

				// now draw an arrow that points towards users mouse to show starting velocity
				arrowShader.use();
				// draw arrow for starting velocity
				model = glm::mat4(1);
				model = glm::scale(model, glm::vec3(0.05, 0.05, 0.05));
				model = glm::translate(model, temporaryPlanet.pos);
				// rotate arrow to point towards mouse
				if (!velocityChoosen)
					model = glm::rotate(model, generateAngle(mxpos * _simWidth - temporaryPlanet.pos.x, mypos * _simHeight - temporaryPlanet.pos.y), glm::vec3(0, 0, -1));
				else
					model = glm::rotate(model, generateAngle(temporaryVector.x, temporaryVector.y), glm::vec3(0, 0, -1));

				shaderInputs(arrowShader, model, view, projection,
					glm::vec4(colourFunction(temporaryVector.z / 10), 0, 0.75),
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
