#pragma once

class ShaderSource
{
public:
	const  char* VertexFrame = {
		"#version 460 core\n"
		"layout(location = 0) in vec2 aPos;\n"
		"uniform mat4 transformMatrix;\n"
		"uniform vec4 aColor;\n"

		"out vec4 VertexColor;\n"

		"void main() {\n"
			"VertexColor = aColor;\n"
			"gl_Position = transformMatrix * vec4(aPos.xy, 0, 1.0f);\n"
		"}\0"
	};

	const char* VertexParticle = {
		"#version 460 core\n"
		"layout(location = 0) in vec2 aPos;\n"
		"layout(location = 1) in vec2 aVelocity;\n"
		"layout(location = 2) in vec4 aColorFast;\n"
		"layout(location = 3) in vec4 aColorSlow;\n"
		"uniform mat4 transformMatrix;\n"
		"uniform float velocityMax;\n"

		"out vec4 VertexColor;\n"

		"void main() {\n"
			"float colorVelocity = sqrt(pow(aVelocity.x, 2) + pow(aVelocity.y, 2)) * (1 / velocityMax);\n"
			"VertexColor = colorVelocity * aColorFast + (1 - colorVelocity) * aColorSlow;\n"
			"gl_Position = transformMatrix * vec4(aPos, 0, 1.0f);\n"
		"}\0"
	};

	const char* FragmentParticle = {
		"#version 460 core\n"
		"in vec4 VertexColor;\n"
		"out vec4 FragColor;\n"

		"void main() {\n"
			"FragColor = VertexColor;\n"
		"}\0"
	};

	const char* FragmentFrame = {
		"#version 460 core\n"
		"in vec4 VertexColor;\n"
		"out vec4 FragColor;\n"

		"void main() {\n"
			"FragColor = VertexColor;\n"
		"}\0"
	};
};
