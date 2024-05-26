#include "input.hpp"

namespace Phi
{
    GLFWwindow* Input::pWindow = nullptr;
    bool Input::keys[Input::NUM_KEYS] = {false};
    bool Input::prevKeys[Input::NUM_KEYS] = {false};
    glm::vec2 Input::mousePos;
    glm::vec2 Input::prevMousePos;
    glm::vec2 Input::mouseDelta;
    glm::vec2 Input::mouseScroll;
    bool Input::mouseCaptured = false;

    Input::Input()
    {
    }

    Input::~Input()
    {
    }

    bool Input::IsKeyDown(int key) const
    {
        return keys[key - GLFW_KEY_SPACE];
    }

    bool Input::IsKeyJustDown(int key) const
    {
        return (keys[key - GLFW_KEY_SPACE] && !prevKeys[key - GLFW_KEY_SPACE]);
    }

    bool Input::IsLMBDown() const { return lmbDown; };
    bool Input::IsRMBDown() const { return rmbDown; };
    bool Input::IsMMBDown() const { return mmbDown; };

    bool Input::IsLMBJustDown() const { return lmbDown && !prevLmbDown; };
    bool Input::IsRMBJustDown() const { return rmbDown && !prevRmbDown; };
    bool Input::IsMMBJustDown() const { return mmbDown && !prevMmbDown; };

    const glm::vec2& Input::GetMousePos() const
    {
        return mousePos;
    }

    const glm::vec2& Input::GetMouseDelta() const
    {
        return mouseDelta;
    }

    const glm::vec2& Input::GetMouseScroll() const
    {
        return mouseScroll;
    }

    void Input::CaptureMouse() const
    {
        // Calculate center of screen
        int width, height, x, y;
        glfwGetWindowSize(pWindow, &width, &height);
        x = width >> 1;
        y = height >> 1;

        // Set cursor position and update state
        glfwSetCursorPos(pWindow, x, y);
        mousePos.x = x;
        mousePos.y = y;
        prevMousePos = mousePos;
        mouseDelta = glm::vec2(0.0f);

        // Actually capture the mouse
        glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
    }

    void Input::ReleaseMouse() const
    {
        // Release the mouse
        glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;

        // Ensure there are no jumps in delta from releasing the mouse
        double x, y;
        glfwGetCursorPos(pWindow, &x, &y);
        mousePos.x = x;
        mousePos.y = y;
        prevMousePos = mousePos;
        mouseDelta = glm::vec2(0.0f);
    }

    bool Input::EnableRawMouseMotion() const
    {
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            return true;
        }
        return false;
    }

    void Input::DisableRawMouseMotion() const
    {
        glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }

    void Input::Setup(GLFWwindow* window)
    {
        glfwSetKeyCallback(window, Input::KeyCallback);
        glfwSetCursorPosCallback(window, Input::MousePosCallback);
        glfwSetScrollCallback(window, Input::MouseScrollCallback);
        pWindow = window;
    }
    
    void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        keys[key - GLFW_KEY_SPACE] = action != GLFW_RELEASE;
    }

    void Input::MousePosCallback(GLFWwindow* window, double xpos, double ypos)
    {
    }

    void Input::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        mouseScroll.x = xoffset;
        mouseScroll.y = yoffset;
    }

    void Input::Poll()
    {
        // Update previous keys
        for (int i = 0; i < NUM_KEYS; i++)
        {
            prevKeys[i] = keys[i];
        }

        // Update mouse buttons
        prevLmbDown = lmbDown;
        prevRmbDown = rmbDown;
        prevMmbDown = mmbDown;
        lmbDown = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        rmbDown = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        mmbDown = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        // Update mouse positions
        prevMousePos = mousePos;
        double x, y;
        glfwGetCursorPos(pWindow, &x, &y);
        mousePos.x = x;
        mousePos.y = y;
        mouseDelta = mousePos - prevMousePos;

        // Reset mouse scroll
        mouseScroll = glm::vec2(0.0f);
    }
}