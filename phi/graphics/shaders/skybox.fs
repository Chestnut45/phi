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

// Blend factor between day (0) and night (1)
uniform float blendFactor;
uniform vec2 lightPos;

// Skybox texture samplers
layout(binding = 0) uniform samplerCube dayCube;
layout(binding = 1) uniform samplerCube nightCube;

in vec2 texCoords;
out vec4 finalColor;

vec3 getSky(vec2 uv)
{
    vec3 cameraUp = invView[1].xyz;
    vec3 cameraRight = invView[0].xyz;
    float horizonHeight = 1.0 - uv.y;
    float atmosphere = sqrt(horizonHeight);
    vec3 skyColor = vec3(0.2, 0.4, 0.8);
    
    float scatter = pow(lightPos.y, 1.0 / 15.0);
    scatter = 1.0 - clamp(scatter, 0.8, 1.0);
    
    vec3 scatterColor = mix(vec3(1.0), vec3(1.0, 0.3, 0.0) * 1.5, scatter);
    return mix(skyColor, vec3(scatterColor), atmosphere / 1.3);
}

vec3 getSun(vec2 uv)
{
	float sun = 1.0 - distance(uv, lightPos);
    sun = clamp(sun, 0.0, 1.0);

    float glow = sun;
    glow = clamp(glow, 0.0, 1.0);
    sun = pow(sun, 100.0);
    sun *= 100.0;
    sun = clamp(sun, 0.0, 1.0);
    glow = pow(glow, 6.0) * 1.0;
    glow = pow(glow, (uv.y));
    glow = clamp(glow, 0.0, 1.0);
    sun *= pow(dot(uv.y, uv.y), 1.0 / 1.65);
    glow *= pow(dot(uv.y, uv.y), 1.0 / 2.0);
    sun += glow;
    
    return vec3(1.0, 0.6, 0.05) * sun;
}

void main()
{
    // Calculate unnormalized world-space direction to fragment
    // NOTE: invView is cast to mat3 to remove translation
    vec4 direction = invProj * vec4(texCoords, -1.0, 1.0);
    direction.xyz = mat3(invView) * direction.xyz;

    // Sample both skyboxes
    vec4 dayTexel = texture(dayCube, direction.xyz);
    vec4 nightTexel = texture(nightCube, direction.xyz);

    // Mix both samples using the blend factor
    finalColor = mix(dayTexel, nightTexel, blendFactor);

    // DEBUG: Testing new procedural sky
    // vec2 uv = gl_FragCoord.xy / (viewport.w * 2.0);
    // finalColor = vec4(getSky(uv) + getSun(uv), 1.0);
}