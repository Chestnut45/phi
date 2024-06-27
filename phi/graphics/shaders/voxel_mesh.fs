#version 460

// Geometry buffer outputs
layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gAlbedo;
layout(location = 2) out vec2 gMetallicRoughness;

// Vertex shader inputs
in vec3 fragPos;
in flat vec4 fragAlbedo;
in flat vec2 fragMetallicRoughness;

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
    // Apply transparency
    float alpha = fragAlbedo.a;
    if (alpha == 0.0 || dither8(gl_FragCoord.xy) > (alpha - 0.5) * 0.984375) 
    {
        discard;
    }

    // Calculate normal from gradient of interpolated position
    gNormal = normalize(cross(dFdx(fragPos), dFdy(fragPos)));

    // Output material data
    gAlbedo = fragAlbedo.rgb;
    gMetallicRoughness = fragMetallicRoughness;
}