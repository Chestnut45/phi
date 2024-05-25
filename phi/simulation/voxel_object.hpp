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

            // Raycast queries

            // Structure for returning ray cast query data
            struct RaycastInfo
            {
                // Each voxel visited in order
                std::vector<glm::ivec3> visitedVoxels;
                
                // The index of the first solid hit voxel, or -1 if no solid voxels were hit by the ray
                int firstHit = -1;
            };

            // Casts a ray into the voxel object, returns voxel intersection information
            RaycastInfo Raycast(const Ray& ray);

            // Data management

            // Loads voxel data from a .vobj file, replacing any existing data
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Resets to initial state, unloads all voxel data and destroys internal resources
            void Reset();

            // Mesh management

            // Updates the mesh to match the currently visible voxels of the object
            // If no mesh exists, one is added to the node first
            void UpdateMesh();

            // Destroys the internal mesh if one exists
            void DestroyMesh();

            // Accessors

            // Returns a pointer to the internal mesh component,
            // or nullptr if none exists
            VoxelMesh* GetMesh() const { return mesh; };

            // Returns a const reference to the object local space AABB
            const AABB& GetAABB() const { return aabb; }

            // Returns a reference to the grid of voxel data
            Grid3D<int>& Grid() { return voxels; }
            
            // Gets the voxel at the object local coordinates provided
            int GetVoxel(int x, int y, int z) { return voxels(x - offset.x, y - offset.y, z - offset.z); }
        
        // Data / implementation
        private:

            // Offset to apply to obtain object-local space coordinates
            glm::ivec3 offset{-16};

            // Debug grid of voxel materials
            Grid3D<int> voxels{32, 32, 32};

            // AABB instance
            AABB aabb{glm::vec3(-16.0f), glm::vec3(16.0f)};

            // Internal mesh component (NON-OWNING)
            VoxelMesh* mesh = nullptr;
    };
}