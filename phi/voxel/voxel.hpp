#pragma once

#include <cstdint>

namespace Phi
{
    // State flags
    struct VoxelFlags
    {
        typedef uint32_t type;
        enum : type
        {
            Null = 0,

            Asleep      = 1 << 0,
            OnFire      = 1 << 1,
        };
    };

    // Represents a single volumetric "pixel" of material
    // Can be simulated as an individual particle, or as part of an axis-aligned voxel object
    struct Voxel
    {
        // Flags
        VoxelFlags::type flags = VoxelFlags::Null;

        // Data
        int materialID = 0;

        // State
        int durability = 0;
        int charge = 0;

        // Constructors
        Voxel() = default;
        Voxel(int materialID)
            : materialID(materialID)
        {
        }

        ~Voxel() = default;

        // Default copy constructor/assignment
        Voxel(const Voxel&) = default;
        Voxel& operator=(const Voxel&) = default;

        // Default move constructor/assignment
        Voxel(Voxel&& other) = default;
        Voxel& operator=(Voxel&& other) = default;
    };
}
