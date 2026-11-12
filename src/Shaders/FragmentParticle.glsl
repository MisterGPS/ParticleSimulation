#version 460 core
in vec4 VertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(VertexColor.rgb * 0.15, 1);
}
