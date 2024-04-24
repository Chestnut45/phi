#version 460

const float GAMMA = 2.2;
const int MAX_MATERIALS = 1024;

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

// Basic material definition
struct BasicMaterial
{
    vec4 colorShininess;
};

// Basic material buffer
layout(std430, binding = 1) buffer BasicMaterialBlock
{
    BasicMaterial basicMaterials[MAX_MATERIALS];
};

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gNorm;
layout(binding = 1) uniform usampler2D gMaterial;
layout(binding = 2) uniform sampler2D gDepth;

in flat vec3 lightPosWorld;
in flat vec4 lightColorRadius;

out vec4 finalColor;

// Gets the world position from texCoords and depth
vec3 getWorldPos(vec2 texCoords, float depth)
{
    // Calculate view space position
    vec4 viewSpacePos = invProj * vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Perspective division
    viewSpacePos /= viewSpacePos.w;

    // Calculate world space position
    vec4 worldSpacePos = invView * viewSpacePos;
    return worldSpacePos.xyz;
}

void main()
{
    // Calculate texture coordinates
    vec2 texCoords = (gl_FragCoord.xy - viewport.xy) * 0.5 / viewport.zw;

    // Grab data from geometry buffer
    float depth = texture(gDepth, texCoords).r;
    vec3 fragNorm = normalize(texture(gNorm, texCoords).xyz);
    uint materialID = texture(gMaterial, texCoords).r;

    // Calculate fragment position in world space
    vec3 fragPos = getWorldPos(texCoords, depth);

    // Grab material
    BasicMaterial material = basicMaterials[materialID];

    // Gamma correct and clamp the material's color and shininess values
    vec3 materialColor = pow(material.colorShininess.rgb, vec3(GAMMA));
    float materialShininess = pow(material.colorShininess.w, GAMMA);

    // Extract light data
    vec3 lightColor = lightColorRadius.rgb;
    float lightRadius = lightColorRadius.a;

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos.xyz - fragPos);

    // Calculate alignment to light
    vec3 lightDir = normalize(lightPosWorld - fragPos);
    float alignment = dot(fragNorm, lightDir);

    // Diffuse lighting
    vec3 diffuse = max(alignment, 0.0) * lightColor * materialColor;

    // Specular reflections
    vec3 lightHalfDir = normalize(lightDir + viewDir);
    float specLight = max(pow(max(dot(fragNorm, lightHalfDir), 0.0), materialShininess * 256.0), 0.0);
    vec3 specular = specLight * lightColor * vec3(materialShininess);

    // Apply attenuation
    float dist = distance(fragPos, lightPosWorld);
    float attenuation = clamp(1.0 - dist / lightRadius, 0.0, 1.0);
    attenuation *= attenuation;

    // Final color composition
    finalColor = vec4(pow((diffuse + specular) * attenuation, vec3(1.0 / GAMMA)), 1.0);
}