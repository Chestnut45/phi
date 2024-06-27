#version 460

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

// Vertex data
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNorm;
layout(location = 2) in mat4 iTransform;
// Locations 3, 4, 5 also consumed by mat4
layout(location = 6) in uint iMaterial;

struct PBRMaterial
{
    vec4 color;
    vec4 metallicRoughness;
};

layout(std430, binding = 1) buffer PBRMaterialBlock
{
    PBRMaterial pbrMaterials[MAX_MATERIALS];
};

// Fragment shader outputs
out vec3 fragNormal;
out flat vec4 fragAlbedo;
out flat vec2 fragMetallicRoughness;

void main()
{
    // Calculate projected positions
    vec4 worldSpacePos = iTransform * vec4(vPos, 1.0);
    gl_Position = viewProj * worldSpacePos;

    // Material Data
    PBRMaterial material = pbrMaterials[iMaterial];

    // Send per fragment outputs
    fragNormal = normalize((inverse(transpose(iTransform)) * vec4(vNorm, 1.0)).xyz);
    fragAlbedo = material.color;
    fragMetallicRoughness = material.metallicRoughness.xy;
}