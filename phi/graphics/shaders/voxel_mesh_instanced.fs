#version 460

// Vertex shader inputs
// in vec3 fragPos;
in vec3 fragNormal;
in flat int fragMaterial;

// Geometry buffer outputs
// layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out uint gMaterial;

void main()
{
    // gPosition = fragPos;
    gNormal = fragNormal;
    gMaterial = fragMaterial;
}