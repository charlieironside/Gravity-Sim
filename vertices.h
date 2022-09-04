#pragma once
#include <glm/glm.hpp>
#include <vector>

// Stores data written from source
std::vector<float>vertices;
// indices
std::vector<unsigned int>indices;

// Vertices for triangular pyramid
std::vector<float> vertexData = {
	0.0, 0.0, 1.732051, 1.632993, 0.0, - 0.5773503, -0.8164966, 1.414214, -0.5773503,
	0.0, 0.0, 1.732051, -0.8164966, 1.414214, -0.5773503, -0.8164966, -1.414214, -0.5773503,
	0.0, 0.0, 1.732051, -0.8164966, -1.414214, -0.5773503, 1.632993, 0.0, -0.5773503,
	1.632993, 0.0, -0.5773503, -0.8164966, -1.414214, -0.5773503, -0.8164966, 1.414214, -0.5773503,
};