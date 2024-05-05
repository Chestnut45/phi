#pragma once

#include <phi/core/structures/grid_3d.hpp>
#include <phi/scene/scene.hpp>

namespace Phi
{
    // Represents a voxel world and all of its nodes / components
    // Provides an interface to the internal scene instance
    class VoxelWorld
    {
        // Interface
        public:

            // Creates an empty voxel world with default settings
            VoxelWorld();
            ~VoxelWorld();

            // Delete copy constructor/assignment
            VoxelWorld(const VoxelWorld&) = delete;
            VoxelWorld& operator=(const VoxelWorld&) = delete;

            // Delete move constructor/assignment
            VoxelWorld(VoxelWorld&& other) = delete;
            VoxelWorld& operator=(VoxelWorld&& other) = delete;

            // Simulation

            // Updates the voxel world with the given elapsed time in seconds
            void Update(float delta);

            // Full access to the voxel world's internal scene instance
            // TODO: Restrict more? How much access is actually required by the user at this level?
            Scene& GetScene() { return scene; };

            // Rendering

            // Renders the voxel world to the current framebuffer
            void Render();

        // Data / implementation
        private:

            // Internal scene instance
            Scene scene;

            // DEBUG: Grid of voxel material IDs for testing chunks
            Grid3D<int> voxelGrid{64, 64, 64};

    };
}