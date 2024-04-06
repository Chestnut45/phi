#pragma once

// DEPRECATION WARNING:
// This class is deprecated and will be removed at a later date.
// It is around only for backwards compatibility with the cityscape assignment.

#include <phi/graphics/mesh.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/texture_2d.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/vertex.hpp>
#include <phi/graphics/shader.hpp>

namespace Phi
{
    // Dynamic vertex data batching and rendering logic
    // Usage:
    // 1. Have some meshes that share a vertex format
    // 2. Create RenderBatch instance with initial buffer size
    // 3. Every Frame:
    //  a. AddMesh() all the meshes you want to draw
    //  b. Flush()
    //
    // Limitations:
    // 1. Does not support textures
    template <typename Vertex>
    class RenderBatch
    {
        // Interface
        public:

            RenderBatch(size_t maxVertices, size_t maxIndices = 0);
            ~RenderBatch();
            
            // Delete copy constructor/assignment
            RenderBatch(const RenderBatch&) = delete;
            RenderBatch& operator=(const RenderBatch&) = delete;

            // Delete move constructor/assignment
            RenderBatch(RenderBatch&& other) = delete;
            void operator=(RenderBatch&& other) = delete;

            // Batching / rendering methods
            bool AddMesh(const Mesh<Vertex>& mesh);
            void Flush(const Shader& shader);
        
        // Data / implementation
        private:

            // State / stats
            size_t maxVertices;
            size_t maxIndices;
            size_t vertexCount = 0;
            size_t indexCount = 0;
            int drawCount = 0;
            bool useIndices;
            GLenum mode = GL_TRIANGLES;

            // OpenGL Resources
            GPUBuffer* vertexBuffer = nullptr;
            GPUBuffer* indexBuffer = nullptr;
            VertexAttributes* vertexAttributes = nullptr;
    };

    // Templated code implementation

    template <typename Vertex>
    RenderBatch<Vertex>::RenderBatch(size_t maxVertices, size_t maxIndices) : maxVertices(maxVertices), maxIndices(maxIndices)
    {
        // Use indices if user supplies an index buffer size
        useIndices = maxIndices != 0;

        // Initialize resources
        vertexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, maxVertices * sizeof(Vertex));

        if (useIndices)
        {
            indexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, maxIndices * sizeof(GLuint));
        }
        
        // VAO creation (Depends on vertex format)
        if (std::is_same_v<Vertex, VertexPos>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosColor>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_COLOR, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosColorNorm>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_COLOR_NORM, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosColorNormUv>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_COLOR_NORM_UV, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosColorNormUv1Uv2>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_COLOR_NORM_UV1_UV2, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosColorUv>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_COLOR_UV, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosNorm>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_NORM, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosNormUv>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_NORM_UV, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        else if (std::is_same_v<Vertex, VertexPosUv>)
        {
            vertexAttributes = new VertexAttributes(VertexFormat::POS_UV, vertexBuffer, useIndices ? indexBuffer : nullptr);
        }
        
        if (!vertexAttributes) Error("Renderbatch constructor: Custom vertex format not supported yet, please use one of the internal vertex formats");
    }

    template <typename Vertex>
    RenderBatch<Vertex>::~RenderBatch()
    {
        // Free all OpenGL resources
        delete vertexBuffer;
        if (indexBuffer) delete indexBuffer;
        delete vertexAttributes;
    }

    template <typename Vertex>
    bool RenderBatch<Vertex>::AddMesh(const Mesh<Vertex>& mesh)
    {
        const std::vector<Vertex>& meshVerts = mesh.GetVertices();
        const std::vector<GLuint>& meshInds = mesh.GetIndices();

        // Ensure we have room to add to the batch
        if (vertexCount + meshVerts.size() > maxVertices ||
            (useIndices && indexCount + meshInds.size() > maxIndices))
        {
            return false;
        }
        
        // Sync buffers if this is the first write since last flush
        if (drawCount == 0)
        {
            // Ensure OpenGL is not reading from this section of our buffers
            vertexBuffer->Sync();
            indexBuffer->Sync();
        }

        // Copy index data
        if (useIndices)
        {
            // Ensure we have enough space to hold the indices for the current mesh
            static std::vector<GLuint> offsetIndices;
            offsetIndices.reserve(meshInds.size());

            // Offset the indices before writing them
            std::transform(meshInds.cbegin(), meshInds.cend(), offsetIndices.begin(), [this](GLuint original) { return original + vertexCount; });
            indexBuffer->Write(offsetIndices.data(), meshInds.size() * sizeof(GLuint));

            // Increase counter
            indexCount += meshInds.size();
        }

        // Write vertex data
        vertexBuffer->Write(meshVerts.data(), meshVerts.size() * sizeof(Vertex));
        vertexCount += meshVerts.size();

        drawCount++;

        return true;
    }

    template <typename Vertex>
    void RenderBatch<Vertex>::Flush(const Shader& shader)
    {
        if (useIndices)
        {
            // Bind resources
            vertexAttributes->Bind();
            shader.Use();

            // Issue draw call
            glDrawElementsBaseVertex(mode, indexCount, GL_UNSIGNED_INT,
                                    (void*)(indexBuffer->GetSize() * indexBuffer->GetCurrentSection()),
                                    maxVertices * vertexBuffer->GetCurrentSection());
        
            // Insert a fence sync
            vertexBuffer->Lock();
            indexBuffer->Lock();

            // Needed for double-buffering
            vertexBuffer->SwapSections();
            indexBuffer->SwapSections();

            // Reset counters
            vertexCount = 0;
            indexCount = 0;
            drawCount = 0;

            // Unbind VAO
            glBindVertexArray(0);
        }
        else
        {
            // Bind resources
            vertexAttributes->Bind();
            shader.Use();

            // Issue draw call
            glDrawArrays(mode, maxVertices * vertexBuffer->GetCurrentSection(), vertexCount);
        
            // Insert a fence sync
            vertexBuffer->Lock();

            // Needed for double-buffering
            vertexBuffer->SwapSections();

            // Reset counters
            vertexCount = 0;
            indexCount = 0;
            drawCount = 0;
            
            // Unbind VAO
            glBindVertexArray(0);
        }
    }
}