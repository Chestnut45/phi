#version 460

// Camera uniform block
layout(std140, binding = 0) uniform CameraBlock
{
    mat4 viewProj;
    mat4 invViewProj;
    mat4 view;
    mat4 invView;
    mat4 proj;
    mat4 invProj;
    vec4 cameraPos;
    vec4 viewport; // (x, y, width / 2, height / 2)
    vec4 nearFar; // x = near, y = far, z = null, w = null
};

layout(binding = 5) uniform sampler2D sunTexture;

uniform vec3 sunColor;

in vec2 texCoords;

out vec4 finalColor;

void main()
{
    finalColor = vec4(sunColor, 1.0) * texture(sunTexture, texCoords);
}