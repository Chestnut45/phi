#version 460

// Fullscreen triangle vertices
const vec2 verts[3] =
{
    vec2(-1, -1),
    vec2(3, -1),
    vec2(-1, 3)
};

// Output texture coordinates
out vec2 texCoords;

void main()
{
    gl_Position = vec4(verts[gl_VertexID], 1.0, 1.0);
    texCoords = gl_Position.xy * 0.5 + 0.5;
}