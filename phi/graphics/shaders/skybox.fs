#version 460

// Normalized time of day: 0 = noon, 1 = midnight
uniform float timeOfDay;

// Skybox texture samplers
layout(binding = 0) uniform samplerCube dayCube;
layout(binding = 1) uniform samplerCube nightCube;

// Direction vector
in vec3 texCoords;

out vec4 finalColor;

void main()
{
    // Sample both skyboxes directly
    vec4 dayTexel = texture(dayCube, texCoords);
    vec4 nightTexel = texture(nightCube, texCoords);

    // Blend both texels based on normalized time of day and influence from sun + moon
    finalColor = mix(dayTexel, nightTexel, timeOfDay);
}