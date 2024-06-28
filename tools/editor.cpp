#include "editor.hpp"

Editor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new Editor();
    app->Run();
    delete app;
    return 0;
}

Editor::Editor() : App("Phi Engine | Editor", 4, 6)
{
    // Enable raw mouse if accepted
    input.EnableRawMouseMotion();

    // Initialize scene
    scene.SetRenderMode(Scene::RenderMode::Texture);

    // Materials
    scene.LoadMaterials("data://materials.yaml");

    // Main camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 16, 128});
    scene.SetActiveCamera(camera);

    // Add a point light to the camera
    camera.GetNode()->AddComponent<PointLight>();

    // DEBUG: Environment
    Environment& env = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveEnvironment(env);
}

Editor::~Editor()
{
    // TODO: Shutdown logic
}

void Editor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        scene.SetResolution(wWidth - 256, wHeight);
        windowResized = false;
    }

    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE)) input.IsMouseCaptured() ? input.ReleaseMouse() : input.CaptureMouse();

    // Toggle debug GUI with tilde key
    if (input.IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) showDebug = !showDebug;

    // Update the voxel world
    scene.Update(delta);
}

void Editor::Render()
{
    // Render the scene to internal texture
    scene.Render();

    // Display the main editor gui
    static const auto windowFlags = ImGuiWindowFlags_MenuBar;
    ImGui::Begin("Toolbar", nullptr, windowFlags);

    // Main menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER " Load"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As"))
            {
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    // DEBUG: Testing different imgui methods
    static bool showDemo = false;
    ImGui::Checkbox("Show Demo Window", &showDemo);
    if (showDemo)
    {
        ImGui::ShowDemoWindow();
    }

    ImGui::End();

    Texture2D* sceneTex = scene.GetTexture();
    if (sceneTex)
    {
        sceneTex->BlitToScreen(256, 0);
    }
    
    if (showDebug) ShowDebug();
}