#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aVelocity;
layout (location = 2) in vec4 aColorFast;
layout (location = 3) in vec4 aColorSlow;
uniform mat4 transformMatrix;
uniform float velocityMax;

out vec4 VertexColor;

void main() {
    float colorVelocity = clamp(sqrt(pow(aVelocity.x, 2) + pow(aVelocity.y, 2)) * (1 / velocityMax), 0.0, 1.0);
    VertexColor = colorVelocity * aColorFast + (1 - colorVelocity) * aColorSlow;
    gl_Position = transformMatrix * vec4(aPos, 0, 1.0f);
}
