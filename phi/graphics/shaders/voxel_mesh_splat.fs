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
    vec2 hres;
};

// Vertex Inputs
in flat vec3 voxelCenter;
in flat uint material;
in flat mat3 voxelRotation;
in flat mat3 invVoxelRotation;

// Geometry buffer outputs
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out uint gMaterial;

// Function declarations
bool rayAABB(vec3 rayOrigin, vec3 rayDirection, vec3 boxCenter, vec3 boxRadius, mat3 rotation, mat3 invRotation, out float dist, out vec3 normal);

// Entrypoint
void main()
{
    // Calculate clip space coordinates
    // TODO: This is likely the slowest part of the fragment shader, can the vs help us at all here?
    vec4 clip = vec4(
        (gl_FragCoord.x / hres.x - 1),
        (gl_FragCoord.y / hres.y - 1),
        (2 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / (gl_DepthRange.far - gl_DepthRange.near),
        1.0
    ) / gl_FragCoord.w;

    // Unproject fragment position to world space
    vec3 fragPosWorld = (invViewProj * clip).xyz;

    // Calculate ray direction
    vec3 rayDirection = normalize(fragPosWorld - cameraPos.xyz);

    // Ray cast into fragments to determine normal / box-intersection information
    vec3 normal;
    float dist;
    if (rayAABB(cameraPos.xyz, rayDirection, voxelCenter, vec3(0.5), voxelRotation, invVoxelRotation, dist, normal))
    {
        // Voxel intersection detected
        
        // Calculate position of surface
        vec3 surfacePos = cameraPos.xyz + rayDirection * dist;

        // Output to geometry buffer directly
        gPosition = surfacePos;
        gNormal = normal;
        gMaterial = material;

        // Update depth buffer
        vec4 depth = viewProj * vec4(surfacePos, 1.0);
        gl_FragDepth = ((depth.z / depth.w) + 1) * 0.5;
    }
    else
    {
        // No intersection, discard the fragment
        discard;
    }
}

// Calculates intersection data for a ray and an AABB, if one would occur
// Modified implementation of:
// Alexander Majercik, Cyril Crassin, Peter Shirley, and Morgan McGuire, A Ray-Box Intersection Algorithm and Efficient Dynamic Voxel Rendering,
// Journal of Computer Graphics Techniques (JCGT), vol. 7, no. 3, 66-81, 2018 Available online http://jcgt.org/published/0007/03/04/
bool rayAABB(vec3 rayOrigin, vec3 rayDirection, vec3 boxCenter, vec3 boxRadius, mat3 rotation, mat3 invRotation, out float dist, out vec3 normal)
{
    // Transform ray data into box local coordinates
    rayOrigin = rotation * (rayOrigin - boxCenter);
    rayDirection = rotation * rayDirection;

    // TODO: If ray starts inside the box, behaviour is incorrect

    // Calculate the negated sign of the ray direction
    vec3 sgn = -sign(rayDirection);

    // Only calculate the distance to each front-facing plane of the AABB
    vec3 invRayDir = vec3(1 / rayDirection.x, 1 / rayDirection.y, 1 / rayDirection.z);
    vec3 distPlane = boxRadius * sgn - rayOrigin;
    distPlane *= invRayDir;

    #define TEST(U, VW)\
         /* Is there a hit on this axis in front of the origin? Use multiplication instead of && for a small speedup */\
         (distPlane.U >= 0.0) && \
         /* Is that hit within the face of the box? */\
         all(lessThan(abs(rayOrigin.VW + rayDirection.VW * distPlane.U), boxRadius.VW))

    bvec3 test = bvec3(TEST(x, yz), TEST(y, zx), TEST(z, xy));

    // CMOV chain that guarantees exactly one element of sgn is preserved and that the value has the right sign
    sgn = test.x ? vec3(sgn.x, 0.0, 0.0) : (test.y ? vec3(0.0, sgn.y, 0.0) : vec3(0.0, 0.0, test.z ? sgn.z : 0.0));
    #undef TEST

    // Calculate distance to the box, masked by whichever axis is non-zero
    dist = (sgn.x != 0.0) ? distPlane.x : ((sgn.y != 0.0) ? distPlane.y : distPlane.z);

    // Return hit normal in world space
    normal = invRotation * sgn;

    // Return true if any of the axes are non-zero
    return (sgn.x != 0) || (sgn.y != 0) || (sgn.z != 0);
}