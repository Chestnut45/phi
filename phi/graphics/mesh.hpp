#pragma once

// DEPRECATION WARNING:
// This class is deprecated and will be removed at a later date.
// It is around only for backwards compatibility with the cityscape assignment.

#include <iostream>
#include <type_traits> // Important for std::is_same_v
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <glm/glm.hpp>

#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/texture_2d.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/vertex.hpp>
#include <phi/graphics/shader.hpp>

namespace Phi
{
    // Texture unit mapping
    // When rendering a mesh, its textures will be mapped to these units
    // You can use explicit layouts in your shader's samplers to access the textures (4.2+),
    // or set them directly with glUniform1i(samplerLoc, texUnit).
    // Ex: layout(binding = 0) uniform sampler2D myAlbedo1Sampler;
    enum class TexUnit : int
    {
        ALBEDO_1,       // 0
        ALBEDO_2,       // 1
        SPECULAR_1,     // 2
        SPECULAR_2,     // 3
        NORMAL_1,       // 4
        NORMAL_2,       // 5

        MAX_TEXTURES    // 6

        // Unused units: GL_TEXTURE6+ for your custom textures
    };

    // SSBO binding points
    enum class SSBOBinding : int
    {
        MaterialBuffer, // 0 (Unused for now)
        InstanceBuffer, // 1
    };

    // Static mesh resources (shared by all meshes)
    // Must be in a separate non-template class, otherwise, all templated
    // class instantiations would have their own "static" resources
    class MeshResources
    {
        // All mesh types and models are friends of MeshResources
        template <typename Vertex>
        friend class Mesh;
        friend class Model;

        // Mesh texture data format
        struct Texture
        {
            Texture2D* texture = nullptr;
            TexUnit unit = TexUnit::ALBEDO_1;
            std::string path = {};
            int refCount = 0;
        };

        // Reference counter for all meshes
        static inline int refCount = 0;

        // Texture storage for all meshes
        static inline std::unordered_map<std::string, Texture> loadedTextures;

        // Instance buffer for all meshes
        static inline GPUBuffer* instanceBuffer = nullptr;
        static const size_t INSTANCE_BUFFER_SIZE = sizeof(glm::mat4) * 10'000;

        // Reference counter helpers
        static void IncreaseReferences()
        {
            // Initialize static resources for all meshes
            if (refCount == 0)
            {
                std::cout << "First mesh, instance buffer initialized" << std::endl;
                instanceBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, INSTANCE_BUFFER_SIZE);
            }

            refCount++;
        }

        static void DecreaseReferences()
        {
            refCount--;

            // Delete static resources if we are the last mesh
            if (refCount == 0)
            {
                std::cout << "Last mesh destroyed, instance buffer deleted" << std::endl;
                delete instanceBuffer;
            }
        }
    };

    // Represents a renderable mesh of arbitrary format
    template <typename Vertex>
    class Mesh
    {
        // Interface
        public:

            // Manual / procedural constructors
            Mesh(bool useIndices = true);
            Mesh(const std::vector<Vertex>* const vertices, const std::vector<GLuint>* const indices = nullptr);

            // Destructor
            ~Mesh();

            // Delete copy constructor/assignment
            Mesh(const Mesh&) = delete;
            Mesh& operator=(const Mesh&) = delete;

            // Explicit move constructor
            Mesh(Mesh&& other)
            {
                // Steal resources from other mesh
                vertices = other.vertices;
                indices = other.indices;
                useIndices = other.useIndices;
                vertexAttributes = other.vertexAttributes;
                vertexBuffer = other.vertexBuffer;
                indexBuffer = other.indexBuffer;

                // Ensure other mesh doesn't free resources we're stealing on its destruction
                other.vertexAttributes = nullptr;
                other.vertexBuffer = nullptr;
                other.indexBuffer = nullptr;

                // Steal all textures and set others to nullptr
                // so the texture refCounts are unaffected
                useTextures = other.useTextures;
                for (int i = 0; i < (int)TexUnit::MAX_TEXTURES; ++i)
                {
                    textures[i] = other.textures[i];
                    other.textures[i] = nullptr;
                }

                std::cout << "Mesh moved from " << &other << " to " << this << std::endl;
            };

            // Delete move assignment operator
            void operator=(Mesh&& other) = delete;

            // Vertex data generation
            void AddSurface(const std::vector<Vertex>* vertices, const std::vector<GLuint>* const indices = nullptr);
            void AddTriangle(const Vertex& a, const Vertex& b, const Vertex& c);
            void AddQuad(const Vertex& topLeft, const Vertex& topRight,
                         const Vertex& bottomLeft, const Vertex& bottomRight);

            // Texture loading
            void AddTexture(TexUnit type,
                            const std::string& path,
                            GLint wrapU, GLint wrapV,
                            GLenum minFilter, GLenum magFilter,
                            bool mipmap = false);

            // Commits all mesh data to GPU resources in preperation for rendering:
            // Generates VAO, VBO, EBO (if applicable)
            // If this mesh will only be drawn by a RenderBatch object, you do not have to Commit() any resources
            void Commit();

            // Immediately render to the current FBO
            void Draw(const Shader& shader) const;

            // Immediately render iData.size() or instanceCount instances of the mesh to the current FBO
            // NOTE: The first method uploads iData to the static instance buffer, while the second method
            // only binds the relevant resources and issues the draw call, so the user may use their own instancing method
            template <typename InstanceData>
            void DrawInstances(const Shader& shader, const std::vector<InstanceData>& iData) const;
            void DrawInstances(const Shader& shader, int instanceCount) const;

            // Clear all mesh data, cleanup resources, return to initial state
            void Reset();

            // Const data accessors
            const std::vector<Vertex>& GetVertices() const { return vertices; };
            const std::vector<GLuint>& GetIndices() const { return indices; };
        
        // Data / implementation
        private:

            friend class Model;

            // Mesh data
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            bool useIndices;
            bool useTextures = false;
            GLenum mode = GL_TRIANGLES;

            // OpenGL Resources
            MeshResources::Texture* textures[(int)TexUnit::MAX_TEXTURES] = {nullptr};
            VertexAttributes* vertexAttributes = nullptr;
            GPUBuffer* vertexBuffer = nullptr;
            GPUBuffer* indexBuffer = nullptr;
    };

    // Template implementation

    template <typename Vertex>
    Mesh<Vertex>::Mesh(bool useIndices) : useIndices(useIndices)
    {
        MeshResources::IncreaseReferences();
    }

    template <typename Vertex>
    Mesh<Vertex>::Mesh(const std::vector<Vertex>* vertices, const std::vector<GLuint>* const indices)
    {
        this->vertices = *vertices;
        useIndices = indices; // nullptr == 0 == false for our bool if not supplied

        // Copy indices if they exist
        if (useIndices)
        {
            this->indices = *indices;
        }

        MeshResources::IncreaseReferences();
    }

    template <typename Vertex>
    Mesh<Vertex>::~Mesh()
    {
        Reset();
        MeshResources::DecreaseReferences();
    }

    template <typename Vertex>
    void Mesh<Vertex>::AddSurface(const std::vector<Vertex>* vertices, const std::vector<GLuint>* const indices)
    {
        // Get the number of vertices we have before adding the surface
        GLuint vertCount = this->vertices.size();

        // Add the vertices
        this->vertices.insert(this->vertices.end(), vertices->cbegin(), vertices->cend());

        // Add indices if the mesh uses them
        if (useIndices)
        {
            if (indices)
            {
                // Ensure we have enough space to write the offset indices
                GLuint indCount = this->indices.size();
                this->indices.resize(indCount + indices->size());
                std::transform(indices->cbegin(), indices->cend(), this->indices.begin() + indCount, [vertCount](GLuint original) { return original + vertCount; });
            }
            else
            {
                // Generate indices automatically if this surface supplies none
                for (GLuint ind : indices)
                {
                    this->indices.push_back(ind + vertCount);
                }
            }
        }
    }

    template <typename Vertex>
    void Mesh<Vertex>::AddTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
    {
        GLuint n = vertices.size();

        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);

        if (useIndices)
        {
            indices.push_back(n);
            indices.push_back(n + 1);
            indices.push_back(n + 2);
        }
    }

    template <typename Vertex>
    void Mesh<Vertex>::AddQuad(const Vertex& topLeft, const Vertex& topRight,
                               const Vertex& bottomLeft, const Vertex& bottomRight)
    {
        GLuint n = vertices.size();
        
        if (useIndices)
        {
            vertices.push_back(topLeft);
            vertices.push_back(topRight);
            vertices.push_back(bottomLeft);
            vertices.push_back(bottomRight);

            indices.push_back(n);
            indices.push_back(n + 2);
            indices.push_back(n + 1);
            indices.push_back(n + 1);
            indices.push_back(n + 2);
            indices.push_back(n + 3);
        }
        else
        {
            vertices.push_back(topLeft);
            vertices.push_back(bottomLeft);
            vertices.push_back(topRight);
            
            vertices.push_back(topRight);
            vertices.push_back(bottomLeft);
            vertices.push_back(bottomRight);
        }
    }

    template <typename Vertex>
    void Mesh<Vertex>::AddTexture(TexUnit type,
                                  const std::string& path,
                                  GLint wrapU, GLint wrapV,
                                  GLenum minFilter, GLenum magFilter,
                                  bool mipmap)
    {
        // Ensure this mesh doesn't already have this texture type loaded
        if (textures[(int)type])
        {
            std::cout << "Mesh @" << this << " already contains a texture of type " << (int)type << std::endl;
            return;
        }

        // Load texture from disk if not loaded
        if (MeshResources::loadedTextures.count(path) == 0)
        {
            // Load from disk if not loaded yet
            Texture2D* tex = new Texture2D(path, wrapU, wrapV, minFilter, magFilter, mipmap);
            MeshResources::loadedTextures[path].texture = tex;
            MeshResources::loadedTextures[path].path = path;
            MeshResources::loadedTextures[path].unit = type;
        }

        // Point to the static texture resource and increase reference counter
        textures[(int)type] = &MeshResources::loadedTextures[path];
        MeshResources::loadedTextures[path].refCount++;

        // Update state
        if (!useTextures) useTextures = true;

        // Final output if successful
        std::cout << "Mesh texture loaded: " << path << std::endl;
    }

    template <typename Vertex>
    void Mesh<Vertex>::Commit()
    {
        // Create VBO and EBO
        vertexBuffer = new GPUBuffer(BufferType::Static, sizeof(Vertex) * vertices.size(), vertices.data());

        if (useIndices)
        {
            indexBuffer = new GPUBuffer(BufferType::Static, sizeof(GLuint) * indices.size(), indices.data());
        }
        
        // VAO creation (Depends on vertex format)
        // This is ugly, but it works
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
        
        if (!vertexAttributes) Error("Mesh Commit: Custom vertex format not supported yet, please use one of the internal vertex formats");

        std::cout << "Mesh resources committed to VRAM" << std::endl;
    }


    template <typename Vertex>
    void Mesh<Vertex>::Draw(const Shader& shader) const
    {
        shader.Use();

        // Bind the VAO
        vertexAttributes->Bind();

        // Bind all textures
        if (useTextures)
        {
            for (MeshResources::Texture* tex : textures)
            {
                if (tex) tex->texture->Bind((int)tex->unit);
            }
        }

        // Issue draw call
        if (useIndices)
        {
            glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(mode, 0, vertices.size());
        }

        // Unbind VAO
        glBindVertexArray(0);
    }


    template <typename Vertex>
    template <typename InstanceData>
    void Mesh<Vertex>::DrawInstances(const Shader& shader, const std::vector<InstanceData>& iData) const
    {
        shader.Use();

        // Bind the VAO
        vertexAttributes->Bind();

        // Bind all textures
        if (useTextures)
        {
            for (MeshResources::Texture* tex : textures)
            {
                if (tex) tex->texture->Bind((int)tex->unit);
            }
        }

        // Upload instance data and bind the buffer
        MeshResources::instanceBuffer->Sync();
        MeshResources::instanceBuffer->Write(iData.data(), iData.size() * sizeof(InstanceData));
        MeshResources::instanceBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, (int)SSBOBinding::InstanceBuffer, MeshResources::INSTANCE_BUFFER_SIZE * MeshResources::instanceBuffer->GetCurrentSection(), MeshResources::INSTANCE_BUFFER_SIZE);

        // Issue draw call
        if (useIndices)
        {
            glDrawElementsInstanced(mode, indices.size(), GL_UNSIGNED_INT, 0, iData.size());
        }
        else
        {
            glDrawArraysInstanced(mode, 0, vertices.size(), iData.size());
        }

        // Lock the buffer section and switch to the next one
        MeshResources::instanceBuffer->Lock();
        MeshResources::instanceBuffer->SwapSections();

        // Unbind VAO
        glBindVertexArray(0);
    }

    template <typename Vertex>
    void Mesh<Vertex>::DrawInstances(const Shader& shader, int instanceCount) const
    {
        shader.Use();

        // Bind the VAO
        vertexAttributes->Bind();

        // Bind all textures
        if (useTextures)
        {
            for (MeshResources::Texture* tex : textures)
            {
                if (tex) tex->texture->Bind((int)tex->unit);
            }
        }

        // Issue draw call
        if (useIndices)
        {
            glDrawElementsInstanced(mode, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
        }
        else
        {
            glDrawArraysInstanced(mode, 0, vertices.size(), instanceCount);
        }

        // Unbind VAO
        glBindVertexArray(0);
    }

    template <typename Vertex>
    void Mesh<Vertex>::Reset()
    {
        vertices.clear();
        indices.clear();

        // Manage static texture resources from our pointer
        for (MeshResources::Texture* tex : textures)
        {
            if (tex)
            {
                tex->refCount--;

                // Delete static resources if we were the last mesh using them
                if (tex->refCount == 0)
                {
                    // This automatically calls the Texture2D object's destructor
                    delete tex->texture;
                    MeshResources::loadedTextures.erase(tex->path);
                    tex = nullptr;
                    
                    std::cout << "Texture unloaded: unused by any meshes" << std::endl;
                }
            }
        }
        useTextures = false;

        if (vertexAttributes) { delete vertexAttributes; vertexAttributes = nullptr; }
        if (vertexBuffer) { delete vertexBuffer; vertexBuffer = nullptr; }
        if (indexBuffer) { delete indexBuffer; indexBuffer = nullptr; }
    }
}