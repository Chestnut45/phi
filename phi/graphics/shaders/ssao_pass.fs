#version 460

// Sample count
const int SAMPLE_COUNT = 16;

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

// Sample kernel block
layout(std140, binding = 2) uniform SSAOBlock
{
    vec4 samples[SAMPLE_COUNT];
};

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gNorm;
layout(binding = 3) uniform sampler2D gDepth;
layout(binding = 4) uniform sampler2D ssaoRotation;

in vec2 texCoords;

out float finalOcclusion;

// Gets the view position from texCoords and depth
vec3 getViewPos(vec2 texCoords, float depth)
{
    // Calculate view space position
    vec4 viewSpacePos = invProj * vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Perspective division
    viewSpacePos.xyz /= viewSpacePos.w;
    return viewSpacePos.xyz;
}

// Linearizes a depth value based on the camera's near and far plane distances
float linearizeDepth(float depth)
{
    return nearFar.x * nearFar.y / (nearFar.y - depth * (nearFar.y - nearFar.x));
}

void main()
{
    // Calculate the scale for tiling the SSAO rotation vectors
    const vec2 tileScale = viewport.zw * 2.0 / textureSize(ssaoRotation, 0);

    // Grab data from gbuffer textures
    float depth = texture(gDepth, texCoords).r;
    vec3 fragPos = getViewPos(texCoords, depth);
    vec3 fragNorm = normalize((view * vec4(texture(gNorm, texCoords).xyz, 0.0)).xyz);
    vec3 rotation = vec3(texture(ssaoRotation, texCoords * tileScale).rg, 0.0);

    // Construct TBN matrix
    vec3 tangent = normalize(rotation - fragNorm * dot(rotation, fragNorm));
    vec3 bitangent = cross(fragNorm, tangent);
    mat3 tbn = mat3(tangent, bitangent, fragNorm);

    // Sample around the fragment in view space
    float occlusion = 0.0;
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // Calculate view space sample position
        const float radius = 0.5;
        vec3 samplePos = tbn * samples[i].xyz * radius + fragPos;

        // Project the sample coordinates
        vec4 sampleCoords = proj * vec4(samplePos, 1.0);
        sampleCoords.xyz /= sampleCoords.w;
        sampleCoords.xyz = sampleCoords.xyz * 0.5 + 0.5;

        // Sample depth
        float sampleDepth = -linearizeDepth(texture(gDepth, sampleCoords.xy).r);

        // Add occlusion with range check
        occlusion += (sampleDepth >= samplePos.z ? 1.0 : 0.0) * smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
    }

    // Output final value
    finalOcclusion = 1.0 - (occlusion / SAMPLE_COUNT);
}