#pragma once

#include <phi/scene/components/base_component.hpp>
#include <phi/scene/components/renderable/voxel_mesh_splat_method.hpp>

namespace Phi
{
    // A component representing an instance of a voxel object
    class VoxelObject : public BaseComponent
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
            VoxelObject& operator=(VoxelObject&& other) = delete;

            // Loading

            // Loads a voxel object from a .pvox file
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Rendering

            // Draws the object with the given transformation matrix,
            // or the identity matrix if none is supplied
            // Drawn objects won't be displayed to the screen until
            // the next call to VoxelObject::FlushRenderQueue()
            void Render(const glm::mat4& transform = glm::mat4(1.0f), const glm::mat3& rotation = glm::mat3(1.0f));

            // Flushes internal render queue and displays all objects
            static void FlushRenderQueue();
        
        // Data / implementation
        private:

            // DEBUG: Testing different implementations
            VoxelMeshSplatMethod* splatMesh = nullptr;
    };
}