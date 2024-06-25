#include "voxel_map_editor.hpp"

// File dialogues
#include <portable-file-dialogs.h>

VoxelMapEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelMapEditor();
    app->Run();
    delete app;
    return 0;
}

VoxelMapEditor::VoxelMapEditor() : App("Voxel Map Editor", 4, 6)
{
    // Enable raw mouse if accepted
    input.EnableRawMouseMotion();

    // Initialize scene

    // Materials
    scene.LoadMaterials("data://materials.yaml");

    // Main camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 16, 128});
    scene.SetActiveCamera(camera);

    // Environment
    Environment& env = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveEnvironment(env);

    // Voxel map
    VoxelMap& map = scene.CreateNode()->AddComponent<VoxelMap>();
    scene.SetActiveVoxelMap(map);

    Log(name, " initialized");
}

VoxelMapEditor::~VoxelMapEditor()
{
    Log(name, " shutdown");
}

void VoxelMapEditor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        scene.SetResolution(wWidth, wHeight);
        windowResized = false;
    }

    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE)) input.IsMouseCaptured() ? input.ReleaseMouse() : input.CaptureMouse();

    // Toggle debug GUI with tilde key
    if (input.IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) showGUI = !showGUI;
    
    // Display GUI windows
    if (showGUI)
    {
        ShowDebug();
        scene.ShowDebug(wWidth - 360, wHeight - 450, 360, 450);
        ShowInterface();
    }

    scene.Update(delta);
}

void VoxelMapEditor::Render()
{
    scene.Render();
}

void VoxelMapEditor::ShowInterface()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(320, wHeight));
    ImGui::Begin("Voxel Map Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Main menu bar
    // bool loadModelFlag = false;
    // if (ImGui::BeginMenuBar())
    // {
    //     if (ImGui::BeginMenu("File"))
    //     {
    //         if (ImGui::MenuItem("Load Model (.pvox)"))
    //         {
    //             // Load a new model from disk
    //             auto modelFile = pfd::open_file("Load Voxel Model File", Phi::File::GetDataPath() + "models", {"Voxel Model Files (.pvox)", "*.pvox"}, pfd::opt::none);
    //             if (modelFile.result().size() > 0)
    //             {
    //                 // Grab the model path, and convert it to the proper format
    //                 auto modelPath = std::filesystem::path(modelFile.result()[0]).generic_string();

    //                 // Load the data into the current voxel object
    //                 // voxelObject->Load(modelPath);

    //                 // Reset rotation
    //                 // voxelObject->GetNode()->Get<Transform>()->SetRotationXYZ(0, 0, 0);
    //             }
    //             lastTime = glfwGetTime();   
    //         }
    //         if (ImGui::MenuItem("Save"))
    //         {
    //             // TODO
    //         }
    //         if (ImGui::MenuItem("Reset"))
    //         {
    //             // Reset voxel object
    //             // voxelObject->Reset();

    //             // Reset rotation
    //             // voxelObject->GetNode()->Get<Transform>()->SetRotationXYZ(0, 0, 0);
    //         }
    //         ImGui::EndMenu();
    //     }
    //     ImGui::EndMenuBar();
    // }

    // Grab active voxel map
    VoxelMap* map = scene.GetActiveVoxelMap();

    if (map)
    {
        // Statistics
        ImGui::SeparatorText("Statistics");
        ImGui::Text("Chunks Loaded: %lu", map->loadedChunks.size());
        ImGui::Text("Voxels Rendered: %lu", map->voxelsRendered);

        // Main controls
        ImGui::SeparatorText("Controls");

        // Regenerates the map's terrain using the current data
        if (ImGui::Button("Regenerate")) map->UnloadChunks();

        // Display all voxel masses
        ImGui::SeparatorText("Voxel Masses");
        if (ImGui::Button("Add")) map->AddVoxelMass(VoxelMap::VoxelMass());

        bool keepMass = true;
        auto& voxelMasses = map->GetVoxelMasses();
        for (int i = 0; i < voxelMasses.size(); ++i)
        {
            // Grab a reference to the voxel mass
            VoxelMap::VoxelMass& mass = voxelMasses[i];

            // Push unique ID to allow duplicate names
            ImGui::PushID(&mass);

            if (ImGui::CollapsingHeader((mass.name + "###").c_str(), &keepMass, ImGuiTreeNodeFlags_None))
            {
                // Name editor
                ImGui::InputText("Name", &mass.name);

                // Noise editor
                ImGui::Text("Noise:");
                ImGui::Separator();

                float frequency = mass.noise.GetFrequency();
                if (ImGui::DragFloat("Frequency", &frequency, 0.001f, 0.0f, 1.0f)) mass.noise.SetFrequency(frequency);

                // Material type selections
                ImGui::Text("Materials:");
                ImGui::Separator();
                static const char* materialTypeNames[] = {"Single Material"};
                const char* materialTypeSelection = materialTypeNames[(int)mass.materialType];
                if (ImGui::BeginCombo("Mapping", materialTypeSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(materialTypeNames); n++)
                    {
                        bool is_selected = (materialTypeSelection == materialTypeNames[n]);
                        if (ImGui::Selectable(materialTypeNames[n], is_selected))
                        {
                            mass.materialType = (VoxelMap::VoxelMass::MaterialType)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                switch ((int)mass.materialType)
                {
                    case (int)VoxelMap::VoxelMass::MaterialType::SingleMaterial:
                        
                        // Single material name editor
                        ImGui::InputText("Material", &mass.materialName);
                        
                        break;
                }

                ImGui::Text("Volume:");
                ImGui::Separator();

                // Access to the volume
                auto& volume = mass.volume;
                auto& spheres = volume.GetSpheres();
                auto& aabbs = volume.GetAABBs();

                // Add shape buttons
                if (ImGui::Button("Add Sphere")) volume.AddSphere(Sphere()); ImGui::SameLine();
                if (ImGui::Button("Add AABB")) volume.AddAABB(AABB());

                // Display all shapes
                bool keepShape = true;
                for (int j = 0; j < spheres.size(); ++j)
                {
                    Sphere& sphere = spheres[j];

                    ImGui::PushID(&sphere);
                    if (ImGui::CollapsingHeader(("Sphere " + std::to_string(j) + "###").c_str(), &keepShape, ImGuiTreeNodeFlags_None))
                    {
                        ImGui::DragFloat3("Position", &sphere.position.x);
                        ImGui::DragFloat("Radius", &sphere.radius, 1.0f, 0.0f, INT32_MAX);
                    }
                    ImGui::PopID();

                    if (!keepShape)
                    {
                        spheres.erase(spheres.begin() + j);
                        j--;
                        keepShape = true;
                    }
                }

                for (int j = 0; j < aabbs.size(); ++j)
                {
                    AABB& aabb = aabbs[j];

                    ImGui::PushID(&aabb);
                    if (ImGui::CollapsingHeader(("AABB " + std::to_string(j) + "###").c_str(), &keepShape, ImGuiTreeNodeFlags_None))
                    {
                        ImGui::DragFloat3("Min", &aabb.min.x, 0.1f);
                        ImGui::DragFloat3("Max", &aabb.max.x, 0.1f);
                    }
                    ImGui::PopID();

                    if (!keepShape)
                    {
                        aabbs.erase(aabbs.begin() + j);
                        j--;
                        keepShape = true;
                    }
                }
            }

            // Delete mass if requested
            if (!keepMass)
            {
                voxelMasses.erase(voxelMasses.begin() + i);
                i--;
                keepMass = true;
            }
            
            // Pop the mass ID
            ImGui::PopID();
        }

        // TODO: Biomes

        // TODO: Features

        // TODO: Serialization (saving / loading all volume, biome, and feature data to / from .vmap files)
    }

    ImGui::End();
}