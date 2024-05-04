#version 460

layout(binding = 4) uniform sampler2D sunTexture;

in vec2 texCoords;
out vec4 finalColor;

void main()
{
    finalColor = texture(sunTexture, texCoords);
}