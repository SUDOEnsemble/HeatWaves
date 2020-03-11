#version 400

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color; // a not used
layout(location = 2) in vec2 tex; // s,t not used
layout(location = 3) in vec3 normal;

out Vertex {
  vec3 position;
  vec3 forward;
  vec3 up;
  //vec3 color;
}
vertex;

void main() {
  vertex.position = position;
  vertex.forward = normal;
  vertex.up = color.rgb;
  // vertex.color = vec3(color.a, tex.s, tex.t);
}
