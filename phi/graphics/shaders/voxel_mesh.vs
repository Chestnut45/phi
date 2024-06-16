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

struct MeshData
{
    mat4 transform;
    mat4 invTransform;
};

struct PBRMaterial
{
    vec4 color;
    vec4 metallicRoughness;
};

layout(std430, binding = 1) buffer PBRMaterialBlock
{
    PBRMaterial pbrMaterials[MAX_MATERIALS];
};

layout(std430, binding = 3) restrict buffer VoxelData
{
    uvec2 voxelData[];
};

layout(std430, binding = 4) restrict buffer InstanceData
{
    MeshData meshData[];
};

// Fragment outputs
out vec3 fragPos;
out flat uint fragMaterial;
out flat float fragAlpha;

// Vertex shader entrypoint
void main()
{
    // Calculate voxel data index
    // NOTE: gl_BaseInstance holds the index of the first voxel for the current mesh
    uint vertexID = gl_VertexID;
    uint voxelIndex = gl_BaseInstance + (vertexID >> 3);

    // Grab voxel data
    uvec2 voxel = voxelData[voxelIndex];
    ivec3 voxelPos = ivec3(bitfieldExtract(int(voxel.x), 0, 16), bitfieldExtract(int(voxel.x), 16, 16), bitfieldExtract(int(voxel.y), 0, 16));
    uint voxelMaterial = bitfieldExtract(voxel.y, 16, 16);

    // Mirroring hack (render only 3 faces per voxel)
    
    // Calculate mirror mask
    // TODO: Profile: Precalculate localCamPos CPU side? Are we ALU limited here?
    vec3 localCamPos = (meshData[gl_DrawID].invTransform * cameraPos).xyz - voxelPos;
    uint mask = (uint(localCamPos.x > 0) | uint(localCamPos.y > 0) << 1 | uint(localCamPos.z > 0) << 2);
    vertexID ^= mask;

    // Generate cube position on (0, 1)
    uvec3 xyz = uvec3(vertexID & 0x1, (vertexID & 0x2) >> 1, (vertexID & 0x4) >> 2);

    // Apply mesh transformation to calculate world space position
    vec4 worldPos = meshData[gl_DrawID].transform * vec4(vec3(xyz) + voxelPos, 1.0);

    // Set position
    gl_Position = viewProj * worldPos;

    // Fragment outputs
    fragPos = worldPos.xyz;
    fragMaterial = voxelMaterial;
    fragAlpha = pbrMaterials[voxelMaterial].color.a;
}