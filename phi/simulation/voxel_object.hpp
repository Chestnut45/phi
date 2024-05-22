#pragma once

#include <phi/scene/components/base_component.hpp>
#include <phi/scene/components/renderable/voxel_mesh.hpp>

namespace Phi
{
    // A custom component representing an instance of a voxel object
    class VoxelObject : public BaseComponent
    {
        // Interface
        public:

            // Creates an empty voxel object
            VoxelObject();
            ~VoxelObject();

            // Delete copy constructor/assignment
            VoxelObject(const VoxelObject&) = delete;
            VoxelObject& operator=(const VoxelObject&) = delete;

            // Delete move constructor/assignment
            VoxelObject(VoxelObject&& other) = delete;
            VoxelObject& operator=(VoxelObject&& other) = delete;

            // Loading

            // Loads voxel data from a .vobj file, replacing any existing data
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Management

            // Resets to initial state, unloads all voxel data and destroys internal resources
            void Reset();
        
        // Data / implementation
        private:

            // Internal mesh component (NON-OWNING)
            VoxelMesh* mesh = nullptr;
    };
}