#version 460

out vec4 finalColor;

in flat vec3 fragColor;

void main()
{
    finalColor = vec4(fragColor, 1.0);
}