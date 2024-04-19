#include "voxel_mesh_implicit.hpp"

#include <phi/graphics/geometry.hpp>

namespace Phi
{
    VoxelMeshImplicit::VoxelMeshImplicit(const std::vector<VertexVoxel>& voxels)
    {
        if (refCount == 0)
        {
            // Initialize static resources

            shader = new Shader();
            shader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/voxel_mesh_implicit.vs");
            shader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/voxel_mesh_implicit.fs");
            shader->Link();

            // Dummy vao
            glGenVertexArrays(1, &dummyVAO);

            // Cube indices
            static GLuint cubeInds[] =
            {
                0, 2, 1, 2, 3, 1,
                5, 4, 1, 1, 4, 0,
                0, 4, 6, 0, 6, 2,
                6, 5, 7, 6, 4, 3,
                2, 6, 3, 6, 7, 3,
                7, 1, 3, 7, 5, 1
            };

            // Generate static index buffer
            GLuint* indexData = new GLuint[36 * MAX_VOXELS];
            for (int i = 0; i < MAX_VOXELS; ++i)
            {
                for (int j = 0; j < 36; ++j)
                {
                    indexData[i * 36 + j] = cubeInds[j];
                }
            }

            // Generate buffers
            indexBuffer = new GPUBuffer(BufferType::Static, sizeof(GLuint) * 36 * MAX_VOXELS, indexData);
            voxelDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(VertexVoxel) * MAX_VOXELS * MAX_DRAW_CALLS);
            transformBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(glm::mat4) * MAX_DRAW_CALLS);
            indirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawElementsCommand) * MAX_DRAW_CALLS);

            // Free up heap memory used for initial buffer construction
            delete indexData;

            // Debug Logging
            Phi::Log("VoxelMeshImplicit resources initialized");
        }
        
        // Copy voxel data
        vertices = voxels;

        refCount++;
    }

    VoxelMeshImplicit::~VoxelMeshImplicit()
    {
        refCount--;

        if (refCount == 0)
        {
            // Cleanup static resources
            delete shader;
            delete voxelDataBuffer;
            delete transformBuffer;
            delete indexBuffer;
            delete indirectBuffer;
            glDeleteVertexArrays(1, &dummyVAO);
        }
    }

    void VoxelMeshImplicit::Render(const glm::mat4& transform)
    {
        // Write the voxels to the buffer
        voxelDataBuffer->Sync();
        voxelDataBuffer->Write(vertices.data(), vertices.size() * sizeof(VertexVoxel));
        voxelDataBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 3, voxelDataBuffer->GetCurrentSection() * voxelDataBuffer->GetSize(), voxelDataBuffer->GetSize());
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 18);
        glBindVertexArray(0);
        voxelDataBuffer->Lock();
        voxelDataBuffer->SwapSections();
    }

    void VoxelMeshImplicit::FlushRenderQueue()
    {
    }
}