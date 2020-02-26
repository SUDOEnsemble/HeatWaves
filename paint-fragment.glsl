#version 400

in Fragment {
    vec4 color;
    float textureCoordinate;
}
fragment;

uniform sampler1D alphaTexture;

layout(location = 0) out vec4 fragmentColor;

void main() {
    float a = texture(alphaTexture, fragment.textureCoordinate).r;
    if (a < 0.05) discard;
    fragmentColor = vec4(fragment.color.xyz, a);
}