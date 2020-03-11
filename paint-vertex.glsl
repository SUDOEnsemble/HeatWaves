#version 400

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2
    vertexSize;  // as a 2D texture cordinate, but we ignore the y
layout(location = 3)
    in vec3 normal;  // only x is used (to report where in a strip we are)

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

out Vertex {
    vec4 color;
    float size;
    float normal;
}
vertex;

void main() {
    gl_Position = al_ModelViewMatrix * vec4(vertexPosition, 1.0);
    vertex.normal = normal.x;
    vertex.color = vertexColor;
    vertex.size = vertexSize.x;
}