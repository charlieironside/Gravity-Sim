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
#define pi 3.14159

// NULL for vec3
#define GLM_NULL glm::vec3(-1000, -1000, -1000)

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
	// generates a RG colour value for starting velocity vector
	// returns 2 values, x = red, y = green
	glm::vec2 colourFunction(float x) {
		return glm::vec2(
			-log(x + 0.05) + (2 * sin(x + (pi / 3)) - 1.75),
			-log(1.1 - x) + (2 * sin(x + (pi / 3)) - 1.75)
		);
	}

	// generates an angle to rotate a matrix based on x and y inputs
	// change in width, change in height
	float generateAngle(float mx, float my) {
		// top half
		if (my >= 0) {
			return atan(mx / my);
		}
		// bottom half
		if (my <= 0) {
			return atan(mx / my) + glm::radians(180.0);
		}
	}

	class planet {
	public:
		glm::vec3 pos = GLM_NULL;
		// velocity is added per frame so its essentially acceleration
		glm::vec3 velocity = glm::vec3(0);
		float colour[3]{1,1,1};
		float mass = 1;

		// function to reset planets values to nulls
		void reset() {
			pos = GLM_NULL;
			colour[0] = 0; colour[1] = 0; colour[2] = 0;
			mass = 0;
			//memset(name, 0, sizeof(name));
		}
	};

	// called before placing planet to ensure each variable has a value
	// returns true if checks pass
	bool checkPlanet(planet p) {
		// check each value of p has been filled in
		if (p.pos == GLM_NULL) {
			return false;
		}

		return true;
	}

	typedef glm::vec3 Triangle[3];

	void shaderInputs(
		Shader& shader,
		glm::mat4 model,
		glm::mat4 view,
		glm::mat4 proj,
		// object light colour
		glm::vec4 colour,
		// background light colour
		glm::vec3 lColour,
		// dont blend colours of force vectors
		bool blendColours)
	{
		shader.setMat4("model", model);
		shader.setMat4("view", view);
		shader.setMat4("proj", proj);
		shader.setVec4("colour", colour);
		shader.setVec3("lightColour", lColour);
		shader.setBool("blend", blendColours);
	}

	// Finds midpoint of one side of the triangle, returns vec3 of midpoint
	inline glm::vec3 findMidpoint(glm::vec3 s, glm::vec3 e) {
		glm::vec3 result;

		result.x = (s.x + e.x) / 2;
		result.y = (s.y + e.y) / 2;
		result.z = (s.z + e.z) / 2;

		return result;
	}

	// function that only normalizes x and y component of a vec3
	void normalize(glm::vec3& v) {
		float length = sqrt(pow(v.x, 2) + pow(v.y, 2));
		v.x /= length;
		v.y /= length;
	}

	// Simply normalizes each vertice
	void circleGeneration(std::vector<float>& vertices) {
		for (int i = 0; i < vertices.size(); i += 3) {
			float vectorLength = sqrt(pow(vertices[i], 2) + pow(vertices[i + 1], 2) + pow(vertices[i + 2], 2));
			vertices[i] /= vectorLength;
			vertices[i + 1] /= vectorLength;
			vertices[i + 2] /= vectorLength;
		}
	}

	// finds length of a vector
	inline float hypo(glm::vec3 dist) {
		// pythag
		return sqrt(pow(dist.x, 2) + pow(dist.y, 2));
	}

	// apply a force to a planet
	// planet refrence, force to apply, delta time, time step
	void applyForce(planet& p, glm::vec3 dir, float force, float dt, float ts) {
		p.velocity += (force * glm::normalize(dir)) / p.mass;
		//printf("vel: %f %f %f\n", p.velocity.x, p.velocity.y, p.velocity.z);
		p.pos += p.velocity * dt * ts;
	}

	// check collisions of all planets
	void checkCollisions(std::vector<planet>& p, float dt, float i, float j) {
		// planets have radius of 1
		if (hypo(p[i].pos - p[j].pos) <= 2) {
			// if theres a collision apply a force in the opposing direction of the planet
			p[i].velocity *= -1;
		}
	}

	// called once per frame to calculate physics
	// planet array, sun, delta time, gravity strength, time step
	void physics(std::vector<planet>& p, planet sun, float dt, float g, float ts) {
		for (int i = 0; i < p.size(); i++) {
			// force from center object
			float gForce = (g / 100) * ((p[i].mass * sun.mass) / (hypo(p[i].pos) * hypo(p[i].pos)));
			//printf("%f %f\n", p[i].pos.x, p[i].pos.y);
			applyForce(p[i], -p[i].pos, gForce, dt, ts);

			// check collision with sun
			if (hypo(p[i].pos) <= 2) {
				p[i].velocity *= -1;
				// after a collision move object to surface of sun so planet wont get stuck inside sun between frames
				p[i].pos = glm::normalize(p[i].pos) * 2.0f;
			}

			// loop through every other planet
			for (int j = 0; j < p.size(); j++) {
				// dont check for collision on the same planet
				if (j != i) {
					checkCollisions(p, dt, i, j);
					// calculate x and y components of gravity force vector
					gForce = g * ((p[i].mass * p[j].mass) / pow(hypo(p[i].pos - p[j].pos), 2));
					// apply force to planet[i]
					applyForce(p[i], p[j].pos, gForce, dt, ts);
				}
			}
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
