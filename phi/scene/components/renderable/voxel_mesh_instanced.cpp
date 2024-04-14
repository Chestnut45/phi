#include "voxel_mesh_instanced.hpp"

#include <phi/graphics/geometry.hpp>
#include <phi/scene/components/renderable/basic_mesh.hpp>

namespace Phi
{
    VoxelMeshInstanced::VoxelMeshInstanced(const std::vector<VertexVoxel>& voxels)
    {
        if (refCount == 0)
        {
            // Initialize static resources

            shader = new Shader();
            shader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/voxel_mesh_instanced.vs");
            shader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/voxel_mesh_instanced.fs");
            shader->Link();

            // Generate unit cube with normals without any indices
            int pattern[] = {0, 2, 1, 1, 2, 3};
            std::vector<VertexPosNorm> cubeVerts;
            for (int i  = 0; i < 24; i += 4)
            {
                for (int j = 0; j < 6; ++j)
                {
                    VertexPosNorm v;
                    v.x = Cube::UNIT_CUBE_VERTICES[i + pattern[j]].x;
                    v.y = Cube::UNIT_CUBE_VERTICES[i + pattern[j]].y;
                    v.z = Cube::UNIT_CUBE_VERTICES[i + pattern[j]].z;
                    cubeVerts.push_back(v);
                }
            }
            // Generate normals
            BasicMesh::GenerateNormalsFlat(cubeVerts);

            // Generate buffers
            cubeBuffer = new GPUBuffer(BufferType::Static, sizeof(VertexPosNorm) * cubeVerts.size(), cubeVerts.data());
            voxelDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(VertexVoxel) *  MAX_VOXELS);
            transformBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(glm::mat4) * MAX_DRAW_CALLS);
            indirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawArraysCommand) * MAX_DRAW_CALLS);

            // Initialize with base vertex attributes from VertexPosNorm
            vao = new VertexAttributes(VertexFormat::POS_NORM, cubeBuffer);

            // Add per-voxel data via an instanced array attribute
            vao->Bind();
            voxelDataBuffer->Bind(GL_ARRAY_BUFFER);
            vao->AddAttribute(3, GL_INT, 1, sizeof(glm::ivec4), 0);
            vao->AddAttribute(1, GL_INT, 1, sizeof(glm::ivec4), sizeof(glm::ivec3));

            // Debug Logging
            Phi::Log("VoxelMeshInstanced resources initialized");
        }
        
        // Copy voxel data
        vertices = voxels;

        refCount++;
    }

    VoxelMeshInstanced::~VoxelMeshInstanced()
    {
        refCount--;

        if (refCount == 0)
        {
            // Cleanup static resources
            delete shader;
            delete cubeBuffer;
            delete voxelDataBuffer;
            delete indirectBuffer;
            delete transformBuffer;
            delete vao;
        }
    }

    void VoxelMeshInstanced::Render(const glm::mat4& transform)
    {
        // Check to ensure the buffers can contain this mesh currently
        if (meshDrawCount >= MAX_DRAW_CALLS || vertexDrawCount + vertices.size() >= MAX_VOXELS)
        {
            FlushRenderQueue();
        }

        // Sync the buffers if this is the first draw call since last render
        // If this has been signaled, the other buffers must also not be being read from
        if (meshDrawCount == 0) indirectBuffer->Sync();

        // Create the indirect draw command
        DrawArraysCommand cmd;
        cmd.count = 36;
        cmd.instanceCount = vertices.size();
        cmd.first = 0;
        cmd.baseInstance = vertexDrawCount;

        // Write the draw command to the buffer
        indirectBuffer->Write(cmd);

        // Write the voxels to the buffer
        voxelDataBuffer->Write(vertices.data(), vertices.size() * sizeof(VertexVoxel));

        // Write the per-mesh matrices
        transformBuffer->Write(transform);

        // Increase internal counters
        meshDrawCount++;
        vertexDrawCount += vertices.size();
    }

    void VoxelMeshInstanced::FlushRenderQueue()
    {
        // Ensure we only flush if necessary
        if (meshDrawCount == 0) return;

        // Bind the relevant resources
        vao->Bind();
        shader->Use();
        indirectBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);
        transformBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 0, transformBuffer->GetCurrentSection() * transformBuffer->GetSize(), transformBuffer->GetSize());

        // Issue the multi draw command
        glMultiDrawArraysIndirect(GL_TRIANGLES, (void*)(indirectBuffer->GetCurrentSection() * indirectBuffer->GetSize()), meshDrawCount, 0);
        vao->Unbind();

        // Set a lock and reset buffers
        indirectBuffer->Lock();
        indirectBuffer->SwapSections();
        voxelDataBuffer->SwapSections();
        transformBuffer->SwapSections();

        // Reset counters
        meshDrawCount = 0;
        vertexDrawCount = 0;
    }
}