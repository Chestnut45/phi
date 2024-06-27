#version 460

// Camera uniform block
layout(std140, binding = 0) uniform CameraBlock
{
    mat4 viewProj;
    mat4 invViewProj;
    mat4 view;
    mat4 invView;
    mat4 proj;
    mat4 invProj;
    vec4 cameraPos;
    vec4 viewport; // (x, y, width / 2, height / 2)
    vec4 nearFar; // x = near, y = far, z = null, w = null
};

layout(binding = 5) uniform sampler2D lightTexture;

// Constants and uniforms
const int SAMPLES = 32;
uniform vec2 lightPos;
uniform vec4 exposureDecayDensityWeight;

in vec2 texCoords;
out vec3 finalColor;

// Helper bayer matrix dithering functions
// https://www.shadertoy.com/view/4ssfWM
float bayer2 (vec2 a) { a = floor(a); return fract(dot(a, vec2(.5, a.y * 0.75))); }
float bayer4 (vec2 a) { return bayer2(0.5 * a) * 0.25 + bayer2(a); }
float bayer8 (vec2 a) { return bayer4(0.5 * a) * 0.25 + bayer2(a); }
float bayer16 (vec2 a) { return bayer4(0.25 * a) * 0.0625 + bayer4(a); }
float bayer32 (vec2 a) { return bayer8(0.25 * a) * 0.0625 + bayer4(a); }
float bayer64 (vec2 a) { return bayer8(0.125 * a) * 0.015625 + bayer8(a); }
float bayer128 (vec2 a) { return bayer16(0.125 * a) * 0.015625 + bayer8(a); }
#define dither2(p) (bayer2(p) - 0.375)
#define dither4(p) (bayer4(p) - 0.46875)
#define dither8(p) (bayer8(p) - 0.4921875)
#define dither16(p) (bayer16(p) - 0.498046875)
#define dither32(p) (bayer32(p) - 0.499511719)
#define dither64(p) (bayer64(p) - 0.49987793)
#define dither128(p) (bayer128(p) - 0.499969482)

void main()
{
    // Unpack uniforms
    const float exposure = exposureDecayDensityWeight.x;
    const float decay = exposureDecayDensityWeight.y;
    const float density = exposureDecayDensityWeight.z;
    const float weight = exposureDecayDensityWeight.w;

    // Calculate delta and initialize values
    vec2 coords = texCoords;
    vec2 deltaTex = (texCoords - lightPos) * (1 / float(SAMPLES) * density);
    float dither = dither2(gl_FragCoord.xy);
    float illuminationDecay = 1.0;
    vec3 result = vec3(0.0);

    // Sample along ray towards light position in screen space
    for (int i = 0; i < SAMPLES; ++i)
    {
        coords -= deltaTex;
        vec3 s = texture(lightTexture, coords + (deltaTex * dither)).rgb;
        s *= illuminationDecay * weight;
        result += s;
        illuminationDecay *= decay;
    }

    finalColor = result * exposure;
}