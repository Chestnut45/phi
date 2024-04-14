#pragma once

#include <vector>
#include <string>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <phi/graphics/materials.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/shader.hpp>
#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // A basic renderable mesh
    class BasicMesh : public BaseComponent
    {
        // Interface
        public:

            // Convenience vertex declaration
            typedef Phi::VertexPosNorm Vertex;

            BasicMesh();
            ~BasicMesh();

            // Delete copy constructor/assignment
            BasicMesh(const BasicMesh&) = delete;
            BasicMesh& operator=(const BasicMesh&) = delete;

            // Delete move constructor/assignment
            BasicMesh(BasicMesh&& other) = delete;
            BasicMesh& operator=(BasicMesh&& other) = delete;

            // Rendering

            // Draws the mesh with the given transformation matrix,
            // or the identity matrix if none is supplied
            // Drawn meshes won't be displayed to the screen until 
            // the next call to BasicMesh::FlushRenderQueue()
            void Render(const glm::mat4& transform = glm::mat4(1.0f));

            // Flushes internal render queue and displays all meshes
            static void FlushRenderQueue();

            // Procedural generation

            // Adds a box to the mesh with the given dimensions and offset
            void AddBox(float width, float height, float depth, const glm::vec3& offset = glm::vec3(0.0f));

            // Adds a cube to the mesh with the given side length
            void AddCube(float sideLength = 1.0f, const glm::vec3& offset = glm::vec3(0.0f));

            // Adds an icosphere to the mesh with the given radius
            void AddIcosphere(float radius, int subdivisions = 1, const glm::vec3& offset = glm::vec3(0.0f));

            // TODO: More proc gen options + offsets / rotations to all existing methods

            // Material management

            // Sets the current material to the correct ID if it exists
            void SetMaterial(const std::string& name);

            // Returns the current ID of the material used by this mesh
            int GetMaterial() const { return material; };

            // Normal generation

            // Generates flat shaded normals for each triangle in the provided vectors
            static void GenerateNormalsFlat(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
            static void GenerateNormalsFlat(std::vector<Vertex>& vertices);

            // Generates smooth shaded normals, accounting for shared vertices
            static void GenerateNormalsSmooth(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
        
        // Data / implementation
        private:

            // Vertex data
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;

            // Material id (relative to scene)
            int material = 0;
            
            // Constants
            // TODO: Enforce limit on single meshes for all vertex generation methods
            // Phi::Error("Mesh exceeds vertex/index limit, consider changing BasicMesh::MAX_[VERT/IND]ICES");
            static inline const size_t MAX_VERTICES = 131'072;
            static inline const size_t MAX_INDICES = MAX_VERTICES * 3;
            static inline const size_t MAX_DRAW_CALLS = 1'024;

            // Static mesh resources
            static inline Shader* shader = nullptr;
            static inline VertexAttributes* vao = nullptr;
            static inline GPUBuffer* vertexBuffer = nullptr;
            static inline GPUBuffer* indexBuffer = nullptr;
            static inline GPUBuffer* meshDataBuffer = nullptr;
            static inline GPUBuffer* indirectBuffer = nullptr;

            // Reference counting for static resources
            static inline size_t refCount = 0;

            // Render counters
            static inline size_t meshDrawCount = 0;
            static inline size_t vertexDrawCount = 0;
            static inline size_t indexDrawCount = 0;
    };
}