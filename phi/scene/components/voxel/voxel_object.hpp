#pragma once

#include <phi/scene/components/base_component.hpp>
#include <phi/scene/components/voxel/voxel_mesh_implicit.hpp>

// Forward declaration
class VoxelEditor;

namespace Phi
{
    // A component representing an instance of a voxel object
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

            // Loads voxel data from a .pvox file, replacing any existing data
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Rendering

            // Draws the object (using node's transform if it exists)
            // Drawn objects won't be displayed to the screen until
            // the next call to VoxelObject::FlushRenderQueue()
            void Render();

            // Flushes internal render queue and displays all objects
            static void FlushRenderQueue();

            // Management

            // Resets to initial state, unloads all voxel data and destroys internal resources
            void Reset();
        
        // Data / implementation
        private:

            // Internal mesh
            VoxelMeshImplicit* mesh = nullptr;

            // Friends

            // Needed for Voxel Editor to work
            friend class ::VoxelEditor;
    };
}