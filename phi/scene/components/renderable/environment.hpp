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
#include <phi/scene/components/camera.hpp>

namespace Phi
{
    // Represents an environment used in scenes
    // Includes ambient lighting, skybox textures, sun rendering, and time simulation
    class Environment : public BaseComponent
    {
        // Interface
        public:

            // Convenient normalized time of day constants
            static const inline float SUNRISE = 0.0f;
            static const inline float NOON = 0.25f;
            static const inline float SUNSET = 0.5f;
            static const inline float MIDNIGHT = 0.75f;

            // Loads skybox textures from 2 folders containing all the images for day / night cubemaps
            // Accepts local paths like data:// and user://
            Environment(const std::string& dayMapPath, const std::string& nightMapPath);
            ~Environment();

            // DEBUG: Testing BaseComponent interfaces
            void InspectorGUI() {};

            // Delete copy constructor/assignment
            Environment(const Environment&) = delete;
            Environment& operator=(const Environment&) = delete;

            // Delete move constructor/assignment
            Environment(Environment&& other) = delete;
            void operator=(Environment&& other) = delete;

            // Updates the environment, advancing time by delta
            void Update(float delta);

            // Renders the environment to the default framebuffer
            void Render(const Camera* camera);

            // Renders the skybox texture to the current framebuffer
            void RenderSkybox();

            // Renders the sun to the current framebuffer
            void RenderSun();

            // Accessors / Mutators
            void SetTime(float time) { timeOfDay = std::clamp(time, 0.0f, 1.0f); };
            float GetTime() const { return timeOfDay; };
            void StopTime() { advanceTime = false; }
            void PlayTime() { advanceTime = true; }
            
            // Sets the sun's rotation in radians about the y axis
            void SetSunRotation(float rotation) { sunRotation = rotation; }

        // Data / implementation
        private:

            // Skybox textures
            Phi::Cubemap dayMap;
            Phi::Cubemap nightMap;

            // Timing variables

            // Day / night lengths in seconds
            float dayLength = 600.0f;
            float nightLength = 600.0f;
            float timeOfDay = SUNRISE;
            bool advanceTime = true;

            // Visual settings

            // Sun data
            glm::vec3 sunColor{3.5, 2.0f, 0.9f};
            glm::vec3 sunPos;
            bool renderSun = true;
            bool lensFlare = false;
            bool godRays = true;
            float sunRotation = 0.0f;
            float sunAmbient = 0.032f;
            float sunSize = 32.0f;
            float sunDistance = 256.0f;
            float exposure = 0.145f;
            float decay = 0.975f;
            float density = 0.950f;
            float weight = 0.314f;

            // Static internal resources
            static inline Texture2D* sunTexture = nullptr;
            static inline Shader* sunShader = nullptr;
            static inline Shader* skyboxShader = nullptr;
            static inline GLuint dummyVAO = 0;

            // Reference counting for static resources
            static inline int refCount = 0;

            // Allow imgui access to internals through scene editor
            friend class Scene;
    };
}