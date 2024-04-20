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

                // Negative faces (unused)
                // 6, 5, 7, 6, 4, 5,
                // 2, 6, 3, 6, 7, 3,
                // 7, 1, 3, 7, 5, 1
            };

            // Generate static index buffer
            GLuint* indexData = new GLuint[NUM_CUBE_INDS * MAX_VOXELS];
            for (int i = 0; i < MAX_VOXELS; ++i)
            {
                for (int j = 0; j < NUM_CUBE_INDS; ++j)
                {
                    indexData[i * NUM_CUBE_INDS + j] = cubeInds[j] + i * NUM_CUBE_VERTS;
                }
            }

            // Generate buffers
            indexBuffer = new GPUBuffer(BufferType::Static, sizeof(GLuint) * NUM_CUBE_INDS * MAX_VOXELS, indexData);
            voxelDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(VertexVoxel) * MAX_VOXELS);
            meshDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(glm::mat4) * 2 * MAX_DRAW_CALLS);
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
            delete meshDataBuffer;
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
        cmd.count = NUM_CUBE_INDS * vertices.size();
        cmd.firstIndex = 0;
        cmd.baseVertex = 0;
        cmd.instanceCount = 1;
        cmd.baseInstance = queuedVoxels; // So the vs knows where to start in the voxel buffer for this object

        // Write the command
        indirectBuffer->Write(cmd);

        // Write the voxel and mesh data
        voxelDataBuffer->Write(vertices.data(), vertices.size() * sizeof(VertexVoxel));
        meshDataBuffer->Write(transform);
        meshDataBuffer->Write(glm::inverse(transform));

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
        meshDataBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 4, meshDataBuffer->GetCurrentSection() * meshDataBuffer->GetSize(), meshDataBuffer->GetSize());

        // Issue draw call
        // NOTE: Culling disabled for mirroring optimization
        glDisable(GL_CULL_FACE);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(indirectBuffer->GetCurrentSection() * indirectBuffer->GetSize()), drawCount, 0);
        glEnable(GL_CULL_FACE);

        // Unbind
        glBindVertexArray(0);

        // Lock buffers
        indirectBuffer->Lock();
        indirectBuffer->SwapSections();
        voxelDataBuffer->SwapSections();
        meshDataBuffer->SwapSections();
        
        // Reset counters
        drawCount = 0;
        queuedVoxels = 0;
    }
}