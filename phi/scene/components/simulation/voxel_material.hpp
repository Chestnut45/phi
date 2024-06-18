#pragma once

#include <cstdint>
#include <string>

namespace Phi
{
    // A material used for simulation with voxel objects
    struct VoxelMaterial
    {
        // Simulation flags for various material behaviours / interactions
        struct Flags
        {
            Flags() = delete;
            typedef uint32_t type;
            enum : type
            {
                None = 0,
                Solid = 1,
                Liquid = 1 << 1,
                Gas = 1 << 2,
                Powder = 1 << 3,
                Fire = 1 << 4,
            };
        };

        VoxelMaterial(const std::string& name = "new_material", Flags::type flags = Flags::None, int pbrID = 0);
        ~VoxelMaterial();

        // Identifiers
        std::string name;

        // Simulation data
        Flags::type flags;
        float flammability; // 0 = nonflammable, 1 = catches instantly on contact with fire

        // PBR material ID for rendering
        int pbrID;
    };
}