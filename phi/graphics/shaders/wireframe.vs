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

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vColor;

out flat vec3 fragColor;

void main()
{
    gl_Position = viewProj * vec4(vPos, 1.0);
    fragColor = vColor;
}