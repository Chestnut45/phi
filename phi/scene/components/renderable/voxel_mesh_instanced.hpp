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
    class VoxelMeshInstanced
    {
        // Interface
        public:

            // Constants
            static inline const size_t MAX_VOXELS = 1'048'576;

            // Creates a voxel mesh with the given voxel data
            VoxelMeshInstanced(const std::vector<VertexVoxel>& voxels);
            ~VoxelMeshInstanced();

            // Delete copy constructor/assignment
            VoxelMeshInstanced(const VoxelMeshInstanced&) = delete;
            VoxelMeshInstanced& operator=(const VoxelMeshInstanced&) = delete;

            // Delete move constructor/assignment
            VoxelMeshInstanced(VoxelMeshInstanced&& other) = delete;
            VoxelMeshInstanced& operator=(VoxelMeshInstanced&& other) = delete;

            // Rendering

            // Draws the mesh with the given transformation matrix
            // Drawn meshes won't be displayed to the screen until
            // the next call to VoxelMeshInstanced::FlushRenderQueue()
            void Render(const glm::mat4& transform);

            // Flushes internal render queue and displays all meshes
            static void FlushRenderQueue();

            // TODO: Procedural generation
        
        // Data / implementation
        private:

            // Vertex data
            std::vector<VertexVoxel> vertices;
            
            // Constants
            static inline const size_t MAX_DRAW_CALLS = 1'024;

            // Static mesh resources
            static inline Shader* shader = nullptr;
            static inline VertexAttributes* vao = nullptr;
            static inline GPUBuffer* cubeBuffer = nullptr;
            static inline GPUBuffer* voxelDataBuffer = nullptr;
            static inline GPUBuffer* transformBuffer = nullptr;
            static inline GPUBuffer* indirectBuffer = nullptr;

            // Reference counting for static resources
            static inline size_t refCount = 0;

            // Render counters
            static inline size_t meshDrawCount = 0;
            static inline size_t vertexDrawCount = 0;
    };
}