#version 400

in Fragment {
    vec4 color;
    float textureCoordinate;
}
fragment;

// uniform sampler1D alphaTexture;

layout(location = 0) out vec4 fragmentColor;

void main() {
    // float a = texture(alphaTexture, fragment.textureCoordinate).r;
    // if (a < 0.05) discard;

    // distance
    // float dist = 0;
    // float fogFactor = 0;

    // dist = length(viewSpace);

    // // 20 - fog starts; 80 - fog ends
    // fogFactor = (80 - dist) / (80 - 20);
    // fogFactor = clamp(fogFactor, 0.0, 1.0);

    // // if you inverse color in glsl mix function you have to
    // // put 1.0 - fogFactor
    // finalColor = mix(fogColor, lightColor, fogFactor);

    fragmentColor = vec4(fragment.color);
}