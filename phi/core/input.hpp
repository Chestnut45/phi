#pragma once

#include <algorithm>
#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Phi
{
    // Handles all input via GLFW callbacks
    class Input
    {
        // Interface
        public:

            // Constants
            static const int NUM_KEYS = GLFW_KEY_LAST - GLFW_KEY_SPACE;
            
            Input();
            ~Input();

            // Delete copy constructor/assignment
            Input(const Input&) = delete;
            Input& operator=(const Input&) = delete;

            // Delete move constructor/assignment
            Input(Input&& other) = delete;
            void operator=(Input&& other) = delete;

            // Key inputs
            bool IsKeyDown(int key) const;
            bool IsKeyJustDown(int key) const;
            bool IsKeyHeld(int key) const;
            bool IsKeyReleased(int key) const;

            // Mouse inputs
            bool IsLMBDown() const;
            bool IsRMBDown() const;
            bool IsMMBDown() const;
            bool IsLMBJustDown() const;
            bool IsRMBJustDown() const;
            bool IsMMBJustDown() const;
            bool IsLMBHeld() const;
            bool IsRMBHeld() const;
            bool IsMMBHeld() const;
            bool IsLMBReleased() const;
            bool IsRMBReleased() const;
            bool IsMMBReleased() const;
            bool IsMouseCaptured() const { return mouseCaptured; };
            const glm::vec2& GetMousePos() const;
            const glm::vec2& GetMouseDelta() const;
            const glm::vec2& GetMouseScroll() const;

            // Mouse config
            void CaptureMouse() const;
            void ReleaseMouse() const;
            bool EnableRawMouseMotion() const;
            void DisableRawMouseMotion() const;

            // Sets all input callbacks for the given window
            // Must be called once before any instances work
            // NOTE: Phi::App calls this in its constructor, so the user shouldn't have to
            static void Setup(GLFWwindow* window);

        // Data / implementation
        private:

            // Necessary so App can call Poll()
            friend class App;

            // Window pointer
            static GLFWwindow* pWindow;

            // Key input state keeping
            static bool keys[NUM_KEYS];
            static bool prevKeys[NUM_KEYS];

            // Mouse state keeping
            static bool mouseCaptured;
            static inline bool lmbDown = false, rmbDown = false, mmbDown = false;
            static inline bool prevLmbDown = false, prevRmbDown = false, prevMmbDown = false;
            static glm::vec2 mousePos;
            static glm::vec2 prevMousePos;
            static glm::vec2 mouseDelta;
            static glm::vec2 mouseScroll;

            // GLFW callback functions
            static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
            static void MousePosCallback(GLFWwindow* window, double xpos, double ypos);
            static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

            // Updates mouse delta and held keys buffers
            // By default, Phi::App will call this function automatically once per frame,
            // so you should not call it yourself
            static void Poll();
    };
}