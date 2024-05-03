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

// Normalized time of day: 0 = noon, 1 = midnight
uniform float timeOfDay;

// Skybox texture samplers
layout(binding = 0) uniform samplerCube dayCube;
layout(binding = 1) uniform samplerCube nightCube;

in vec2 texCoords;

out vec4 finalColor;

void main()
{
    // Calculate direction to fragment
    vec3 direction = (invViewProj * vec4(texCoords, 1.0, 1.0)).xyz;

    // Sample both skyboxes
    vec4 dayTexel = texture(dayCube, direction);
    vec4 nightTexel = texture(nightCube, direction);

    // Blend both texels based on normalized time of day
    finalColor = mix(dayTexel, nightTexel, timeOfDay);
}