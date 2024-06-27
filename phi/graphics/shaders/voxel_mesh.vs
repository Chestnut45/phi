#version 460

const int MAX_MATERIALS = 1024;

// Global time
uniform float time;

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
    vec4 emissive;
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
out flat vec4 fragAlbedo;
out flat vec4 fragEmissive;
out flat vec2 fragMetallicRoughness;

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x)
{
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Construct a float with half-open range [0, 1) using low 23 bits
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m)
{
    // IEEE Binary32 Constants
    const uint ieeeMantissa = 0x007FFFFFu;
    const uint ieeeOne = 0x3F800000u;

    // Only keep fractional part
    m &= ieeeMantissa;
    m |= ieeeOne;

    // Adjust to range [0, 1]
    float f = uintBitsToFloat(m);
    return f - 1.0;
}

// Pseudo-random value in half-open range [0, 1)
float random(float x)
{
    return floatConstruct(hash(floatBitsToUint(x)));
}

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
    int voxelMaterial = bitfieldExtract(int(voxel.y), 16, 16);

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

    // Special effects
    vec4 albedo;
    vec4 emissive;
    vec2 metallicRoughness;
    if (voxelMaterial == -1)
    {
        // Fire effect

        // Uniform distribution between red and yellow
        float r = random(float(voxelIndex) + time);

        // Set material properties
        emissive = vec4(mix(vec3(1, 0, 0), vec3(1, 0.5, 0), r), 8.0);
        albedo = vec4(0.0, 0.0, 0.0, 1.0);
        metallicRoughness = vec2(0.0);
    }
    else
    {
        // Load material
        PBRMaterial material = pbrMaterials[voxelMaterial];
        albedo = material.color;
        emissive = material.emissive;
        metallicRoughness = material.metallicRoughness.xy;
    }

    // Fragment outputs
    fragPos = worldPos.xyz;
    fragAlbedo = albedo;
    fragEmissive = emissive;
    fragMetallicRoughness = metallicRoughness;
}