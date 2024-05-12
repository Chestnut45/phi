#version 460

const int MAX_DIRECTIONAL_LIGHTS = 5; // 4 for user, 1 for active sky
const int MAX_MATERIALS = 1024;
const float GAMMA = 2.2;
const float PI = 3.141592653589793238;

// Light structure
struct DirectionalLight
{
    vec4 color;
    vec4 directionAmbient;
};

// Voxel material definition
struct VoxelMaterial
{
    vec4 color;
    vec4 metallicRoughness;
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
    vec4 nearFar; // x = near, y = far, z = null, w = null
};

// Lighting uniform block
layout(std140, binding = 1) uniform GlobalLightBlock
{
    DirectionalLight globalLights[MAX_DIRECTIONAL_LIGHTS];
    int globalLightCount;
    float baseAmbientLight;
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

// SSAO sampler
layout(binding = 3) uniform sampler2D ssaoTex;

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

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(n, h), 0.0);
    float nDotH2 = nDotH * nDotH;
    return a2 / (PI * pow((nDotH2 * (a2 - 1.0) + 1.0), 2));
}

float geometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    return nDotV / (nDotV * (1.0 - k) + k);
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float nDotV = max(dot(n, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float ggx2  = geometrySchlickGGX(nDotV, roughness);
    float ggx1  = geometrySchlickGGX(nDotL, roughness);
    return ggx1 * ggx2;
}

void main()
{
    // Grab data from geometry buffer
    float depth = texture(gDepth, texCoords).r;
    vec3 fragNorm = normalize(texture(gNorm, texCoords).xyz);
    uint materialID = texture(gMaterial, texCoords).r;
    float occlusion = texture(ssaoTex, texCoords).r;

    // Calculate fragment position in world space
    vec3 fragPos = getWorldPos(texCoords, depth);

    // Grab material data
    VoxelMaterial material = voxelMaterials[materialID];
    vec3 materialColor = pow(material.color.rgb, vec3(GAMMA));
    float materialMetallic = material.metallicRoughness.x;
    float materialRoughness = material.metallicRoughness.y;

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos.xyz - fragPos);

    // Calculate influence from all active global lights
    // Start with the base ambient light of the scene
    vec3 result = materialColor * baseAmbientLight * occlusion;
    for (int i = 0; i < globalLightCount; i++)
    {
        // Grab light information
        vec3 lightDir = normalize(-globalLights[i].directionAmbient.xyz);
        vec3 lightHalfDir = normalize(lightDir + viewDir);
        vec3 lightColor = pow(globalLights[i].color.rgb, vec3(GAMMA));
        float lightAmbience = globalLights[i].directionAmbient.w;

        // Calculate alignment to light
        float alignment = dot(fragNorm, lightDir);
        
        // Calculate surface reflection at zero
        vec3 srz = mix(vec3(0.04), materialColor, materialMetallic);
        
        // Cook-Torrance BRDF
        vec3 f = fresnelSchlick(max(dot(lightHalfDir, viewDir), 0.0), srz);
        float ndf = distributionGGX(fragNorm, lightHalfDir, materialRoughness);
        float g = geometrySmith(fragNorm, viewDir, lightDir, materialRoughness);
        vec3 specular = ndf * g * f / (4.0 * max(dot(fragNorm, viewDir), 0.0) * max(alignment, 0.0001));

        // Ambient lighting
        vec3 ambient = vec3(lightAmbience) * materialColor * occlusion;

        // Final color composition
        vec3 kD = (vec3(1.0) - f) * (1.0 - materialMetallic);
        result += ambient + (kD * materialColor / PI + specular) * lightColor * max(alignment, 0.0);
    }

    // DEBUG: Tone mapping
    // NOTE: Should be done at the end of all lighting passes, not at each pass
    result = result / (result + vec3(1.0));
    
    // Output final accumulation with gamma correction
    finalColor = vec4(pow(result, vec3(1.0 / GAMMA)), 1.0);
}