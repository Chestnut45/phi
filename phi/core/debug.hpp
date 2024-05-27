#pragma once

#include <string>

#include <phi/core/math/shapes.hpp>
#include <phi/scene/components/camera.hpp>
#include <phi/graphics/shader.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/indirect.hpp>

namespace Phi
{
    // Checks for OpenGL errors and outputs them to the console
    void GLErrorCheck(const std::string& label);

    // Helper methods and debug drawing
    class Debug
    {
        // Interface
        public:

            // Access to singleton instance
            static Debug& Instance()
            {
                static Debug instance;
                return instance;
            }

            // Debug rendering
            static const int MAX_DRAW_CALLS = 16'384;
            void DrawAABB(const AABB& aabb, const glm::vec3& color);
            void FlushShapes(const Camera* camera);

        // Data / implementation
        private:

            // Private ctor / dtor
            Debug();
            ~Debug();

            // Resources
            Shader shader;
            VertexAttributes vao;
            GPUBuffer vertexBuffer{BufferType::DynamicDoubleBuffer, sizeof(VertexPosColor) * 24 * MAX_DRAW_CALLS};
    };
}