#version 460

// Geometry buffer outputs
layout(location = 0) out vec3 gNormal;
layout(location = 1) out uint gMaterial;

// Vertex shader inputs
in vec3 fragPos;
in flat uint fragMaterial;

void main()
{
    // Calculate normal from gradient of interpolated position
    gNormal = normalize(cross(dFdx(fragPos), dFdy(fragPos)));

    // Output material directly
    gMaterial = fragMaterial;
}