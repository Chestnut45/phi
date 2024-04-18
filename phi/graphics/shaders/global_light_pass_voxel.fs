#version 460

const int MAX_DIRECTIONAL_LIGHTS = 4;
const int MAX_MATERIALS = 1024;
const float GAMMA = 2.2;

// Light structure
struct DirectionalLight
{
    vec4 color;
    vec4 directionAmbient;
};

// Voxel material definition
struct VoxelMaterial
{
    vec4 colorShininess;
};

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

// Lighting uniform block
layout(std140, binding = 1) uniform GlobalLightBlock
{
    DirectionalLight globalLights[MAX_DIRECTIONAL_LIGHTS];
    int globalLightCount;
};

// Voxel material buffer
layout(std430, binding = 2) buffer VoxelMaterialBlock
{
    VoxelMaterial voxelMaterials[MAX_MATERIALS];
};

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gNorm;
layout(binding = 1) uniform usampler2D gMaterial;
layout(binding = 2) uniform sampler2D gDepth;

in vec2 texCoords;

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
    // Grab data from geometry buffer
    float depth = texture(gDepth, texCoords).r;
    vec3 fragNorm = normalize(texture(gNorm, texCoords).xyz);
    uint materialID = texture(gMaterial, texCoords).r;

    // Calculate fragment position in world space
    vec3 fragPos = getWorldPos(texCoords, depth);

    // Grab material
    VoxelMaterial material = voxelMaterials[materialID];

    // Gamma correct and clamp the material's color and shininess values
    vec3 materialColor = pow(material.colorShininess.rgb, vec3(GAMMA));
    float materialShininess = pow(material.colorShininess.w, GAMMA);

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos.xyz - fragPos);

    // Calculate influence from all active global lights
    vec3 result = vec3(0.0);
    for (int i = 0; i < globalLightCount; i++)
    {
        // Grab light information
        vec3 lightDir = normalize(-globalLights[i].directionAmbient.xyz);
        vec3 lightColor = pow(globalLights[i].color.rgb, vec3(GAMMA));
        float lightAmbience = globalLights[i].directionAmbient.w;

        // Calculate alignment to light
        float alignment = dot(fragNorm, lightDir);

        // Ambient lighting
        vec3 ambient = vec3(lightAmbience) * lightColor * materialColor;

        // Diffuse lighting
        vec3 diffuse = max(alignment, 0.0) * lightColor * materialColor;

        // Specular reflections
        vec3 lightHalfDir = normalize(lightDir + viewDir);
        float specLight = max(pow(max(dot(fragNorm, lightHalfDir), 0.0), materialShininess * 256.0), 0.0);
        vec3 specular = specLight * lightColor * vec3(materialShininess);

        // Final color composition
        result += ambient + diffuse + specular;
    }
    
    // Output final accumulation with gamma correction
    finalColor = vec4(pow(result, vec3(1.0 / GAMMA)), 1.0);
}