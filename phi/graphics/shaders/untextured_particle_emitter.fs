#version 460

const float GAMMA = 2.2;

in vec3 fragPos;
in flat vec4 fragColor;

out vec4 finalColor;

void main()
{
    finalColor = pow(fragColor, vec4(vec3(GAMMA), 1.0));
}