#pragma once

namespace Phi
{
    // Represents an axis-aligned voxel map
    // Can be loaded into a scene for simulation
    // Can be loaded / generated from a .vmap file (and associated resources)
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


    };
}