#include "app.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <phi/core/file.hpp>

namespace Phi
{
    void SetupImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        
        style.Alpha = 1.0f;
        style.DisabledAlpha = 1.0f;
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.WindowMinSize = ImVec2(20.0f, 20.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.ChildRounding = 4.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 4.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(8.0f, 4.0f);
        style.FrameRounding = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(4.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.CellPadding = ImVec2(8.0f, 8.0f);
        style.IndentSpacing = 20.0f;
        style.ColumnsMinSpacing = 0.0f;
        style.ScrollbarSize = 16.0f;
        style.ScrollbarRounding = 16.0f;
        style.GrabMinSize = 16.0f;
        style.GrabRounding = 32.0f;
        style.TabRounding = 4.0f;
        style.TabBorderSize = 0.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
        
        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09411764889955521f, 0.1019607856869698f, 0.1176470592617989f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1137254908680916f, 0.125490203499794f, 0.1529411822557449f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.501960813999176f, 0.4524247348308563f, 0.3188927471637726f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1803921610116959f, 0.1882352977991104f, 0.196078434586525f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1529411822557449f, 0.1529411822557449f, 0.1529411822557449f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.1411764770746231f, 0.1647058874368668f, 0.2078431397676468f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1803921610116959f, 0.1882352977991104f, 0.196078434586525f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.1529411822557449f, 0.1529411822557449f, 0.1529411822557449f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.1294117718935013f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1978316158056259f, 0.2240933626890182f, 0.250980406999588f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.1391003578901291f, 0.157565638422966f, 0.1764705926179886f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.95686274766922f, 0.95686274766922f, 0.95686274766922f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95686274766922f, 0.95686274766922f, 0.95686274766922f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2891195714473724f, 0.3012777864933014f, 0.3137255012989044f, 1.0f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2901960909366608f, 0.3019607961177826f, 0.3137255012989044f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.2901960909366608f, 0.3019607961177826f, 0.3137255012989044f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(9.999907888413873e-07f, 9.999899930335232e-07f, 9.999999974752427e-07f, 0.501960813999176f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(9.999918120229268e-07f, 9.999899930335232e-07f, 9.999999974752427e-07f, 0.501960813999176f);
        style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.8823529481887817f, 0.7960784435272217f, 0.5607843399047852f, 1.0f);

        // Load icon font
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        float baseFontSize = 20.0f;
        float iconFontSize = baseFontSize * 2.0f / 3.0f;
        static const ImWchar iconRange[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = iconFontSize;
        iconConfig.GlyphOffset.y = 1.5f;
        io.Fonts->AddFontFromFileTTF("thirdparty/imgui/" FONT_ICON_FILE_NAME_FAS, iconFontSize, &iconConfig, iconRange);
    }

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

    App::App(const std::string& name, int width, int height) : name(name), wWidth(width), wHeight(height)
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
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        pWindow = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
        if (!pWindow)
        {
            FatalError("Failed to create window");
            exit(1);
        }

        // Set other callbacks
        glfwSetWindowSizeCallback(pWindow, WindowResizeCallback);

        // Setup input
        Input::Setup(pWindow);

        // Setup filesystem (generate user and data folder paths)
        File::Init();

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

        // Setup style
        SetupImGuiStyle();

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
            elapsedTime = (float)(currentTime - lastTime);
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
                ImGui::DockSpaceOverViewport(0U, (const ImGuiViewport*)__null, ImGuiDockNodeFlags_PassthruCentralNode);

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
                totalSamples.push_back(elapsedTime * 1000);
                while (updateSamples.size() > perfSamplesPerSecond) updateSamples.erase(updateSamples.begin());
                while (renderSamples.size() > perfSamplesPerSecond) renderSamples.erase(renderSamples.begin());
                while (totalSamples.size() > perfSamplesPerSecond) totalSamples.erase(totalSamples.begin());
                sampleAccum -= sampleRate;
            }

            glfwSwapBuffers(pWindow);
        }
    }

    void App::ToggleFullscreen()
    {
        // Toggle
        fullscreen = !fullscreen;

        // Apply new mode
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
            glfwSetWindowMonitor(window, NULL, (mode->width - wWidth) / 2, (mode->height - wHeight) / 2, wWidth, wHeight, 0);
        }
    }

    void App::ToggleVsync()
    {
        // Toggle
        vsync = !vsync;

        // Apply
        glfwSwapInterval(vsync);
    }

    void App::ShowDebug()
    {
        // Default window positioning
        ImGui::SetNextWindowPos(ImVec2(wWidth - 256, 0));
        ImGui::SetNextWindowSize(ImVec2(256, 254));
        ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        // Performance monitoring
        ImGui::SeparatorText("Performance:");
        ImGui::Text("Average FPS: %.0f", averageFPS);
        ImGui::PlotLines("Update:", updateSamples.data(), updateSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
        ImGui::SameLine();
        ImGui::Text("%.2fms", lastUpdate * 1000);
        ImGui::PlotLines("Render:", renderSamples.data(), renderSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
        ImGui::SameLine();
        ImGui::Text("%.2fms", lastRender * 1000);
        ImGui::PlotLines("Total:", totalSamples.data(), totalSamples.size(), 0, (const char*)nullptr, 0.0f, 16.67f, ImVec2{128.0f, 32.0f});
        ImGui::SameLine();
        ImGui::Text("%.2fms", elapsedTime * 1000);

        // Window settings
        ImGui::SeparatorText("Window Settings");
        bool v = vsync;
        bool f = fullscreen;
        if (ImGui::Checkbox("Fullscreen", &f)) ToggleFullscreen();
        if (ImGui::Checkbox("Vsync", &v)) ToggleVsync();

        ImGui::End();
    }
}