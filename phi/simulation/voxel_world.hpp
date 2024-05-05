#pragma once

#include <phi/core/structures/grid_3d.hpp>

namespace Phi
{
    // Represents a simulatable voxel world
    // Provides an interface to the internal scene instance
    class VoxelWorld
    {
        // Interface
        public:

            VoxelWorld();
            ~VoxelWorld();

            // Delete copy constructor/assignment
            VoxelWorld(const VoxelWorld&) = delete;
            VoxelWorld& operator=(const VoxelWorld&) = delete;

            // Delete move constructor/assignment
            VoxelWorld(VoxelWorld&& other) = delete;
            VoxelWorld& operator=(VoxelWorld&& other) = delete;

        // Data / implementation
        private:

            // DEBUG: Grid of voxel material IDs
            Grid3D<int> voxelGrid{64, 64, 64};

    };
}