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

            // Generate buffers
            voxelDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(VertexVoxel) *  MAX_VOXELS);

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