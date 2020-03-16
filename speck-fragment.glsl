#version 400

in Fragment { vec4 color; }
fragment;

layout(location = 0) out vec4 fragmentColor;

void main() {
  //
  fragmentColor = fragment.color;
}