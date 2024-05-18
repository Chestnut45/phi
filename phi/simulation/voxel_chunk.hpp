#pragma once

#include <phi/core/structures/grid_3d.hpp>
#include <phi/scene/components/renderable/voxel_mesh.hpp>

namespace Phi
{
    // Represents a single chunk in a voxel world
    // Holds terrain voxels and a list of active entities (nodes)
    class VoxelChunk
    {
        // Interface
        public:

            // Constants
            static const int CHUNK_DIM = 16;

            VoxelChunk();
            ~VoxelChunk();

            // Delete copy constructor/assignment
            VoxelChunk(const VoxelChunk&) = delete;
            VoxelChunk& operator=(const VoxelChunk&) = delete;

            // Delete move constructor/assignment
            VoxelChunk(VoxelChunk&& other) = delete;
            VoxelChunk& operator=(VoxelChunk&& other) = delete;

            // Simulation

            // Steps the chunk simulation forward by delta seconds
            void Update(float delta);

        // Data / implementation
        private:

            // DEBUG: Grid of voxel material IDs for testing
            Grid3D<int> voxelGrid{CHUNK_DIM, CHUNK_DIM, CHUNK_DIM};

            // Voxel Worlds should have full access to chunk data
            friend class VoxelWorld;
    };
}