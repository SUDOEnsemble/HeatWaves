#version 400

// // hash & simplex noise from https://www.shadertoy.com/view/Msf3WH
// vec2 hash(vec2 p) {
//     p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
//     return -1. + 2. * fract(sin(p) * 43758.5453123);
// }
// // returns -.5 to 1.5. i think.
// float noise(in vec2 p) {
//     const float K1 = .366025404;  // (sqrt(3)-1)/2;
//     const float K2 = .211324865;  // (3-sqrt(3))/6;

//     vec2 i = floor(p + (p.x + p.y) * K1);

//     vec2 a = p - i + (i.x + i.y) * K2;
//     vec2 o = (a.x > a.y) ? vec2(1., 0.)
//                          : vec2(0.,
//                                 1.);  // vec2 of = 0.5 +
//                                 0.5*vec2(sign(a.x-a.y),
//                                       // sign(a.y-a.x));
//     vec2 b = a - o + K2;
//     vec2 c = a - 1. + 2. * K2;

//     vec3 h = max(.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.);

//     vec3 n =
//         h * h * h * h *
//         vec3(dot(a, hash(i + 0.)), dot(b, hash(i + o)), dot(c, hash(i
//         + 1.)));

//     return dot(n, vec3(70.));
// }
// float noise01(vec2 p) { return clamp((noise(p) + .5) * .5, 0., 1.); }

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 al_ProjectionMatrix;

in Vertex {
    vec4 color;
    float size;
    float normal;
}
vertex[];

out Fragment {
    vec4 color;
    float textureCoordinate;
}
fragment;

void main() {
    mat4 m = al_ProjectionMatrix;  // rename to make lines shorter
    vec4 a = gl_in[0].gl_Position;
    vec4 b = gl_in[1].gl_Position;
    vec4 c = gl_in[2].gl_Position;
    vec4 d = gl_in[3].gl_Position;

    const float r = 0.3;
    vec4 dist;
    vec4 dist2;
    vec4 normal = normalize(vec4(-b.y, c.x, 0.0, 0.0));

    if (vertex[1].normal == 0) {  // miter leading edge if it's a body segment
        vec4 tangent =
            vec4(normalize(normalize(c.xyz - b.xyz) + normalize(b.xyz - a.xyz)),
                 0.0);
        vec4 miter = vec4(-tangent.y, tangent.x, 0.0, 0.0);
        float length = r / dot(miter, normal);
        dist = vec4(normalize(cross(miter.xyz, vec3(0.0, 0.0, 1.0))), 0.0) *
               length;
    } else {
        dist = vec4(normalize(c.xyz - b.xyz), 0.0) * r;  // unmitered head
    }

    if (vertex[2].normal == 0) {  // miter trailing edge if it's a body segment
        vec4 tangent2 =
            vec4(normalize(normalize(d.xyz - c.xyz) + normalize(c.xyz - b.xyz)),
                 0.0);
        vec4 miter2 = vec4(-tangent2.y, tangent2.x, 0.0, 0.0);
        float length = r / dot(miter2, normal);
        dist2 = vec4(normalize(cross(miter2.xyz, vec3(0.0, 0.0, 1.0))), 0.0) *
                length;
    } else {
        dist2 = vec4(normalize(c.xyz - b.xyz), 0.0) * r;  // unmitered tail
    }

    if (vertex[1].normal == 0 || vertex[1].normal == 1) {
        gl_Position = m * (b + dist * vertex[1].size);
        fragment.color = vertex[1].color;
        fragment.textureCoordinate = 0.0;
        EmitVertex();

        gl_Position = m * (b - dist * vertex[1].size);
        fragment.color = vertex[1].color;
        fragment.textureCoordinate = 1.0;
        EmitVertex();

        gl_Position = m * (c + dist2 * vertex[2].size);
        fragment.color = vertex[2].color;
        fragment.textureCoordinate = 0.0;
        EmitVertex();

        gl_Position = m * (c - dist2 * vertex[2].size);
        fragment.color = vertex[2].color;
        fragment.textureCoordinate = 1.0;
        EmitVertex();
    }

    EndPrimitive();
}