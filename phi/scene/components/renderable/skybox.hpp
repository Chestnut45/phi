#pragma once

#include <algorithm>
#include <vector>
#include <string>

#include <phi/graphics/cubemap.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/vertex_attributes.hpp>
#include <phi/graphics/shader.hpp>
#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // Represents a skybox with separate day / night textures
    class Skybox : public BaseComponent
    {
        // Interface
        public:

            // Loads a skybox from 2 folders containing all the images for day / night cubemaps
            // Accepts local paths like data:// and user://
            Skybox(const std::string& dayMapPath, const std::string& nightMapPath);
            ~Skybox();

            // Delete copy constructor/assignment
            Skybox(const Skybox&) = delete;
            Skybox& operator=(const Skybox&) = delete;

            // Delete move constructor/assignment
            Skybox(Skybox&& other) = delete;
            void operator=(Skybox&& other) = delete;

            // Set blend factor between day / night textures
            // 0 = noon, 1 = midnight
            // Values outside of those bounds will be clamped
            void SetTime(float time) { timeOfDay = std::clamp(time, 0.0f, 1.0f); };
            float GetTime() const { return timeOfDay; };

            // Renders the skybox to the current framebuffer
            void Render();

        // Data / implementation
        private:

            // Skybox textures
            Phi::Cubemap dayMap;
            Phi::Cubemap nightMap;

            // Blend factor between textures
            // 0 = noon, 1 = midnight
            float timeOfDay = 1.0f;

            // Static internal resources
            static inline Phi::GPUBuffer* skyboxVBO = nullptr;
            static inline Phi::VertexAttributes* skyboxVAO = nullptr;
            static inline Phi::Shader* skyboxShader = nullptr;

            // Reference counting for static resources
            static inline int refCount = 0;

            // Active scene pointer for safe destruction
            Scene* activeScene = nullptr;
            friend class Scene;
    };
}