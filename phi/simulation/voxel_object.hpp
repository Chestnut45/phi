#pragma once

#include <phi/core/math/shapes.hpp>
#include <phi/core/structures/grid_3d.hpp>
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

            // Data management

            // Loads voxel data from a .vobj file, replacing any existing data
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Resets to initial state, unloads all voxel data and destroys internal resources
            void Reset();

            // Mesh management

            // Returns a pointer to the internal mesh component,
            // or nullptr if none exists
            VoxelMesh* GetMesh() const { return mesh; };

            // Updates the mesh to match the currently visible voxels of the object
            // If no mesh exists, one is added to the node first
            void UpdateMesh();

            // Destroys the internal mesh if one exists
            void DestroyMesh();
        
        // Data / implementation
        private:

            // Offset to apply to coordinates
            glm::ivec3 offset{-16};

            // Debug grid of voxel materials
            Grid3D<int> voxels{32, 32, 32};

            // AABB instance
            AABB aabb{glm::vec3(-16.0f), glm::vec3(16.0f)};

            // Internal mesh component (NON-OWNING)
            VoxelMesh* mesh = nullptr;
    };
}