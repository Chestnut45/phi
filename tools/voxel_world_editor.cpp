#include "voxel_world_editor.hpp"

// File dialogues
#include <portable-file-dialogs.h>

VoxelWorldEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelWorldEditor();
    app->Run();
    delete app;
    return 0;
}

VoxelWorldEditor::VoxelWorldEditor() : App("Voxel World Editor", 4, 6)
{
    Log(name, " initialized");
}

VoxelWorldEditor::~VoxelWorldEditor()
{
    Log(name, " shutdown");
}

void VoxelWorldEditor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        world.GetScene().SetResolution(wWidth, wHeight);
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
        world.GetScene().ShowDebug();
        ShowInterface();
    }

    // Update the voxel world
    // NOTE: Must be done after GUI since regen button can
    // delete chunks that are already queued for rendering
    world.Update(delta);
}

void VoxelWorldEditor::Render()
{
    world.Render();
}

void VoxelWorldEditor::ShowInterface()
{
    ImGui::SetNextWindowPos(ImVec2(4, 4));
    ImGui::SetNextWindowSize(ImVec2(320, wHeight - 8));
    ImGui::Begin("Voxel World Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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

    // Statistics
    ImGui::SeparatorText("Statistics");
    ImGui::Text("Chunks Loaded: %lu", world.loadedChunks.size());

    // Main controls
    ImGui::SeparatorText("Controls");

    // Regenerates the world's terrain using the current data
    if (ImGui::Button("Regenerate")) world.ReloadChunks();

    // Display all voxel masses
    ImGui::SeparatorText("Voxel Masses");
    if (ImGui::Button("Add")) world.AddVoxelMass(VoxelMass());

    bool keepMass = true;
    auto& voxelMasses = world.GetVoxelMasses();
    for (int i = 0; i < voxelMasses.size(); ++i)
    {
        // Grab a reference to the voxel mass
        VoxelMass& mass = voxelMasses[i];

        // Push unique ID to allow duplicate names
        ImGui::PushID(&mass);

        if (ImGui::CollapsingHeader((mass.name + "###").c_str(), &keepMass, ImGuiTreeNodeFlags_None))
        {
            // Name editor
            ImGui::InputText("Name", &mass.name);

            // Material type selections
            static const char* materialTypeNames[] = {"Single Material"};
            const char* materialTypeSelection = materialTypeNames[(int)mass.materialType];
            if (ImGui::BeginCombo("Material Type", materialTypeSelection))
            {
                for (int n = 0; n < IM_ARRAYSIZE(materialTypeNames); n++)
                {
                    bool is_selected = (materialTypeSelection == materialTypeNames[n]);
                    if (ImGui::Selectable(materialTypeNames[n], is_selected))
                    {
                        mass.materialType = (VoxelMass::MaterialType)n;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            switch ((int)mass.materialType)
            {
                case (int)VoxelMass::MaterialType::SingleMaterial:
                    
                    // Single material name editor
                    ImGui::InputText("Material", &mass.materialName);
                    
                    break;
            }

            ImGui::Text("Volume:");
            ImGui::Separator();

            // Access to the volume
            auto& volume = mass.GetVolume();
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

    ImGui::End();
}