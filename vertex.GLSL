#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 normals;
out vec3 FragPos;

uniform mat4 proj;
uniform mat4 model;
uniform mat4 view;

void main() {
	FragPos = vec3(model * vec4(aPos, 1.0));
	normals = aPos;
	gl_Position = proj * view * model * vec4(aPos, 1.0);
}