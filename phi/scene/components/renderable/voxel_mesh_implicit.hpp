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
    class VoxelMeshImplicit
    {
        // Interface
        public:

            // Constants
            static inline const size_t MAX_VOXELS = 1'048'576;

            // Creates a voxel mesh with the given voxel data
            VoxelMeshImplicit(const std::vector<VertexVoxel>& voxels);
            ~VoxelMeshImplicit();

            // Delete copy constructor/assignment
            VoxelMeshImplicit(const VoxelMeshImplicit&) = delete;
            VoxelMeshImplicit& operator=(const VoxelMeshImplicit&) = delete;

            // Delete move constructor/assignment
            VoxelMeshImplicit(VoxelMeshImplicit&& other) = delete;
            VoxelMeshImplicit& operator=(VoxelMeshImplicit&& other) = delete;

            // Rendering

            // Draws the mesh with the given transformation matrix
            // Drawn meshes won't be displayed to the screen until
            // the next call to VoxelMeshImplicit::FlushRenderQueue()
            void Render(const glm::mat4& transform);

            // Flushes internal render queue and displays all meshes
            static void FlushRenderQueue();

            // TODO: Procedural generation
        
        // Data / implementation
        private:

            // Vertex data
            std::vector<VertexVoxel> vertices;

            // Static mesh resources
            static inline Shader* shader = nullptr;
            static inline GLuint dummyVAO = 0;
            static inline GPUBuffer* voxelDataBuffer = nullptr;
            static inline GPUBuffer* transformBuffer = nullptr;
            static inline GPUBuffer* indexBuffer = nullptr;
            static inline GPUBuffer* indirectBuffer = nullptr;

            // Constants
            static const int MAX_DRAW_CALLS = 1024;

            // Reference counting for static resources
            static inline size_t refCount = 0;
    };
}