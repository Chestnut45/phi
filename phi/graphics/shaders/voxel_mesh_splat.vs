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

// Vertex data
layout(location = 0) in ivec3 vPos;
layout(location = 1) in uint vMaterial;
layout(location = 2) in mat4 iTransform;
// Locations 3, 4, 5 also consumed by mat4
layout(location = 6) in mat3 iRotation;
// Locations 7, 8 also consumed by mat3
layout(location = 9) in mat3 iInvRotation;

// Fragment shader outputs
out flat vec3 voxelCenter;
out flat uint material;
out flat mat3 voxelRotation;
out flat mat3 invVoxelRotation;

// Function declarations
void quadricProj(vec3 osPosition, float voxelSize, mat4 objectToScreenMatrix, vec2 halfScreenSize, inout vec4 position, inout float pointSize);

// Entrypoint
void main()
{
    // Calculate projected positions
    vec4 worldSpacePos = iTransform * vec4(vPos, 1.0);
    gl_Position = viewProj * worldSpacePos;

    // Fast Quadratic Proj to determine the AABB covering the projected voxel
    quadricProj(worldSpacePos.xyz, 0.5, viewProj, viewport.zw, gl_Position, gl_PointSize);

    // Send per fragment outputs
    voxelCenter = worldSpacePos.xyz;
    material = vMaterial;
    voxelRotation = transpose(iRotation);
    invVoxelRotation = transpose(iInvRotation);
}

// Fast Quadric Proj: "GPU-Based Ray-Casting of Quadratic Surfaces" http://dl.acm.org/citation.cfm?id=2386396
void quadricProj(vec3 osPosition, float voxelSize, mat4 objectToScreenMatrix, vec2 halfScreenSize, inout vec4 position, inout float pointSize)
{
    const vec4 quadricMat = vec4(1.0, 1.0, 1.0, -1.0);
    float sphereRadius = voxelSize * 1.732051; // sqrt(3), the diagonal of a unit cube (voxel), diameter of its bounding sphere
    vec4 sphereCenter = vec4(osPosition.xyz, 1.0);
    mat4 modelViewProj = transpose(objectToScreenMatrix);

    mat3x4 matT = mat3x4( mat3(modelViewProj[0].xyz, modelViewProj[1].xyz, modelViewProj[3].xyz) * sphereRadius);
    matT[0].w = dot(sphereCenter, modelViewProj[0]);
    matT[1].w = dot(sphereCenter, modelViewProj[1]);
    matT[2].w = dot(sphereCenter, modelViewProj[3]);

    mat3x4 matD = mat3x4(matT[0] * quadricMat, matT[1] * quadricMat, matT[2] * quadricMat);
    vec4 eqCoefs = vec4(dot(matD[0], matT[2]), dot(matD[1], matT[2]), dot(matD[0], matT[0]), dot(matD[1], matT[1])) / dot(matD[2], matT[2]);

    // Calculate out position and pointsize from the AABB covering the voxel
    vec4 outPosition = vec4(eqCoefs.x, eqCoefs.y, 0.0, 1.0);
    vec2 AABB = sqrt(eqCoefs.xy * eqCoefs.xy - eqCoefs.zw);
    AABB *= halfScreenSize * 2.0;
    position.xy = outPosition.xy * position.w;
    pointSize = max(AABB.x, AABB.y);
}