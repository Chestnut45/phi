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

Editor::Editor() : App("New Project | Phi Editor", 4, 6)
{
    // Enable vsync by default
    if (!vsync) ToggleVsync();
    
    // Enable raw mouse if accepted
    input.EnableRawMouseMotion();

    // Initialize scene
    scene.SetRenderMode(Scene::RenderMode::Texture);

    // Materials
    scene.LoadMaterials("data://materials.yaml");

    // Main camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 0, 16});
    scene.SetActiveCamera(camera);

    // Add a point light to the camera
    camera.GetNode()->AddComponent<PointLight>();

    // Load default fire effect
    CPUParticleEffect& effect = scene.CreateNode3D()->AddComponent<CPUParticleEffect>("data://effects/fire.effect");

    // Add an environment to the scene
    Environment& env = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveEnvironment(env);

    // Create mesh
    BasicMesh& mesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    mesh.AddIcosphere(1.0f, 2);
    mesh.AddBox(1.0f, 2.0f, 1.0, glm::vec3(0, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(1, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(-1, -1, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(-0.25f, -2, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(0.25f, -2, 0));
    mesh.SetMaterial("sapphire");

    // Add eyes
    BasicMesh& eyesMesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(-0.5f, 0.25f, 0.5f));
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(0.5f, 0.25f, 0.5f));
    eyesMesh.SetMaterial("pearl");
    mesh.GetNode()->AddChild(eyesMesh.GetNode());

    // Add pupils to eyes (lol)
    BasicMesh& pupilsMesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(0.5f, 0.25f, 0.9f));
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(-0.5f, 0.25f, 0.9f));
    pupilsMesh.SetMaterial("obsidian");
    mesh.GetNode()->AddChild(pupilsMesh.GetNode());
}

Editor::~Editor()
{
    // TODO: Shutdown logic
}

void Editor::Update(float delta)
{
    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE)) input.IsMouseCaptured() ? input.ReleaseMouse() : input.CaptureMouse();

    // Toggle debug window with tilde key
    if (input.IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) showDebug = !showDebug;

    // Update the voxel world
    scene.Update(delta);
}

void Editor::Render()
{
    // DEBUG: Show ImGui Demo
    ImGui::ShowDemoWindow();

    // Main menu bar
    GUIMainMenuBar();

    // Editor Windows
    GUISceneHierarchy();
    GUIInspector();
    GUISceneCamera();
    GUIResources();
    GUIConsole();
    GUIPerformanceStats();

    // Show debug statistics
    if (showDebug) ShowDebug();
}

void Editor::GUIMainMenuBar()
{
    // Popup flags
    bool newProjectPopup = false;
    bool openProjectPopup = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER_PLUS " New"))
            {
                newProjectPopup = true;
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open"))
            {
                openProjectPopup = true;
            }

            if (ImGui::MenuItem(ICON_FA_GEAR " Properties (Unimplemented)"))
            {
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene"))
        {
            if (ImGui::MenuItem(ICON_FA_CIRCLE_PLUS " New"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FILE " Load"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As..."))
            {
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    if (newProjectPopup) ImGui::OpenPopup("New Project");
    if (openProjectPopup) ImGui::OpenPopup("Open Project");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Select a folder...");
        ImGui::Separator();
        if (ImGui::Button("Create Project", ImVec2(128, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(128, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Open Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Select a folder...");
        ImGui::Separator();
        if (ImGui::Button("Open Project", ImVec2(128, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(128, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void Editor::GUISceneHierarchy()
{
    ImGui::Begin("Scene Hierarchy");

    // Iterate all nodes in the scene
    // TODO: Ensure consistent order
    for (auto&&[_, node] : scene.Each<Node>())
    {
        // Only display the top level nodes
        if (node.GetParent() == nullptr)
        {
            // Selectable tree node
            // TODO: Display components here or in inspector?
            if (ImGui::TreeNodeEx((void*)0, 0, ICON_FA_CODE_COMMIT " %s", node.GetName().c_str()))
            {
                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
}

void Editor::GUIInspector()
{
    ImGui::Begin("Inspector");
    ImGui::End();
}

void Editor::GUISceneCamera()
{
    ImGui::Begin("Scene Camera");

    // Update resolution if necessary
    glm::ivec2 sceneRes = scene.GetResolution();
    auto availRes = ImGui::GetContentRegionAvail();
    if (availRes.x != sceneRes.x || availRes.y != sceneRes.y)
    {
        scene.SetResolution(availRes.x, availRes.y);
    }

    // Render scene and display it
    scene.Render();
    Texture2D* sceneTex = scene.GetTexture();
    if (sceneTex)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(sceneTex->GetID()), ImVec2(sceneTex->GetWidth(), sceneTex->GetHeight()), ImVec2(0, 1), ImVec2(1, 0));
    }
    
    ImGui::End();
}

void Editor::GUIResources()
{
    ImGui::Begin("Resources");
    ImGui::End();
}

void Editor::GUIConsole()
{
    ImGui::Begin("Console");
    ImGui::End();
}

void Editor::GUIPerformanceStats()
{
    ImGui::Begin("Performance Statistics");
    ImGui::End();
}