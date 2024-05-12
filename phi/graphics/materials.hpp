#pragma once

#include <glm/glm.hpp>

namespace Phi
{
    // A PBR material model used for rendering BasicMesh instances
    struct BasicMaterial
    {
        glm::vec3 color{0.5f, 0.5f, 0.5f};
        float metallic = 0.5f;
        float roughness = 0.5f;
        
        // Constructors
        BasicMaterial() {};
        BasicMaterial(const glm::vec3& color, float metallic, float roughness)
            : color(color), metallic(metallic), roughness(roughness)
        {
        }
    };

    // A PBR material model used for voxel rendering
    struct VoxelMaterial
    {
        glm::vec3 color{0.5f, 0.5f, 0.5f};
        float metallic = 0.5f;
        float roughness = 0.5f;
        
        // Constructors
        VoxelMaterial() {};
        VoxelMaterial(const glm::vec3& color, float metallic, float roughness)
            : color(color), metallic(metallic), roughness(roughness)
        {
        }
    };
}