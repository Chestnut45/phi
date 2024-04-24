#version 460

// Definition of per-emitter data
struct EmitterData
{
    mat4 transform;
    ivec4 textureID;
};

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

// Emitter data SSBO (Each emitter gets their own indirect draw command,
// so we can index this buffer with gl_DrawID directly)
layout(std430, binding = 0) buffer EmitterSSBO
{
    EmitterData emitters[];
};

// Unit quad vertices
layout(location = 0) in vec3 vQuad;

// Instanced particle vertex data
layout(location = 1) in vec3 vPos;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec2 vSize;

// Fragment shader outputs
out vec3 fragPos;
out flat vec4 fragColor;
out vec2 uv;
out flat int texID;

void main()
{
    // Grab the camera vectors
    vec3 cameraUp = invView[1].xyz;
    vec3 cameraRight = invView[0].xyz;

    // Calculate final worldspace position
    vec3 vertexPosWorld = (emitters[gl_DrawID].transform * vec4(vPos, 1.0)).xyz + (cameraRight * vQuad.x * vSize.x) + (cameraUp * vQuad.y * vSize.y);

    // Set gl_Position
    gl_Position = viewProj * vec4(vertexPosWorld, 1.0);

    // Fragment shader outputs
    fragPos = vertexPosWorld;
    fragColor = vec4(vColor.rgb * vColor.a, vColor.a);
    uv = vQuad.xy + vec2(0.5);
    texID = emitters[gl_DrawID].textureID.x;
}