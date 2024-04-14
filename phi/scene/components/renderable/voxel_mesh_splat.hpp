#pragma once

#include <vector>
#include <string>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <phi/graphics/materials.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/shader.hpp>

namespace Phi
{
    // A renderable voxel mesh using splatting and ray-tracing
    class VoxelMeshSplat
    {
        // Interface
        public:

            // Constants
            static inline const size_t MAX_VOXELS = 1'048'576;

            // Convenience vertex declaration
            typedef Phi::VertexVoxel Vertex;

            // Creates a voxel mesh with the given voxel data
            VoxelMeshSplat(const std::vector<Vertex>& voxels);
            ~VoxelMeshSplat();

            // Delete copy constructor/assignment
            VoxelMeshSplat(const VoxelMeshSplat&) = delete;
            VoxelMeshSplat& operator=(const VoxelMeshSplat&) = delete;

            // Delete move constructor/assignment
            VoxelMeshSplat(VoxelMeshSplat&& other) = delete;
            VoxelMeshSplat& operator=(VoxelMeshSplat&& other) = delete;

            // Rendering

            // Draws the mesh with the given transformation matrix,
            // or the identity matrix if none is supplied
            // Drawn meshes won't be displayed to the screen until
            // the next call to VoxelMeshSplat::FlushRenderQueue()
            void Render(const glm::mat4& transform = glm::mat4(1.0f), const glm::mat3& rotation = glm::mat3(1.0f));

            // Flushes internal render queue and displays all meshes
            static void FlushRenderQueue();

            // TODO: Procedural generation
        
        // Data / implementation
        private:

            // Vertex data
            std::vector<Vertex> vertices;
            
            // Constants
            static inline const size_t MAX_DRAW_CALLS = 1'024;

            // Static mesh resources
            static inline Shader* shader = nullptr;
            static inline VertexAttributes* vao = nullptr;
            static inline GPUBuffer* vertexBuffer = nullptr;
            static inline GPUBuffer* meshDataBuffer = nullptr;
            static inline GPUBuffer* indirectBuffer = nullptr;

            // Reference counting for static resources
            static inline size_t refCount = 0;

            // Render counters
            static inline size_t meshDrawCount = 0;
            static inline size_t vertexDrawCount = 0;
    };
}