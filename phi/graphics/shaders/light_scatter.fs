#version 460

layout(binding = 4) uniform sampler2D sunlightTexture;

// Constants and uniforms
const int SAMPLES = 32;
uniform vec2 lightPos;
uniform vec4 exposureDecayDensityWeight;

in vec2 texCoords;
out vec4 finalColor;

void main()
{
    // Unpack uniforms
    const float exposure = exposureDecayDensityWeight.x;
    const float decay = exposureDecayDensityWeight.y;
    const float density = exposureDecayDensityWeight.z;
    const float weight = exposureDecayDensityWeight.w;

    // Calculate delta and initialize values
    vec2 deltaTex = (texCoords - lightPos) * (1.0 / float(SAMPLES) * density);
    vec2 coords = texCoords;
    float illuminationDecay = 1.0;
    vec3 result = vec3(0.0);

    // Sample along ray
    for (int i = 0; i < SAMPLES; ++i)
    {
        coords = texCoords - (deltaTex * i);
        vec3 s = texture(sunlightTexture, coords).rgb;
        s *= illuminationDecay * weight;
        result += s;
        illuminationDecay *= decay;
    }

    finalColor = vec4(result * exposure, 1.0);
}