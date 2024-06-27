#version 460

const float GAMMA = 2.2;

layout(binding = 0) uniform sampler2D textures[16];

in vec3 fragPos;
in flat vec4 fragColor;
in vec2 uv;
in flat int texID;

out vec4 finalColor;

void main()
{
    // NOTE: texID is a dynamically uniform expression because of how the indirect draw calls are structured
    finalColor = pow(fragColor * texture(textures[texID], uv), vec4(vec3(GAMMA), 1.0));
}