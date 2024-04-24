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

in vec3 vPos;
in vec3 instancePos;
in vec4 instanceColorRadius;

out flat vec3 lightPosWorld;
out flat vec4 lightColorRadius;

void main()
{
    gl_Position = viewProj * vec4(vPos * instanceColorRadius.w + instancePos, 1.0);
    lightPosWorld = instancePos;
    lightColorRadius = instanceColorRadius;
}