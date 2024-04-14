#version 460

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
out vec3 fragPos;
out vec3 fragNormal;
out flat int fragMaterial;

void main()
{
    // Calculate world position
    vec4 worldPos = transforms[gl_DrawID] * vec4(vPos + voxelPos, 1.0);

    // Set position
    gl_Position = viewProj * vec4(worldPos.xyz, 1.0);

    // Set fragment shader outputs
    fragPos = worldPos.xyz;
    fragNormal = normalize((inverse(transpose(transforms[gl_DrawID])) * vec4(vNorm, 1.0)).xyz);
    fragMaterial = voxelMaterial;
}