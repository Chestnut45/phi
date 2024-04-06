#version 460

in vec3 fragPos;
in flat vec4 fragColor;

out vec4 finalColor;

void main()
{
    finalColor = fragColor;
}