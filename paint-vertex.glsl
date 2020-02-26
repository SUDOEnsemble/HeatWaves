#version 400

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec2 vertexTexture;

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

out Vertex {
    vec3 position;
    vec3 forward;
    vec3 up;
    // vec3 color;
}
vertex;

void main() {
    gl_Position =
        al_ProjectionMatrix * al_ModelViewMatrix * vec4(vertexPosition, 1.0);
    vertex.color = vertexColor;
    vertex.size = vertexSize.x;
}
