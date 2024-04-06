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

// Basic material definition
struct BasicMaterial
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
    vec4 cameraPos;
    vec2 hres;
};

// Material buffer
layout(std140, binding = 1) uniform MaterialBlock
{
    BasicMaterial materials[MAX_MATERIALS];
};

// Lighting uniform block
layout(std140, binding = 2) uniform GlobalLightBlock
{
    DirectionalLight globalLights[MAX_DIRECTIONAL_LIGHTS];
};

uniform int globalLightCount;

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gPos;
layout(binding = 1) uniform sampler2D gNorm;
layout(binding = 2) uniform usampler2D gMaterial;

in vec2 texCoords;

out vec4 finalColor;

void main()
{
    // Grab data from geometry buffer
    vec4 fragPos = texture(gPos, texCoords);
    vec3 fragNorm = normalize(texture(gNorm, texCoords).xyz);
    uint materialID = texture(gMaterial, texCoords).r;

    // Grab material
    BasicMaterial material = materials[materialID];

    // Gamma correct and clamp the material's color and shininess values
    vec3 materialColor = pow(material.colorShininess.rgb, vec3(GAMMA));
    float materialShininess = pow(material.colorShininess.w, GAMMA);

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos.xyz - fragPos.xyz);

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