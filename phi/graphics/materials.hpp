#pragma once

#include <glm/glm.hpp>

namespace Phi
{
    // A PBR material model used for rendering
    struct PBRMaterial
    {
        glm::vec3 color{0.5f, 0.5f, 0.5f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        
        // Constructors
        PBRMaterial() {};
        PBRMaterial(const glm::vec3& color, float metallic, float roughness)
            : color(color), metallic(metallic), roughness(roughness)
        {
        }
    };
}