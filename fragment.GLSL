#version 330 core

out vec4 FragColor;

uniform vec4 colour;
uniform vec3 lightColour;
uniform bool blend;

void main() {
	if (blend)
		FragColor = colour * vec4(lightColour, 1);
	else
		FragColor = colour;
}
