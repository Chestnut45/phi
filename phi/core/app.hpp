#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

// Dear ImGui: https://github.com/ocornut/imgui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <glm/glm.hpp>

#include <phi/core/input.hpp>
#include <phi/core/logging.hpp>
#include <phi/core/math/rng.hpp>

namespace Phi
{
    // Main app class, handles OpenGL context creation and input
    class App
    {
        public:

            // Creates an app with the given name and OpenGL version
            App(const std::string& name, int glMajVer = 4, int glMinVer = 5);
            virtual ~App();

            // Main methods
            virtual void Run();
            virtual void Update(float delta) = 0;
            virtual void Render() = 0;

            // Graphics / Window management
            void ToggleFullscreen();
            void ToggleVsync();

            // Accessors
            GLFWwindow* GetWindow() const { return pWindow; };
            glm::vec2 GetWindowSize() const { return {wWidth, wHeight}; };

        protected:

            // App details
            std::string name;
            int glMajorVersion;
            int glMinorVersion;

            // Window details
            GLFWwindow* pWindow;
            int wWidth = 0;
            int wHeight = 0;
            bool windowResized = true;
            static const int defaultWidth = 1280;
            static const int defaultHeight = 720;

            // Timing
            float programLifetime = 0;
            float lastUpdate = 0;
            float lastRender = 0;
            float lastTime = 0;
            float elapsedTime = 0;
            float averageFPS = 0;
            uint32_t timingFrameCount = 0;
            uint32_t totalFrameCount = 0;
            std::vector<float> updateSamples;
            std::vector<float> renderSamples;
            std::vector<float> totalSamples;
            static const int perfSamplesPerSecond = 240;
            static inline float sampleRate = 1.0f / perfSamplesPerSecond;
            static inline float fpsUpdateRate = 1.0f / 2.0f;

            // Graphical settings
            bool vsync = false;
            bool fullscreen = false;

            // Input handler
            Input input{};

            // Random number generator
            RNG rng{4545};

            // Displayes debug information about the application in an imgui window
            void ShowDebug();

        private:

            // Friends who should have access to private data
            friend void WindowResizeCallback(GLFWwindow* window, int width, int height);
    };
}