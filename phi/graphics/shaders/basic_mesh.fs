#version 460

// Vertex Inputs
in vec3 fragPos;
in vec3 normal;
in flat uint material;

// Geometry buffer outputs
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out uint gMaterial;

void main()
{
    // Output to geometry buffer directly
    gPosition = fragPos;
    gNormal = normal;
    gMaterial = material;
}