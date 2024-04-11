#include "voxel_mesh.hpp"

// DEBUG
#include <phi/core/math/rng.hpp>

#include <phi/core/logging.hpp>
#include <phi/graphics/geometry.hpp>
#include <phi/graphics/indirect.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    VoxelMesh::VoxelMesh()
    {
        if (refCount == 0)
        {
            // Initialize static resources

            shader = new Shader();
            shader->LoadSource(GL_VERTEX_SHADER, "phi/graphics/shaders/voxel_mesh.vs");
            shader->LoadSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/voxel_mesh.fs");
            shader->Link();

            vertexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(Vertex) * MAX_VOXELS);
            meshDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, (sizeof(glm::mat4) + sizeof(glm::mat3) * 2) * MAX_DRAW_CALLS);
            indirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawArraysCommand) * MAX_DRAW_CALLS);

            // Initialize with base vertex attributes from VertexPosNorm
            vao = new VertexAttributes(VertexFormat::VOXEL, vertexBuffer);

            // Add per-mesh data via instanced array attributes
            vao->Bind();
            meshDataBuffer->Bind(GL_ARRAY_BUFFER);
            
            // Add the attributes for the model matrix
            // Attributes can only hold 4 components so we use 4 consecutive
            // vec4s and then consume them in the vertex shader as a single matrix
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, 0);
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4));
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 2);
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 3);

            // Add the attributes for the rotation matrix
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4);
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4 + sizeof(glm::vec3));
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4 + sizeof(glm::vec3) * 2);

            // And the inverse rotation matrix
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4 + sizeof(glm::vec3) * 3);
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4 + sizeof(glm::vec3) * 4);
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(glm::mat3) * 2, sizeof(glm::vec4) * 4 + sizeof(glm::vec3) * 5);

            // Debug Logging
            Phi::Log("VoxelMesh resources initialized");
        }

        // DEBUG: test verts
        RNG rng(4545);
        int distance = 20;
        for (int x = -distance; x < distance; x++)
        {
            for (int y = -distance; y < distance; y++)
            {
                for (int z = -distance; z < distance; z++)
                {
                    if (sqrt(x * x + y * y + z * z) < distance) vertices.push_back({x, y, z, rng.NextInt(1, 13)});
                }
            }
        }

        refCount++;
    }

    VoxelMesh::~VoxelMesh()
    {
        refCount--;

        if (refCount == 0)
        {
            // Cleanup static resources
            delete shader;
            delete vertexBuffer;
            delete meshDataBuffer;
            delete indirectBuffer;
            delete vao;
        }
    }

    void VoxelMesh::Render(const glm::mat4& transform, const glm::mat3& rotation)
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
        cmd.count = vertices.size();
        cmd.instanceCount = 1;
        cmd.first = vertexDrawCount + (MAX_VOXELS * vertexBuffer->GetCurrentSection());
        cmd.baseInstance = meshDrawCount + (MAX_DRAW_CALLS * meshDataBuffer->GetCurrentSection());

        // Write the draw command to the buffer
        indirectBuffer->Write(cmd);

        // Write the vertices to the buffer
        vertexBuffer->Write(vertices.data(), vertices.size() * sizeof(Vertex));

        // Write the per-mesh matrices
        meshDataBuffer->Write(transform);
        meshDataBuffer->Write(rotation);
        meshDataBuffer->Write(glm::inverse(rotation));

        // Increase internal counters
        meshDrawCount++;
        vertexDrawCount += vertices.size();
    }

    void VoxelMesh::FlushRenderQueue()
    {
        // Ensure we only flush if necessary
        if (meshDrawCount == 0) return;

        // Bind the relevant resources
        vao->Bind();
        shader->Use();
        indirectBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);

        // Issue the multi draw command
        glMultiDrawArraysIndirect(GL_POINTS, (void*)(indirectBuffer->GetCurrentSection() * indirectBuffer->GetSize()), meshDrawCount, 0);
        vao->Unbind();

        // Set a lock and reset buffers
        indirectBuffer->Lock();
        indirectBuffer->SwapSections();
        meshDataBuffer->SwapSections();
        vertexBuffer->SwapSections();

        // Reset counters
        meshDrawCount = 0;
        vertexDrawCount = 0;
    }
}