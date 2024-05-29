#pragma once

#include <cstdint>
#include <string>

namespace Phi
{
    // A material used for simulation with voxel objects
    struct VoxelMaterial
    {
        // Flags used to determine what types of simulation apply to the material
        struct SimulationFlags
        {
            typedef uint32_t type;
            enum
            {
                NONE = 0,
                LIQUID = 1,
                POWDER = 1 << 1,
                FLAMMABLE = 1 << 2,
                // ...
            };

            // Don't actually construct this! Use SimulationFlags::type instead
            // This is so we can use bitwise operators without casting all the time
            SimulationFlags() = delete;
        };

        VoxelMaterial(const std::string& name = "new_material", SimulationFlags::type simulationFlags = SimulationFlags::NONE, int pbrID = 0);
        ~VoxelMaterial();

        // Identifiers
        std::string name;

        // Simulation data
        SimulationFlags::type simulationFlags;

        // PBR material ID for rendering
        int pbrID;
    };
}