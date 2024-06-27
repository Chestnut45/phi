#pragma once

#include <phi/graphics/color.hpp>
#include <glm/glm.hpp>

namespace Phi
{
    // A PBR material model used for rendering
    struct PBRMaterial
    {
        // Material diffuse color, alpha is interpreted as transparency
        Color color{0.5f, 0.5f, 0.5f, 1.0f};

        // Emissive color, alpha is interpreted as emissive strength (non pre-multiplied)
        Color emissive{0.0f, 0.0f, 0.0f, 0.0f};

        // Other visual properties
        float metallic = 0.0f;
        float roughness = 0.5f;
        
        // Constructors
        PBRMaterial() {};
        PBRMaterial(const Color& color, const Color& emissive = Color(0.0f, 0.0f, 0.0f, 0.0f), float metallic = 0.0f, float roughness = 0.0f)
            : color(color), emissive(emissive), metallic(metallic), roughness(roughness)
        {
        }
    };
}