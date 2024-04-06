#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Phi
{
    // Represents a physical material in the engine; used for various simulations and rendering.
    struct VoxelMaterial
    {
        // Tags for various behaviours supported by the engine
        enum class Tags : int
        {

        };

        // Builtin engine behaviour
        std::vector<Tags> tags;

        // Custom behaviour
        

        // Physical properties
        float density;
        int durability;
        int hardness;
        int viscosity;

        // Chemical properties
        int flammability; // 0 = can't catch fire, 100 = catches instantly
        int conductivity; // 0 = perfect insulator, 100 = perfect conductor
        int magneticCharge; // sign(x) = polarity, abs(x) = strength;
        int pH; // 0 = strongest acid, 7 = neutral, 14 = strongest base

        // Visual / rendering properties
        float specularStrength;
        int shininess;
        glm::vec4 color;
    };
}