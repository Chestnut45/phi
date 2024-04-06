#include "app.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Phi
{
    void ErrorCallback(int error, const char* const description)
    {
        Log(description);
        exit(error);
    }

    void WindowResizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
        App* pApp = (App*)glfwGetWindowUserPointer(window);
        pApp->wWidth = width;
        pApp->wHeight = height;
        pApp->windowResized = true;
    }

    App::App(const std::string& name, int glMajVer, int glMinVer) : name(name), wWidth(defaultWidth), wHeight(defaultHeight)
    {
        // Initialize GLFW
        if (!glfwInit())
        {
            FatalError("Failed to initialize GLFW");
            exit(1);
        }
        Log("GLFW initialized");

        // Set callbacks
        glfwSetErrorCallback(ErrorCallback);

        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajVer);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinVer);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        pWindow = glfwCreateWindow(defaultWidth, defaultHeight, name.c_str(), NULL, NULL);
        if (!pWindow)
        {
            FatalError("Failed to create window");
            exit(1);
        }

        // Set other callbacks
        glfwSetWindowSizeCallback(pWindow, WindowResizeCallback);

        // Setup input
        Input::Setup(pWindow);

        // Set user pointer and make context current
        glfwSetWindowUserPointer(pWindow, this);
        glfwMakeContextCurrent(pWindow);

        // Vsync
        glfwSwapInterval(0);

        // Initialize GLEW
        GLenum status = glewInit();
        if (status != GLEW_OK)
        {
            FatalError((const char* const)glewGetErrorString(status));
            exit(1);
        }
        Log("GLEW initialized");
        
        // Output current OpenGL context version
        Log("OpenGL Context: ", glGetString(GL_VERSION));

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        // Setup docking
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Edit default style
        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);

        // Setup Dear ImGui Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(GetWindow(), true);
        ImGui_ImplOpenGL3_Init();
    }

    App::~App()
    {
        // Shutdown ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        Log("ImGui shutdown");

        // De-init GLFW
        glfwDestroyWindow(pWindow);
        glfwTerminate();
        Log("GLFW terminated");
    }

    void App::Run()
    {
        lastTime = glfwGetTime();
        while (!glfwWindowShouldClose(pWindow) && !input.IsKeyDown(GLFW_KEY_END))
        {
            // Update timing
            double currentTime = glfwGetTime();
            float elapsedTime = (float)(currentTime - lastTime);
            programLifetime += elapsedTime;
            lastTime = currentTime;

            // Calculate FPS
            static float timeAccum = 0;
            timeAccum += elapsedTime;
            timingFrameCount++;
            totalFrameCount++;
            while (timeAccum >= fpsUpdateRate)
            {
                // 5 updates / second
                averageFPS = (float)timingFrameCount / timeAccum;
                timingFrameCount = 0;
                timeAccum -= fpsUpdateRate;
            }
            
            // Poll for inputs
            input.Poll();
            glfwPollEvents();

            // Ensure framebuffer has non-zero size
            glfwGetFramebufferSize(pWindow, &wWidth, &wHeight);
            if (wWidth != 0 && wHeight != 0)
            {
                // Init ImGui frame
                // NOTE: Done here so we can use ImGUI in either Update() or Render()
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                ImGui::DockSpaceOverViewport((const ImGuiViewport*)__null, ImGuiDockNodeFlags_PassthruCentralNode);

                // Update and measure time
                Update(elapsedTime);
                lastUpdate = (glfwGetTime() - currentTime);

                // Clear the default framebuffer
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                
                // Render and measure time
                Render();
                lastRender = (glfwGetTime() - lastUpdate - currentTime);

                // Finish ImGui rendering
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            // Update samples
            static float sampleAccum = 0;
            sampleAccum += elapsedTime;
            while (sampleAccum >= sampleRate)
            {
                updateSamples.push_back(lastUpdate * 1000);
                renderSamples.push_back(lastRender * 1000);
                while (updateSamples.size() > perfSamplesPerSecond) updateSamples.erase(updateSamples.begin());
                while (renderSamples.size() > perfSamplesPerSecond) renderSamples.erase(renderSamples.begin());
                sampleAccum -= sampleRate;
            }

            glfwSwapBuffers(pWindow);
        }
    }

    void App::ShowDebug()
    {
        ImGui::Begin("App Debug");
        
        // Performance monitoring
        ImGui::Text("Performance:");
        ImGui::Separator();
        ImGui::Text("Average FPS: %.0f", averageFPS);
        ImGui::PlotLines("Update:", updateSamples.data(), updateSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
        ImGui::SameLine();
        ImGui::Text("%.2fms", lastUpdate * 1000);
        ImGui::PlotLines("Render:", renderSamples.data(), renderSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
        ImGui::SameLine();
        ImGui::Text("%.2fms", lastRender * 1000);
        ImGui::NewLine();
        
        ImGui::Text("Settings:");
        ImGui::Separator();
        if (ImGui::Checkbox("Vsync", &vsync)) glfwSwapInterval(vsync);
        if (ImGui::Checkbox("Fullscreen", &fullscreen))
        {
            GLFWwindow* window = GetWindow();
            if (fullscreen)
            {
                // Get primary monitor and enable fullscreen
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else
            {
                // Get window monitor and revert to windowed mode
                GLFWmonitor* monitor = glfwGetWindowMonitor(window);
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, NULL, (mode->width - defaultWidth) / 2, (mode->height - defaultHeight) / 2, defaultWidth, defaultHeight, 0);
            }
        }

        ImGui::End();
    }
}