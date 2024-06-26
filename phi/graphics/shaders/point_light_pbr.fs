#version 460

const float GAMMA = 2.2;
const int MAX_MATERIALS = 1024;
const float PI = 3.141592653589793238;

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

// Geometry buffer texture samplers
layout(binding = 0) uniform sampler2D gNorm;
layout(binding = 1) uniform sampler2D gAlbedo;
// gEmissive not necessary for point lights
layout(binding = 3) uniform sampler2D gMetallicRoughness;
layout(binding = 4) uniform sampler2D gDepth;

in flat vec3 lightPosWorld;
in flat vec4 lightColorRadius;

out vec4 finalColor;

// Gets the world position from texCoords and depth
vec3 getWorldPos(vec2 texCoords, float depth)
{
    // Calculate view space position
    vec4 viewSpacePos = invProj * vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Perspective division
    viewSpacePos /= viewSpacePos.w;

    // Calculate world space position
    vec4 worldSpacePos = invView * viewSpacePos;
    return worldSpacePos.xyz;
}

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(n, h), 0.0);
    float nDotH2 = nDotH * nDotH;
    return a2 / (PI * pow((nDotH2 * (a2 - 1.0) + 1.0), 2));
}

float geometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    return nDotV / (nDotV * (1.0 - k) + k);
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float nDotV = max(dot(n, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float ggx2  = geometrySchlickGGX(nDotV, roughness);
    float ggx1  = geometrySchlickGGX(nDotL, roughness);
    return ggx1 * ggx2;
}

void main()
{
    // Calculate texture coordinates
    vec2 texCoords = (gl_FragCoord.xy - viewport.xy) * 0.5 / viewport.zw;

    // Grab data from geometry buffer
    float depth = texture(gDepth, texCoords).r;
    vec3 fragNorm = normalize(texture(gNorm, texCoords).xyz);
    vec3 fragAlbedo = texture(gAlbedo, texCoords).rgb;
    vec2 fragMetallicRoughness = texture(gMetallicRoughness, texCoords).rg;

    // Material data
    vec3 albedo = pow(fragAlbedo, vec3(GAMMA));
    float metallic = fragMetallicRoughness.x;
    float roughness = fragMetallicRoughness.y;

    // Calculate fragment position in world space
    vec3 fragPos = getWorldPos(texCoords, depth);

    // Calculate view direction
    vec3 viewDir = normalize(cameraPos.xyz - fragPos);

    // Extract light data
    vec3 lightColor = lightColorRadius.rgb;
    float lightRadius = lightColorRadius.a;
    vec3 lightDir = normalize(lightPosWorld - fragPos);
    vec3 lightHalfDir = normalize(lightDir + viewDir);

    // Calculate alignment to light
    float alignment = dot(fragNorm, lightDir);

    // Calculate surface reflection at zero
    vec3 srz = mix(vec3(0.04), albedo, metallic);
    
    // Cook-Torrance BRDF
    vec3 f = fresnelSchlick(max(dot(lightHalfDir, viewDir), 0.0), srz);
    float ndf = distributionGGX(fragNorm, lightHalfDir, roughness);
    float g = geometrySmith(fragNorm, viewDir, lightDir, roughness);
    vec3 specular = ndf * g * f / (4.0 * max(dot(fragNorm, viewDir), 0.0) * max(alignment, 0.0001));

    // Attenuation
    float dist = distance(fragPos, lightPosWorld);
    float attenuation = clamp(1.0 - dist / lightRadius, 0.0, 1.0);
    attenuation *= attenuation;

    // Final color composition
    vec3 kD = (vec3(1.0) - f) * (1.0 - metallic);
    vec3 radiance = lightColor * attenuation;
    vec3 result = (kD * albedo / PI + specular) * radiance * max(alignment, 0.0);

    // DEBUG: Tone mapping
    // NOTE: Should be done at the end of all lighting passes, not at each pass
    // result = result / (result + vec3(1.0));

    // Final color composition
    finalColor = vec4(result, 1.0);
}