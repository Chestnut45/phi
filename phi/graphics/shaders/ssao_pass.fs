#version 460

// Sample count
const int SAMPLE_COUNT = 32;

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
};

// Sample kernel block
layout(std140, binding = 2) uniform SSAOBlock
{
    vec4 samples[SAMPLE_COUNT];
};

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gNorm;
layout(binding = 1) uniform usampler2D gMaterial;
layout(binding = 2) uniform sampler2D gDepth;
layout(binding = 3) uniform sampler2D ssaoRotation;

in vec2 texCoords;

out float finalOcclusion;

void main()
{
    // Calculate the scale for tiling the SSAO rotation vectors
    const vec2 tileScale = viewport.zw * 2.0 / textureSize(ssaoRotation);

    // Output final value
    finalOcclusion = 0;
}