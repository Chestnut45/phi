#pragma once

#include <algorithm>
#include <vector>
#include <string>

#include <phi/graphics/cubemap.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/texture_2d.hpp>
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

            // Updates the sky, advancing time by delta
            void Update(float delta);

            // Renders the skybox texture to the current framebuffer
            void RenderSkybox();

            // Renders the sun to the current framebuffer
            void RenderSun();

            // Accessors / Mutators
            void SetTime(float time) { timeOfDay = std::clamp(time, 0.0f, 1.0f); };
            float GetTime() const { return timeOfDay; };

        // Data / implementation
        private:

            // Skybox textures
            Phi::Cubemap dayMap;
            Phi::Cubemap nightMap;

            // Timing variables

            // Day / night lengths in seconds
            float dayLength = 60.0f;
            float nightLength = 60.0f;
            float timeOfDay = 0.0f;
            bool advanceTime = true;

            // Visual settings

            // Sun data
            glm::vec3 sunColor{1.0f, 0.85f, 0.64f};
            glm::vec3 sunPos{0.0f};
            float sunAmbient = 0.032f;
            float sunSize = 64.0f;
            float sunDistance = 256.0f;
            bool renderSun = true;
            bool lensFlare = false;
            bool godRays = true;
            float exposure = 0.24f;
            float decay = 0.974f;
            float density = 0.95f;
            float weight = 0.25f;

            // Static internal resources
            static inline Texture2D* sunTexture = nullptr;
            static inline Shader* sunShader = nullptr;
            static inline Shader* skyboxShader = nullptr;
            static inline GLuint dummyVAO = 0;

            // Reference counting for static resources
            static inline int refCount = 0;

            // Active scene pointer for safe destruction
            Scene* activeScene = nullptr;
            friend class Scene;
    };
}