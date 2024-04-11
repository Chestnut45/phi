#include "basic_mesh.hpp"

#include <cstdint>
#include <unordered_map>
#include <glm/gtx/vector_angle.hpp>

#include <phi/core/logging.hpp>
#include <phi/graphics/geometry.hpp>
#include <phi/graphics/indirect.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    BasicMesh::BasicMesh()
    {
        if (refCount == 0)
        {
            // Initialize static resources

            shader = new Shader();
            shader->LoadSource(GL_VERTEX_SHADER, "phi/graphics/shaders/basic_mesh.vs");
            shader->LoadSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/basic_mesh.fs");
            shader->Link();

            vertexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(Vertex) * MAX_VERTICES);
            indexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(GLuint) * MAX_INDICES);
            meshDataBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, (sizeof(glm::mat4) + sizeof(GLint)) * MAX_DRAW_CALLS);
            indirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawElementsCommand) * MAX_DRAW_CALLS);

            // Initialize with base vertex attributes from VertexPosNorm
            vao = new VertexAttributes(VertexFormat::POS_NORM, vertexBuffer, indexBuffer);

            // Add per-mesh data via instanced array attributes
            vao->Bind();
            meshDataBuffer->Bind(GL_ARRAY_BUFFER);
            
            // First add the attributes for the matrix
            // Attributes can only hold 4 components so we use 4 consecutive
            // vec4s and then consume them in the vertex shader as a single matrix
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(GLint), 0);
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(GLint), sizeof(glm::vec4));
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(GLint), sizeof(glm::vec4) * 2);
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::mat4) + sizeof(GLint), sizeof(glm::vec4) * 3);

            // Per-mesh material is a single int attribute
            vao->AddAttribute(1, GL_INT, 1, sizeof(glm::mat4) + sizeof(GLint), sizeof(glm::vec4) * 4);

            // Log
            Phi::Log("BasicMesh resources initialized");
        }

        refCount++;
    }

    BasicMesh::~BasicMesh()
    {
        refCount--;

        if (refCount == 0)
        {
            // Cleanup static resources
            delete shader;
            delete vertexBuffer;
            delete indexBuffer;
            delete meshDataBuffer;
            delete indirectBuffer;
            delete vao;
        }
    }

    void BasicMesh::Render(const glm::mat4& transform)
    {
        // Check to ensure the buffers can contain this mesh currently
        if (meshDrawCount >= MAX_DRAW_CALLS ||
            vertexDrawCount + vertices.size() >= MAX_VERTICES ||
            indexDrawCount + indices.size() >= MAX_INDICES)
        {
            // Some buffer is full, flush the queue and then add
            FlushRenderQueue();

            // NOTE: Degenerate case should not occur since all vertex
            // generation functions enforce the MAX_[VERT/IND]EX limit
        }

        // Sync the buffers if this is the first draw call since last render
        // If this has been signaled, the other buffers must also not be being read from
        if (meshDrawCount == 0) indirectBuffer->Sync();

        // Create the indirect draw command
        DrawElementsCommand cmd;
        cmd.count = indices.size();
        cmd.instanceCount = 1;
        cmd.firstIndex = indexDrawCount + (MAX_INDICES * indexBuffer->GetCurrentSection());
        cmd.baseVertex = vertexDrawCount + (MAX_VERTICES * vertexBuffer->GetCurrentSection());
        cmd.baseInstance = meshDrawCount + (MAX_DRAW_CALLS * meshDataBuffer->GetCurrentSection());

        // Write the draw command to the buffer
        indirectBuffer->Write(cmd);

        // Write the vertices / indices to the buffer
        vertexBuffer->Write(vertices.data(), vertices.size() * sizeof(Vertex));
        indexBuffer->Write(indices.data(), indices.size() * sizeof(GLuint));

        // Write the per-mesh transform and material
        meshDataBuffer->Write(transform);
        meshDataBuffer->Write(material);

        // Increase internal counters
        meshDrawCount++;
        vertexDrawCount += vertices.size();
        indexDrawCount += indices.size();
    }

    void BasicMesh::FlushRenderQueue()
    {
        // Ensure we only flush if necessary
        if (meshDrawCount == 0) return;

        // Bind the relevant resources
        vao->Bind();
        shader->Use();
        indirectBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);

        // Issue the multi draw command
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(indirectBuffer->GetCurrentSection() * indirectBuffer->GetSize()), meshDrawCount, 0);
        vao->Unbind();

        // Set a lock and reset buffers
        indirectBuffer->Lock();
        indirectBuffer->SwapSections();
        meshDataBuffer->SwapSections();
        vertexBuffer->SwapSections();
        indexBuffer->SwapSections();

        // Reset counters
        meshDrawCount = 0;
        vertexDrawCount = 0;
        indexDrawCount = 0;
    }

    void BasicMesh::AddBox(float width, float height, float depth, const glm::vec3& offset)
    {
        std::vector<Vertex> newVerts;
        std::vector<GLuint> newInds;

        // Add all the indices
        for (GLuint i : Cube::UNIT_CUBE_INDICES)
        {
            newInds.push_back(i);
        }

        // Add all the vertices from internal geometry data
        for (const VertexPos& v : Cube::UNIT_CUBE_VERTICES)
        {
            newVerts.push_back({v.x * width + offset.x,
                                v.y * height + offset.y,
                                v.z * depth + offset.z,
                                0.0f,
                                0.0f,
                                0.0f});
        }

        // Generate flat shaded normals by default
        GenerateNormalsFlat(newVerts, newInds);

        // Calculate the new indices and add to the global buffer
        GLuint baseIndex = vertices.size();
        for (GLuint i : newInds)
        {
            indices.push_back(baseIndex + i);
        }

        // Add the final vertices
        for (const auto& vertex : newVerts)
        {
            vertices.push_back(vertex);
        }
    }

    void BasicMesh::AddCube(float sideLength, const glm::vec3& offset)
    {
        AddBox(sideLength, sideLength, sideLength, offset);
    }

    void BasicMesh::AddIcosphere(float radius, int subdivisions, const glm::vec3& offset)
    {
        // Vertex containers for the new sphere
        std::vector<Vertex> newVerts;
        std::vector<GLuint> newInds;

        // Cache for middle points
        std::unordered_map<uint64_t, GLuint> middlePointCache{};

        // Local lambda to compute new vertices
        auto AddMiddlePoint = [&newVerts, &newInds, &middlePointCache](GLuint i1, GLuint i2)
        {
            // Check if in cache
            uint64_t smaller = i1 < i2 ? i1 : i2;
            uint64_t larger = i1 > i2 ? i1 : i2;
            uint64_t key = (smaller << 32) + larger;

            // Return cached value if it exists
            if (middlePointCache.count(key) > 0)
            {
                return middlePointCache.at(key);
            }

            // Grab the 2 vertices
            const Vertex& v1 = newVerts[i1];
            const Vertex& v2 = newVerts[i2];

            // The new index to cache and return
            GLuint newIndex = newVerts.size();

            // Create the new midpoint vertex
            glm::vec3 n((v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f, (v1.z + v2.z) * 0.5f);
            n = glm::normalize(n);

            // Push it back to the vertex array
            newVerts.push_back({n.x, n.y, n.z, 0.0f, 0.0f, 0.0f});
            
            // Cache and return the index
            middlePointCache[key] = newIndex;
            return newIndex;
        };

        // Add initial vertices
        for (const VertexPos& vert : Icosphere::UNIT_ICOSPHERE_VERTICES)
        {
            glm::vec3 v = glm::normalize(glm::vec3(vert.x, vert.y, vert.z));
            newVerts.push_back({v.x, v.y, v.z, 0.0f, 0.0f, 0.0f});
        }

        // Add initial indices
        for (GLuint i : Icosphere::UNIT_ICOSPHERE_INDICES)
        {
            newInds.push_back(i);
        }

        // Subdivide sphere vertices
        for (int i = 0; i < subdivisions; i++)
        {
            // Containers for next level of icosphere
            std::vector<GLuint> subdInds;

            // Iterate all triangles
            for (int i = 0; i <= newInds.size() - 3; i += 3)
            {
                // Grab the 3 indices that make up the triangle
                GLuint a = newInds[i];
                GLuint b = newInds[i + 1];
                GLuint c = newInds[i + 2];

                // Grab the mid point indices
                GLuint na = AddMiddlePoint(a, b);
                GLuint nb = AddMiddlePoint(b, c);
                GLuint nc = AddMiddlePoint(c, a);

                // Add the 4 triangles
                subdInds.push_back(a);
                subdInds.push_back(na);
                subdInds.push_back(nc);
                subdInds.push_back(b);
                subdInds.push_back(nb);
                subdInds.push_back(na);
                subdInds.push_back(c);
                subdInds.push_back(nc);
                subdInds.push_back(nb);
                subdInds.push_back(na);
                subdInds.push_back(nb);
                subdInds.push_back(nc);
            }

            // Replace our indices with the new ones
            newInds = subdInds;
        }

        // Generate the normals
        GenerateNormalsSmooth(newVerts, newInds);

        // Add the final indices
        GLuint baseVertex = vertices.size();
        for (GLuint i : newInds)
        {
            indices.push_back(baseVertex + i);
        }

        // Scale vertices to given radius and apply offset, then add to mesh
        for (auto& v : newVerts)
        {
            v.x = v.x * radius + offset.x;
            v.y = v.y * radius + offset.y;
            v.z = v.z * radius + offset.z;
            vertices.push_back(v);
        }
    }

    void BasicMesh::SetMaterial(const std::string& name)
    {
        // Ensure we are attached to a node
        Node* node = GetNode();
        if (!node)
        {
            Phi::Error("BasicMesh was not created using Node::AddComponent<T> and cannot be assigned a material");
            return;
        }

        // Retrieve the material ID
        int materialID = node->GetScene()->GetMaterialID(name);

        if (materialID)
        {
            material = materialID;
        }
        else
        {
            Phi::Error("Invalid material name: ", name);
        }
    }

    void BasicMesh::GenerateNormalsFlat(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
    {
        for (int i = 0; i <= indices.size() - 3; i += 3)
        {
            // Grab the 3 vertices that make up this triangle
            auto& a = vertices[indices[i]];
            auto& b = vertices[indices[i + 1]];
            auto& c = vertices[indices[i + 2]];
            glm::vec3 A = {a.x, a.y, a.z};
            glm::vec3 B = {b.x, b.y, b.z};
            glm::vec3 C = {c.x, c.y, c.z};

            // Calculate the normal
            glm::vec3 normal = glm::normalize(glm::cross(B - A, C - A));

            // Set normal for all vertices
            a.nx = normal.x;
            a.ny = normal.y;
            a.nz = normal.z;
            b.nx = normal.x;
            b.ny = normal.y;
            b.nz = normal.z;
            c.nx = normal.x;
            c.ny = normal.y;
            c.nz = normal.z;
        }
    }

    void BasicMesh::GenerateNormalsSmooth(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
    {
        // Set all normals to zero first
        for (auto& v : vertices)
        {
            v.nx = 0.0f;
            v.ny = 0.0f;
            v.nz = 0.0f;
        }
        
        // Add all contributing normals
        for (int i = 0; i <= indices.size() - 3; i += 3)
        {
            // Grab the 3 vertices that make up this triangle
            auto& a = vertices[indices[i]];
            auto& b = vertices[indices[i + 1]];
            auto& c = vertices[indices[i + 2]];
            glm::vec3 A = {a.x, a.y, a.z};
            glm::vec3 B = {b.x, b.y, b.z};
            glm::vec3 C = {c.x, c.y, c.z};

            // Calculate angles between vertices
            float a1 = glm::angle(B - A, C - A);
            float a2 = glm::angle(C - B, A - B);
            float a3 = glm::angle(A - C, B - C);

            // Calculate the facet normal
            glm::vec3 normal = glm::cross(B - A, C - A);

            // Calculate the angle-weighted normals
            glm::vec3 n1 = normal * a1;
            glm::vec3 n2 = normal * a2;
            glm::vec3 n3 = normal * a3;

            // Accumulate the weighted normals
            a.nx += n1.x;
            a.ny += n1.y;
            a.nz += n1.z;
            b.nx += n2.x;
            b.ny += n2.y;
            b.nz += n2.z;
            c.nx += n3.x;
            c.ny += n3.y;
            c.nz += n3.z;
        }

        // Normalize normals for each vertex
        for (auto& v : vertices)
        {
            glm::vec3 normalized = glm::normalize(glm::vec3(v.nx, v.ny, v.nz));
            v.nx = normalized.x;
            v.ny = normalized.y;
            v.nz = normalized.z;
        }
    }
}