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
};

// Vertex data
layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNorm;
layout(location = 2) in mat4 iTransform;
// Locations 3, 4, 5 also consumed by mat4
layout(location = 6) in uint iMaterial;

// Fragment shader outputs
out vec3 normal;
out flat uint material;

void main()
{
    // Calculate projected positions
    vec4 worldSpacePos = iTransform * vec4(vPos, 1.0);
    gl_Position = viewProj * worldSpacePos;

    // Send per fragment outputs
    normal = normalize((inverse(transpose(iTransform)) * vec4(vNorm, 1.0)).xyz);
    material = iMaterial;
}