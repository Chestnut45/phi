#pragma once

#include <phi/core/structures/experimental/array_grid_3d.hpp>

namespace Phi
{
    // Represents an axis-aligned voxel map
    // Can be loaded / generated from a .vmap file (and associated resources)
    // Can be simulated when set as active in a scene...
    class VoxelMap
    {
        // Interface
        public:

            VoxelMap();
            ~VoxelMap();

            // Delete copy constructor/assignment
            VoxelMap(const VoxelMap&) = delete;
            VoxelMap& operator=(const VoxelMap&) = delete;

            // Delete move constructor/assignment
            VoxelMap(VoxelMap&& other) = delete;
            VoxelMap& operator=(VoxelMap&& other) = delete;

        // Data / implementation
        private:

            // DEBUG: Testing 3D grid
            // For now holds material IDs only (no voxel data)
            ArrayGrid3D<int> grid{64, 64, 64};

    };
}