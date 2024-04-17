#version 460

// Vertex shader inputs
in vec3 fragNormal;
in flat int fragMaterial;

// Geometry buffer outputs
layout(location = 0) out vec3 gNormal;
layout(location = 1) out uint gMaterial;

void main()
{
    gNormal = fragNormal;
    gMaterial = fragMaterial;
}