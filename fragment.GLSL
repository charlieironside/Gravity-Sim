#version 330 core
out vec4 FragColour;

struct line {
	float m, c;
};

// Returns true if point falls between two lines
bool linearSolver(line eq1, line eq2, vec2 p) {
	if (eq1.m * p.x + eq1.c > p.y) {
		if (eq2.m * p.x < p.y)
			return true;

		else if (eq2.m * p.x + eq2.c > p.y)
			return false;
	}

	else if (eq1.m * p.x + eq1.c < p.y) {
		if (eq2.m * p.x > p.y)
			return true;

		else if (eq2.m * p.x * eq2.c < p.y)
			return false;
	}

	else if (eq1.m * p.x + eq1.c == p.y)
		return true;

	else if (eq2.m * p.x + eq2.c == p.y)
		return true;
};

in vec3 FragPos;
in vec3 normals;

uniform vec3 viewPos;
uniform vec3 lightColour;
uniform vec3 lightPos;
uniform vec3 ambient;
uniform bool lightSource;

uniform int shadowSize;
uniform float mShadow[2];
uniform float cShadow[2];

void main() {
	
	if (!lightSource) {
		bool inShadow = false;
		for (int i = 0; i < shadowSize; i += 2) {
			line aLine, bLine;
			aLine.m = mShadow[i];
			aLine.c = cShadow[i];
			bLine.m = mShadow[i+1];
			bLine.c = cShadow[i+1];

			inShadow = linearSolver(aLine, bLine, vec2(gl_FragCoord.x, gl_FragCoord.y));
			}

		if (!inShadow) {
			// LIGHTING
			vec3 viewDirection = normalize(viewPos - FragPos);
			vec3 lightDirection = normalize(lightPos - FragPos);

			// Diffuse light
			float diffuse = max(dot(lightDirection, normals), 0) * 0.5;

			// Specular light
			float specular = pow(max(dot(reflect(-lightDirection, normals), viewDirection), 0.0), 16);

			vec3 lightingStrength = specular * lightColour + diffuse * lightColour;
			FragColour = vec4(lightingStrength * ambient, 1.0);
			//FragColour = vec4(vec3(0.5), 1.0);
		}
		else 
			FragColour = vec4(vec3(1), 1);
	
	}
	else
		FragColour = vec4(ambient, 1);
}	