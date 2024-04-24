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

layout(std430, binding = 0) buffer InstanceData
{
    mat4 transforms[];
};

// Cube vertices
in vec3 vPos;
in vec3 vNorm;

// Instanced voxel data
in ivec3 voxelPos;
in int voxelMaterial;

// Fragment shader outputs
out vec3 fragNormal;
out flat int fragMaterial;

void main()
{
    // Grab the mesh transform
    mat4 meshTransform = transforms[gl_DrawID];

    // Calculate world position
    vec4 worldPos = meshTransform * vec4(vPos + voxelPos, 1.0);

    // Set position
    gl_Position = viewProj * vec4(worldPos.xyz, 1.0);

    // Set fragment shader outputs
    fragNormal = normalize((inverse(transpose(meshTransform)) * vec4(vNorm, 1.0)).xyz);
    fragMaterial = voxelMaterial;
}