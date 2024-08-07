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

Editor::Editor() : App("New Project | Phi Editor", 1280, 720)
{
    // Enable vsync by default
    if (!vsync) ToggleVsync();
    
    // Enable raw mouse if accepted
    input.EnableRawMouseMotion();

    // Register all component types
    RegisterComponents();

    // Initialize scene
    scene.SetRenderMode(Scene::RenderMode::Texture);

    // Materials
    scene.LoadMaterials("data://materials.yaml");

    // Main camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 0, 16});
    scene.SetActiveCamera(camera);

    // Add an environment to the scene
    Environment& env = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveEnvironment(env);

    // Add a point light to the camera
    camera.GetNode()->AddComponent<PointLight>();

    // Load default fire effect
    CPUParticleEffect& effect = scene.CreateNode3D()->AddComponent<CPUParticleEffect>("data://effects/fire.effect");

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

void Editor::RegisterComponents()
{
    // Initialize component names map
    componentNames[entt::type_id<BasicMesh>().hash()] = "BasicMesh";
    componentNames[entt::type_id<BoundingSphere>().hash()] = "BoundingSphere";
    componentNames[entt::type_id<Camera>().hash()] = "Camera";
    componentNames[entt::type_id<CPUParticleEffect>().hash()] = "CPUParticleEffect";
    componentNames[entt::type_id<DirectionalLight>().hash()] = "DirectionalLight";
    componentNames[entt::type_id<Environment>().hash()] = "Environment";
    componentNames[entt::type_id<PointLight>().hash()] = "PointLight";
    componentNames[entt::type_id<Transform>().hash()] = "Transform";
    componentNames[entt::type_id<VoxelChunk>().hash()] = "VoxelChunk";
    componentNames[entt::type_id<VoxelMap>().hash()] = "VoxelMap";
    componentNames[entt::type_id<VoxelMesh>().hash()] = "VoxelMesh";
    componentNames[entt::type_id<VoxelObject>().hash()] = "VoxelObject";
    
    // Add one of each component type to the scene to ensure consistent ordering
    // TODO: A better method might be to have node member functions to generate bitsets of components...
    Node* n = scene.CreateNode();
    n->AddComponent<BasicMesh>();
    n->AddComponent<BoundingSphere>();
    n->AddComponent<Camera>();
    n->AddComponent<CPUParticleEffect>();
    n->AddComponent<DirectionalLight>();
    n->AddComponent<Environment>("", "");
    n->AddComponent<PointLight>();
    n->AddComponent<Transform>();
    n->AddComponent<VoxelChunk>();
    n->AddComponent<VoxelMap>();
    n->AddComponent<VoxelMesh>();
    n->AddComponent<VoxelObject>();
    n->Delete();
}

void Editor::GUIMainMenuBar()
{
    // Popup flags
    bool newProjectPopup = false;
    bool openProjectPopup = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_VIDEO " New Scene"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FILE_VIDEO " Load Scene"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save Scene As..."))
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
    // Displays the node as part of the scene hierarchy tree
    std::function<void(Node&)> DisplayNode = [&](Node& node)
    {
        // Build label and flags
        std::string label = std::string(ICON_FA_CIRCLE_NODES) + " " + node.GetName() + "###";
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        // Remove arrow from leaf nodes
        const auto& children = node.GetChildren();
        if (children.size() == 0) flags |= ImGuiTreeNodeFlags_Leaf;

        // Highlight selected node
        if (selectedNode == &node) flags |= ImGuiTreeNodeFlags_Selected;
        
        // Display tree node
        if (ImGui::TreeNodeEx(label.c_str(), flags))
        {
            // Mark as selected
            if (ImGui::IsItemClicked()) selectedNode = &node;

            // Render all child nodes as well
            for (Node* child : children)
            {
                DisplayNode(*child);
            }
            ImGui::TreePop();
        }
        else
        {
            // Mark as selected if not open
            if (ImGui::IsItemClicked()) selectedNode = &node;
        }
    };

    // Disable inputs when mouse is captured (scene is playing)
    ImGuiWindowFlags flags = input.IsMouseCaptured() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
    ImGui::Begin("Scene", nullptr, flags);

    // Iterate all nodes in the scene
    for (auto&&[_, node] : scene.Each<Node>())
    {
        // Only display the top level nodes
        if (node.GetParent() == nullptr)
        {
            DisplayNode(node);
        }
    }

    ImGui::End();
}

void Editor::GUIInspector()
{
    ImGui::Begin("Inspector");

    if (selectedNode)
    {
        // Push a unique ID to avoid sync issues
        ImGui::PushID(selectedNode);

        ImGui::SeparatorText("Node");

        // Name editor
        const std::string& name = selectedNode->GetName();
        std::string newName = name;
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
        ImGui::InputText("Name", &newName, flags);
        if (newName != name) selectedNode->SetName(newName);

        ImGui::SeparatorText("Components");

        // Iterate all component types in the scene
        for(auto&& current : scene.registry.storage())
        {
            // Check if this node has that component
            if(auto& storage = current.second; storage.contains(selectedNode->GetID()))
            {
                // Grab the component's type info
                entt::id_type id = current.first;
                const std::string& typeName = componentNames[id];

                // Display the component as long as it has a name
                if (typeName != "")
                {
                    ImGui::PushID(id);
                    if (ImGui::CollapsingHeader(typeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        BaseComponent* component = static_cast<BaseComponent*>(storage.value(selectedNode->GetID()));
                        component->InspectorGUI();
                    }
                    ImGui::PopID();
                }
            }
        }

        // Pop the selected node's ID off the stack
        ImGui::PopID();
    }
    
    ImGui::End();
}

void Editor::GUISceneCamera()
{
    ImGui::Begin("Camera");

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
    ImGui::Begin("Performance");
    ImGui::End();
}