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
                0, 4, 6, 0, 6, 2

                // Negative faces (unused)
                // 6, 5, 7, 6, 4, 5,
                // 2, 6, 3, 6, 7, 3,
                // 7, 1, 3, 7, 5, 1
            };

            // Generate static index buffer
            GLuint* indexData = new GLuint[18 * MAX_VOXELS];
            for (int i = 0; i < MAX_VOXELS; ++i)
            {
                for (int j = 0; j < 18; ++j)
                {
                    indexData[i * 18 + j] = cubeInds[j] + i * 8;
                }
            }

            // Generate buffers
            indexBuffer = new GPUBuffer(BufferType::Static, sizeof(GLuint) * 18 * MAX_VOXELS, indexData);
            voxelDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(VertexVoxel) * MAX_VOXELS);
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
        if (drawCount == MAX_DRAW_CALLS) FlushRenderQueue();

        // Sync if necessary
        if (drawCount == 0) indirectBuffer->Sync();

        // Create indirect command
        DrawElementsCommand cmd;
        cmd.count = 18 * vertices.size();
        cmd.firstIndex = 0;
        cmd.baseVertex = 0;
        cmd.instanceCount = 1;
        cmd.baseInstance = queuedVoxels; // So the vs knows where to start in the voxel buffer for this object

        // Write the command
        indirectBuffer->Write(cmd);

        // Write the voxel data and transform
        voxelDataBuffer->Write(vertices.data(), vertices.size() * sizeof(VertexVoxel));
        transformBuffer->Write(transform);

        // Update counters
        drawCount++;
        queuedVoxels += vertices.size();
    }

    void VoxelMeshImplicit::FlushRenderQueue()
    {
        if (drawCount == 0) return;

        // Bind resources
        glBindVertexArray(dummyVAO);
        shader->Use();
        indexBuffer->Bind(GL_ELEMENT_ARRAY_BUFFER);
        indirectBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);
        voxelDataBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 3, voxelDataBuffer->GetCurrentSection() * voxelDataBuffer->GetSize(), voxelDataBuffer->GetSize());
        transformBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 4, transformBuffer->GetCurrentSection() * transformBuffer->GetSize(), transformBuffer->GetSize());

        // Issue draw call
        glDisable(GL_CULL_FACE);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(indirectBuffer->GetCurrentSection() * indirectBuffer->GetSize()), drawCount, 0);
        glEnable(GL_CULL_FACE);

        // Unbind
        glBindVertexArray(0);

        // Lock buffers
        indirectBuffer->Lock();
        indirectBuffer->SwapSections();
        voxelDataBuffer->SwapSections();
        transformBuffer->SwapSections();
        
        // Reset counters
        drawCount = 0;
        queuedVoxels = 0;
    }
}