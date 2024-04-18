#version 460

// Geometry buffer outputs
layout(location = 0) out vec3 gNormal;
layout(location = 1) out uint gMaterial;

void main()
{
    gNormal = vec3(0, 1, 0);
    gMaterial = 0;
}