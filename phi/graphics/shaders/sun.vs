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

// Constant unit quad verts
const vec2 UNIT_QUAD[] =
{
    vec2(-0.5, 0.5),
    vec2(-0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5)
};

// Constant UVs
const vec2 UVS[] =
{
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 1),
    vec2(0, 0),
    vec2(1, 0)
};

uniform vec3 sunPos;
uniform float sunSize;

out vec2 texCoords;

void main()
{
    // Grab the camera vectors
    vec3 cameraUp = invView[1].xyz;
    vec3 cameraRight = invView[0].xyz;

    // Select proper quad vertex
    vec2 vQuad = UNIT_QUAD[gl_VertexID];

    // Calculate final worldspace position
    // TODO: Size should be configurable
    vec4 vertexPosWorld = viewProj * vec4(sunPos + cameraPos.xyz + (cameraRight * vQuad.x * sunSize) + (cameraUp * vQuad.y * sunSize), 1.0);

    // Set final position and texture coordinates
    gl_Position = vertexPosWorld;
    texCoords = UVS[gl_VertexID];
}