#pragma once

#include <glm/glm.hpp>

namespace Phi
{
    // A basic material used for rendering meshes
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
}