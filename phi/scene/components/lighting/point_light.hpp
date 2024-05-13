#pragma once

#include <glm/glm.hpp>

#include <phi/scene/components/base_component.hpp>
#include <phi/graphics/shader.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/vertex_attributes.hpp>

namespace Phi
{
    // Represents a single point light component
    class PointLight : public BaseComponent
    {
        // Interface
        public:

            PointLight();
            ~PointLight();

            // Delete copy constructor/assignment
            PointLight(const PointLight&) = delete;
            PointLight& operator=(const PointLight&) = delete;

            // Delete move constructor/assignment
            PointLight(PointLight&& other) = delete;
            PointLight& operator=(PointLight&& other) = delete;

            // Rendering

            // Queues the point light for rendering (using node's transform if it exists)
            void Render();

            // Renders all queued point lights
            static void FlushRenderQueue(bool pbrPass = true);

            // Mutators
            void SetPosition(const glm::vec3& position) { this->position = position; }
            void SetColor(const glm::vec3& color) { this->color = color; }
            void SetRadius(float radius) { this->radius = radius; }

            // Accessors
            const glm::vec3& GetPosition() const { return position; }
            const glm::vec3& GetColor() const { return color; }
            float GetRadius() const { return radius; }
        
        // Data / implementation
        private:

            // Light data
            glm::vec3 position{0.0f};
            glm::vec3 color{1.0f};
            float radius = 32.0f;

            // Static resources
            static inline Shader* pbrShader = nullptr;
            static inline GPUBuffer* vertexBuffer = nullptr;
            static inline GPUBuffer* indexBuffer = nullptr;
            static inline GPUBuffer* instanceBuffer = nullptr;
            static inline VertexAttributes* vao = nullptr;

            // Proxy geometry data
            static inline std::vector<VertexPos> sphereVerts;
            static inline std::vector<GLuint> sphereInds;

            // Counters
            static inline int refCount = 0;
            static inline int queuedLights = 0;

            // Constants
            static const int MAX_POINT_LIGHTS = 2048;
    };
}