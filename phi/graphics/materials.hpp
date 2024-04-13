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

    // A material that can be used with VoxelObjects
    // Determines rendering properties as well as physical / chemical properties
    struct VoxelMaterial
    {
        glm::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
        float shininess = 0.5f;
        
        // Constructors
        VoxelMaterial() {};
        VoxelMaterial(const glm::vec4& color, float shininess)
            : color(color), shininess(shininess)
        {
        }
    };
}