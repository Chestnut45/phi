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

struct MeshData
{
    mat4 transform;
    mat4 invTransform;
};

layout(std430, binding = 3) buffer VoxelData
{
    ivec4 voxelData[];
};

layout(std430, binding = 4) buffer InstanceData
{
    MeshData meshData[];
};

// Fragment outputs
out vec3 fragPos;
out flat int fragMaterial;

// Vertex shader entrypoint
void main()
{
    // Calculate voxel data index
    // NOTE: gl_BaseInstance holds the index of the first voxel for the current mesh
    uint vertexID = gl_VertexID;
    uint voxelIndex = gl_BaseInstance + (vertexID >> 3);

    // Grab voxel data
    vec3 voxelPos = voxelData[voxelIndex].xyz;
    int voxelMaterial = voxelData[voxelIndex].w;

    // TODO: Mirroring hack (render only 3 faces per voxel)

    // Calculate local camera position
    // vec3 localCamPos = (meshData[gl_DrawID].invTransform * cameraPos).xyz - voxelPos;
    
    // Calculate mirror mask
    // uint mask = (uint(localCamPos.x > 0) | uint(localCamPos.y > 0) << 1 | uint(localCamPos.z > 0) << 2);
    // vertexID ^= mask;

    // Generate cube position (0, 1)
    uvec3 xyz = uvec3(vertexID & 0x1, (vertexID & 0x2) >> 1, (vertexID & 0x4) >> 2);

    // Convert to (-0.5, 0.5)
    vec3 pos = vec3(xyz) - 0.5;

    // Apply mesh transformation to calculate world space position
    vec4 worldPos = meshData[gl_DrawID].transform * vec4(pos + voxelPos, 1.0);

    // Set position
    gl_Position = viewProj * worldPos;

    // Fragment outputs
    fragPos = worldPos.xyz;
    fragMaterial = voxelMaterial;
}