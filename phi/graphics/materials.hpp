#pragma once

#include <phi/graphics/color.hpp>
#include <glm/glm.hpp>

namespace Phi
{
    // A PBR material model used for rendering
    struct PBRMaterial
    {
        Color color{0.5f, 0.5f, 0.5f, 1.0f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        
        // Constructors
        PBRMaterial() {};
        PBRMaterial(const Color& color, float metallic, float roughness)
            : color(color), metallic(metallic), roughness(roughness)
        {
        }
    };
}