#version 460

const float GAMMA = 2.2;

layout(binding = 6) uniform sampler2D hdrTex;

in vec2 texCoords;
layout(location = 0) out vec3 finalColor;

void main()
{
    vec3 hdrColor = texture(hdrTex, texCoords).rgb;
    vec3 toneMapped = hdrColor / (hdrColor + vec3(1.0));
    finalColor = pow(toneMapped, vec3(1.0 / GAMMA));
}