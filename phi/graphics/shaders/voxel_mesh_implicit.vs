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

layout(std430, binding = 3) buffer VoxelData
{
    ivec4 voxelData[];
};

layout(std430, binding = 4) buffer InstanceData
{
    mat4 transforms[];
};

void main()
{
    // Calculate instance
    uint voxelIndex = gl_VertexID;
    uint instance = voxelIndex >> 3;

    // Grab voxel data
    ivec3 voxelPos = voxelData[instance].xyz;
    int voxelMaterial = voxelData[instance].w;

    // Calculate local camera position
    vec3 localCamPos = cameraPos.xyz - voxelPos;

    // Calculate mirror mask
    uint mask = (uint(localCamPos.x > 0) | uint(localCamPos.y > 0) << 1 | uint(localCamPos.z > 0) << 2);
    voxelIndex ^= mask;

    // Generate cube
    uvec3 xyz = uvec3(voxelIndex & 0x1, (voxelIndex & 0x4) >> 2, (voxelIndex & 0x2) >> 1);

    // Set position
    gl_Position = viewProj * vec4(vec3(xyz) * 2.0 - 1.0 + voxelPos, 1.0);
}