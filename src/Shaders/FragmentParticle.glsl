#version 460 core
in vec4 VertexColor;
out vec4 FragColor;

void main() {
    float r = length(gl_PointCoord - 0.5) * 2.0;
    FragColor = vec4(VertexColor.rgb * 0.5 * (1.0 - r), 1);
}
