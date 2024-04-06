#pragma once

#include <phi/core/structures/experimental/hash_grid_3d.hpp>
#include <phi/voxel/voxel.hpp>

namespace Phi
{
    // Represents a collection of axis-aligned voxels, treated as a single object
    // 
    class VoxelObject
    {
        // Interface
        public:

            VoxelObject();
            ~VoxelObject();

            // Delete copy constructor/assignment
            VoxelObject(const VoxelObject&) = delete;
            VoxelObject& operator=(const VoxelObject&) = delete;

            // Delete move constructor/assignment
            VoxelObject(VoxelObject&& other) = delete;
            void operator=(VoxelObject&& other) = delete;

        // Data / implementation
        private:

            // Voxel storage
            HashGrid3D<Voxel> voxels;
    };
}