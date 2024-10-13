#version 460 core
layout (location = 0) in vec2 aPos;
uniform mat4 transformMatrix;
uniform vec4 aColor;

out vec4 VertexColor;

void main() {
	VertexColor = aColor;
	gl_Position = transformMatrix * vec4(aPos.xy, 0, 1.0f);
}
