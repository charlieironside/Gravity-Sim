#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <vector>

#include "Shader.h"

#define squared(x) pow(x, 2)
// acts as a 3 variable term
#define variable(a, b, c) (a * b * c)

// NULL for vec3
#define GLM_NULL glm::vec3(0, 0, -100000)

#define _a_ (m + 1)
#define _b_ (-a + variable(2, m, c) - variable(2, m, b))
#define _c_ (squared(c) - variable(2, b, c) + squared(b) + squared(a) - squared(r))

struct vec3 {
	float x, y, z;
};

struct line {
	float m, c;
};

namespace functions {
	struct planet {
		glm::vec3 pos = GLM_NULL;
		float colour[3]{0,0,0}, radius = 0;
		float mass = 0;
		bool lightSource = false;
		char name[512] = "";
	};

	// called before placing planet to ensure each variable has a value
	void checkPlanet(planet* pList, planet p) {
		// check each value of p has been filled in
		if (p.pos == GLM_NULL ||
			p.radius == 0 ||
			p.mass == 0 ||
			p.name == "")
			printf("ERROR WITH PLANET VALUES\n");

	}

	// find coordinates of planet on simulation plane
	// camera position, mouse x coordinate, mouse y coordinate
	glm::vec3 planetPlacementCoordinates(glm::vec3 camPos, float mx, float my) {
		// line x in terms of z and y in terms of z
		line xz, yz;
		xz.c = camPos.z;
		yz.c = camPos.z;
		// solve gradient of line with dy/dx
		mx - camPos.x != 0 ? xz.m = (mx - camPos.x) / -camPos.z: xz.m = 0;
		my - camPos.y != 0 ? yz.m = (my - camPos.y) / -camPos.z : yz.m = 0;

		return glm::vec3(xz.m * (0) + xz.c, yz.m * (0) + yz.c, 0);
	}

	typedef glm::vec3 Triangle[3];

	// Generates shadow
	void shadowSolver(glm::vec2 lightPos, glm::vec2 obstruction, std::vector<line>&vec, float r, bool firstRun) {
		line perpindicularLine;
		line shadow[2];

		glm::vec2 outerPoints[2];

		float lightToObstructionGradient;
		float perpindicularGradient;
		
		// ensure denomenator (delta x) does not equal 0
		lightPos.x - obstruction.x != 0 ? lightToObstructionGradient = lightToObstructionGradient = (lightPos.y - obstruction.y) / (lightPos.x - obstruction.x) : lightToObstructionGradient = 0;
		perpindicularGradient = -1 / lightToObstructionGradient;

		perpindicularLine.c = -perpindicularGradient * obstruction.x + obstruction.y;

		// (x - a)^2 + (x - b)^2 - r^2 = 0
		float a = obstruction.x, b = obstruction.y;
		float m = perpindicularGradient, c = perpindicularLine.c;

		// Find points on outer edges of circle
		// Solve intersections between line and circle
		outerPoints[0].x = (-_b_ + sqrt(squared(_b_) - variable(4, _a_, _c_))) /
								(variable(2, a, c));
		outerPoints[1].x = (-_b_ - sqrt(squared(_b_) - variable(4, _a_, _c_))) /
								(variable(2, a, c));

		// Gradient of line from light to outer points
		lightPos.x - outerPoints[0].x != 0 ? shadow[0].m = (lightPos.y - outerPoints[0].y) / (lightPos.x - outerPoints[0].x) : shadow[0].m = 0;
		lightPos.x - outerPoints[1].x != 0 ? shadow[1].m = (lightPos.y - outerPoints[1].y) / (lightPos.x - outerPoints[1].x) : shadow[1].m = 0;

		// y intercept
		shadow[0].c = -shadow[0].m * outerPoints[0].x + outerPoints[0].y;
		shadow[1].c = -shadow[1].m * outerPoints[1].x + outerPoints[1].y;

		if (firstRun)
			vec.clear();
		vec.push_back(shadow[0]);
		vec.push_back(shadow[1]);
	}

	void shaderInputs (
		Shader& shader,
		glm::mat4 model,
		glm::mat4 projection,
		glm::mat4 view,
		glm::vec3 viewPos,
		glm::vec3 lightColour,
		glm::vec3 lightPos,
		glm::vec3 baseColour,
		bool lightSource,
		std::vector<line>shadow)
	{
		shader.setMat4("model", model);
		shader.setMat4("view", view);
		shader.setMat4("proj", projection);

		shader.setVec3("viewPos", viewPos);
		shader.setVec3("lightColour", lightColour);
		shader.setVec3("lightPos", lightPos);
		shader.setVec3("ambient", baseColour);
		shader.setBool("lightSource", lightSource);

		const char* mName = "mShadow";
		const char* cName = "cShadow";
		
		std::vector<float>mShadow;
		std::vector<float>cShadow;
		//for (int i = 0; i < shadow.size(); i++) {
		//	mShadow.push_back(shadow[i].m);
		//	cShadow.push_back(shadow[i].c);
		//}

		//glUniform1fv(glGetUniformLocation(shader.ID, mName), mShadow.size(), &mShadow[0]);
		//glUniform1fv(glGetUniformLocation(shader.ID, cName), cShadow.size(), &cShadow[0]);
		//shader.setInt("shadowSize", shadow.size());
	}

	// Finds midpoint of one side of the triangle, returns vec3 of midpoint
	glm::vec3 findMidpoint(glm::vec3 s, glm::vec3 e) {
		glm::vec3 result;

		result.x = (s.x + e.x) / 2;
		result.y = (s.y + e.y) / 2;
		result.z = (s.z + e.z) / 2;

		return result;
	}

	// Simply normalizes each vertice
	void circleGeneration(std::vector<float>& vertices) {
		for (int i = 0; i < vertices.size(); i += 3) {
			float vectorLength = sqrt((vertices[i] * vertices[i]) + (vertices[i + 1] * vertices[i + 1]) + (vertices[i + 2] * vertices[i + 2]));
			vertices[i] /= vectorLength;
			vertices[i + 1] /= vectorLength;
			vertices[i + 2] /= vectorLength;
		}
	}

	// Generates 4 smaller triangles inside a larger one
	// Data vector is vertex data of triangle to split up, vertices is where the data is to be copied to
	void triangleGeneration(std::vector<float>data, std::vector<float>& vertices, int iterations) {
		iterations -= 1;

		Triangle newTriangle;
		Triangle originalTriangle;

		Triangle middleTriangle, topTriangle, leftTriangle, rightTriangle;

		// Load vertices data into another triangle
		originalTriangle[0].x = data[0];
		originalTriangle[0].y = data[1];
		originalTriangle[0].z = data[2];

		originalTriangle[1].x = data[3];
		originalTriangle[1].y = data[4];
		originalTriangle[1].z = data[5];

		originalTriangle[2].x = data[6];
		originalTriangle[2].y = data[7];
		originalTriangle[2].z = data[8];

		// Generate new triangle, each element of tempTriangle contains a single vertex
		newTriangle[0] = findMidpoint(glm::vec3(data[0], data[1], data[2]), glm::vec3(data[6], data[7], data[8]));
		newTriangle[1] = findMidpoint(glm::vec3(data[0], data[1], data[2]), glm::vec3(data[3], data[4], data[5]));
		newTriangle[2] = findMidpoint(glm::vec3(data[3], data[4], data[5]), glm::vec3(data[6], data[7], data[8]));

		// MIDDLE TRIANGLE
		vertices.push_back(newTriangle[0].x);
		vertices.push_back(newTriangle[0].y);
		vertices.push_back(newTriangle[0].z);
		middleTriangle[0] = glm::vec3(newTriangle[0].x, newTriangle[0].y, newTriangle[0].z);

		vertices.push_back(newTriangle[1].x);
		vertices.push_back(newTriangle[1].y);
		vertices.push_back(newTriangle[1].z);
		middleTriangle[1] = glm::vec3(newTriangle[1].x, newTriangle[1].y, newTriangle[1].z);

		vertices.push_back(newTriangle[2].x);
		vertices.push_back(newTriangle[2].y);
		vertices.push_back(newTriangle[2].z);
		middleTriangle[2] = glm::vec3(newTriangle[2].x, newTriangle[2].y, newTriangle[2].z);

		// BOTTOM LEFT TRIANGLE
		vertices.push_back(originalTriangle[0].x);
		vertices.push_back(originalTriangle[0].y);
		vertices.push_back(originalTriangle[0].z);
		leftTriangle[0] = glm::vec3(originalTriangle[0].x, originalTriangle[0].y, originalTriangle[0].z);

		vertices.push_back(newTriangle[1].x);
		vertices.push_back(newTriangle[1].y);
		vertices.push_back(newTriangle[1].z);
		leftTriangle[1] = glm::vec3(newTriangle[1].x, newTriangle[1].y, newTriangle[1].z);

		vertices.push_back(newTriangle[0].x);
		vertices.push_back(newTriangle[0].y);
		vertices.push_back(newTriangle[0].z);
		leftTriangle[2] = glm::vec3(newTriangle[0].x, newTriangle[0].y, newTriangle[0].z);

		// TOP TRIANGLE
		vertices.push_back(newTriangle[1].x);
		vertices.push_back(newTriangle[1].y);
		vertices.push_back(newTriangle[1].z);
		topTriangle[0] = glm::vec3(newTriangle[1].x, newTriangle[1].y, newTriangle[1].z);

		vertices.push_back(originalTriangle[1].x);
		vertices.push_back(originalTriangle[1].y);
		vertices.push_back(originalTriangle[1].z);
		topTriangle[1] = glm::vec3(originalTriangle[1].x, originalTriangle[1].y, originalTriangle[1].z);

		vertices.push_back(newTriangle[2].x);
		vertices.push_back(newTriangle[2].y);
		vertices.push_back(newTriangle[2].z);
		topTriangle[2] = glm::vec3(newTriangle[2].x, newTriangle[2].y, newTriangle[2].z);

		// BOTOTM RIGHT TRIANGLE
		vertices.push_back(newTriangle[0].x);
		vertices.push_back(newTriangle[0].y);
		vertices.push_back(newTriangle[0].z);
		rightTriangle[0] = glm::vec3(newTriangle[0].x, newTriangle[0].y, newTriangle[0].z);

		vertices.push_back(newTriangle[2].x);
		vertices.push_back(newTriangle[2].y);
		vertices.push_back(newTriangle[2].z);
		rightTriangle[1] = glm::vec3(newTriangle[2].x, newTriangle[2].y, newTriangle[2].z);

		vertices.push_back(originalTriangle[2].x);
		vertices.push_back(originalTriangle[2].y);
		vertices.push_back(originalTriangle[2].z);
		rightTriangle[2] = glm::vec3(originalTriangle[2].x, originalTriangle[2].y, originalTriangle[2].z);

		if (iterations > 0) {
			// Temporary vector that passes triangle data to recursive triangleGenertion
			std::vector<float>nextTriangle;

			nextTriangle.clear();
			for (int i = 0; i < 3; i++) {
				nextTriangle.push_back(middleTriangle[i].x);
				nextTriangle.push_back(middleTriangle[i].y);
				nextTriangle.push_back(middleTriangle[i].z);
			}
			// recursize call for middle sub-triangle
			triangleGeneration(nextTriangle, vertices, iterations);

			// Clear the triangle data then load in next triangle
			nextTriangle.clear();
			for (int i = 0; i < 3; i++) {
				nextTriangle.push_back(leftTriangle[i].x);
				nextTriangle.push_back(leftTriangle[i].y);
				nextTriangle.push_back(leftTriangle[i].z);
			}
			// recursize call for left sub-triangle
			triangleGeneration(nextTriangle, vertices, iterations);

			nextTriangle.clear();
			for (int i = 0; i < 3; i++) {
				nextTriangle.push_back(topTriangle[i].x);
				nextTriangle.push_back(topTriangle[i].y);
				nextTriangle.push_back(topTriangle[i].z);
			}
			triangleGeneration(nextTriangle, vertices, iterations);

			nextTriangle.clear();
			for (int i = 0; i < 3; i++) {
				nextTriangle.push_back(rightTriangle[i].x);
				nextTriangle.push_back(rightTriangle[i].y);
				nextTriangle.push_back(rightTriangle[i].z);
			}
			triangleGeneration(nextTriangle, vertices, iterations);
		}
	}
}