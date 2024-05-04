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

layout(binding = 4) uniform sampler2D lightTexture;

// Constants and uniforms
const int SAMPLES = 32;
uniform vec2 lightPos;
uniform vec4 exposureDecayDensityWeight;

in vec2 texCoords;
out vec3 finalColor;

// Analytic bayer over 2 domains
float bayer2  (vec2 a){a=floor(a);return fract(dot(a,vec2(.5, a.y*.75)));}
float bayer4  (vec2 a){return bayer2 (  .5*a)*.25    +bayer2(a);}
float bayer8  (vec2 a){return bayer4 (  .5*a)*.25    +bayer2(a);}
float bayer16 (vec2 a){return bayer4 ( .25*a)*.0625  +bayer4(a);}
float bayer32 (vec2 a){return bayer8 ( .25*a)*.0625  +bayer4(a);}
float bayer64 (vec2 a){return bayer8 (.125*a)*.015625+bayer8(a);}
float bayer128(vec2 a){return bayer16(.125*a)*.015625+bayer8(a);}
#define dither2(p)   (bayer2(  p)-.375      )
#define dither4(p)   (bayer4(  p)-.46875    )
#define dither8(p)   (bayer8(  p)-.4921875  )
#define dither16(p)  (bayer16( p)-.498046875)
#define dither32(p)  (bayer32( p)-.499511719)
#define dither64(p)  (bayer64( p)-.49987793 )
#define dither128(p) (bayer128(p)-.499969482)
//https://www.shadertoy.com/view/4ssfWM

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
    float dither = dither2(gl_FragCoord.xy / vec2(2));
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