#pragma once

#include <glm/glm.hpp>

namespace Phi
{
    // A basic material used for rendering BasicMesh instances
    struct BasicMaterial
    {
        glm::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
        float shininess = 0.5f;
        
        // Constructors
        BasicMaterial() {};
        BasicMaterial(const glm::vec4& color, float shininess)
            : color(color), shininess(shininess)
        {
        }
    };

    // A PBR material model used for VoxelObjects
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