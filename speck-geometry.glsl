#version 400

// take in a point and output a triangle strip with 4 vertices (aka a "quad")
//
layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 al_ProjectionMatrix;
uniform float pointSize;

in Vertex { vec4 color; }
vertex[];

out Fragment { vec4 color; }
fragment;

void main() {
    mat4 m = al_ProjectionMatrix;   // rename to make lines shorter
    vec4 v = gl_in[0].gl_Position;  // al_ModelViewMatrix * gl_Position

    float r = pointSize;

    gl_Position = m * (v + vec4(-r, 0.0, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(r * -0.5, r * 0.866, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(r * -0.5, r * -0.866, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(r * 0.5, r * 0.866, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(r * 0.5, r * -0.866, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    gl_Position = m * (v + vec4(r, 0.0, 0.0, 0.0));
    fragment.color = vertex[0].color;
    EmitVertex();

    EndPrimitive();
}
