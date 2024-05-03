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
    // Represents a sky with separate day / night skybox textures
    class Sky : public BaseComponent
    {
        // Interface
        public:

            // Convenient normalized time of day constants
            static inline float SUNRISE = 0.0f;
            static inline float NOON = 0.25f;
            static inline float SUNSET = 0.5f;
            static inline float MIDNIGHT = 0.75f;

            // Loads skybox textures from 2 folders containing all the images for day / night cubemaps
            // Accepts local paths like data:// and user://
            Sky(const std::string& dayMapPath, const std::string& nightMapPath);
            ~Sky();

            // Delete copy constructor/assignment
            Sky(const Sky&) = delete;
            Sky& operator=(const Sky&) = delete;

            // Delete move constructor/assignment
            Sky(Sky&& other) = delete;
            void operator=(Sky&& other) = delete;

            // Sets the time of the sky
            // NOTE: Can use human readable constants from above
            void SetTime(float time) { timeOfDay = std::clamp(time, 0.0f, 1.0f); };
            float GetTime() const { return timeOfDay; };

            // Renders the skybox texture to the current framebuffer
            void RenderSkybox();

            // Renders the sun to the current framebuffer
            void RenderSun();

        // Data / implementation
        private:

            // Skybox textures
            Phi::Cubemap dayMap;
            Phi::Cubemap nightMap;

            // Timing variables

            // Day / night lengths in seconds
            float dayLength = 10.0f;
            float nightLength = 2.0f;

            // Normalized time of day
            // NOTE: See public constants for readable conversions
            float timeOfDay = 0.0f;

            // Visual settings

            // Enables the sun
            bool sun = false;
            bool godRays = false;
            bool lensFlare = false;

            // Static internal resources
            static inline Phi::Shader* skyboxShader = nullptr;
            static inline GLuint dummyVAO = 0;

            // Reference counting for static resources
            static inline int refCount = 0;

            // Active scene pointer for safe destruction
            Scene* activeScene = nullptr;
            friend class Scene;
    };
}