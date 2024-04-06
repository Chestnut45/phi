#version 460

layout(binding = 0) uniform sampler2D textures[16];

in vec3 fragPos;
in flat vec4 fragColor;
in vec2 uv;
in flat int texID;

out vec4 finalColor;

void main()
{
    // NOTE: texID is a dynamically uniform expression because of how the indirect draw calls are structured
    finalColor = fragColor * texture(textures[texID], uv);
}