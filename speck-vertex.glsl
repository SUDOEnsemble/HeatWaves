#version 400

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

out Vertex { vec4 color; }
vertex;

void main() {
  gl_Position = al_ModelViewMatrix * vec4(vertexPosition, 1.0);
  vertex.color = vertexColor;
}