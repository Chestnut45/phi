#pragma once

#include <vector>
#include <string>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <phi/scene/components/base_component.hpp>
#include <phi/graphics/materials.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/shader.hpp>

namespace Phi
{
    // A renderable voxel mesh using implicit vertex data generated in the VS
    class VoxelMesh : public BaseComponent
    {
        // Interface
        public:

            // Constants
            static inline const size_t MAX_VOXELS = 1'048'576;

            // Typedefs
            typedef VertexVoxelHalfPrecision Vertex;

            // Creates an empty voxel mesh
            VoxelMesh();

            // Creates a voxel mesh with the given voxel data
            VoxelMesh(const std::vector<Vertex>& voxels);
            ~VoxelMesh();

            // Delete copy constructor/assignment
            VoxelMesh(const VoxelMesh&) = delete;
            VoxelMesh& operator=(const VoxelMesh&) = delete;

            // Delete move constructor/assignment
            VoxelMesh(VoxelMesh&& other) = delete;
            VoxelMesh& operator=(VoxelMesh&& other) = delete;

            // Rendering

            // Draws the mesh (using node's transform if it exists)
            // Drawn meshes won't be displayed to the screen until
            // the next call to VoxelMesh::FlushRenderQueue()
            void Render();

            // Draws the mesh with the given transformation matrix
            // Drawn meshes won't be displayed to the screen until
            // the next call to VoxelMesh::FlushRenderQueue()
            void Render(const glm::mat4& transform);

            // Flushes internal render queue and displays all meshes
            static void FlushRenderQueue();

            // Data access

            // Read-write access to the internal voxel vertex buffer
            std::vector<Vertex>& Vertices() { return vertices; }
        
        // Data / implementation
        private:

            // Vertex data
            std::vector<Vertex> vertices;

            // Static mesh resources
            static inline Shader* shader = nullptr;
            static inline GLuint dummyVAO = 0;
            static inline GPUBuffer* voxelDataBuffer = nullptr;
            static inline GPUBuffer* meshDataBuffer = nullptr;
            static inline GPUBuffer* indexBuffer = nullptr;
            static inline GPUBuffer* indirectBuffer = nullptr;

            // Constants
            static const int MAX_DRAW_CALLS = 1024;
            static const int NUM_CUBE_INDS = 18;
            static const int NUM_CUBE_VERTS = 8;

            // Reference counting for static resources
            static inline size_t refCount = 0;
            static inline int drawCount = 0;
            static inline int queuedVoxels = 0;

            static void IncreaseReferences();
    };
}