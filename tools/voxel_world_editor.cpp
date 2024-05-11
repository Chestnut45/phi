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

VoxelWorldEditor::VoxelWorldEditor() : App("Voxel Editor", 4, 6)
{   
    Log("Voxel Editor initialized");
}

VoxelWorldEditor::~VoxelWorldEditor()
{
    Log("Voxel Editor shutdown");
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

    // Update the voxel world
    world.Update(delta);
    
    // Display GUI windows
    if (showGUI)
    {
        ShowDebug();
        world.GetScene().ShowDebug();
        ShowInterface();
    }
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

    // Main controls
    ImGui::SeparatorText("Controls");

    // Regenerates the world's terrain using the current data
    if (ImGui::Button("Regenerate")) world.Regenerate();

    // Display all voxel volumes
    ImGui::SeparatorText("Voxel Volumes");
    if (ImGui::Button("Add")) world.AddVolume(VoxelVolume());

    bool keepVolume = true;
    auto& volumes = world.GetVolumes();
    for (int i = 0; i < volumes.size(); ++i)
    {
        // Grab a reference to the volume
        VoxelVolume& volume = volumes[i];

        // Push unique ID to allow duplicate names
        ImGui::PushID(&volume);

        if (ImGui::CollapsingHeader((volume.name + "###").c_str(), &keepVolume, ImGuiTreeNodeFlags_None))
        {
            // Name editor
            ImGui::InputText("Name", &volume.name);

            ImGui::Text("Shapes:");
            ImGui::Separator();

            // Add shape buttons
            if (ImGui::Button("Add Sphere")) volume.AddSphere(Sphere()); ImGui::SameLine();
            if (ImGui::Button("Add AABB")) { /* todo */ };

            // Display all shapes
            ImGui::Indent();
            bool keepShape = true;
            for (int j = 0; j < volume.shapes.size(); ++j)
            {
                // Grab a reference to the shape
                auto& shape = volume.shapes[j];

                // Push unique ID to allow duplicate names
                ImGui::PushID(&shape);

                // Determine name
                int type = 0;
                if (shape.type() == typeid(Sphere)) type = 0;
                if (shape.type() == typeid(AABB)) type = 1;
                static const char* names[] = {"Sphere ", "AABB "};
                // TODO: Other shapes...

                // Display shape data
                if (ImGui::CollapsingHeader((std::string(names[type]) + std::to_string(j) + "###").c_str(), &keepShape, ImGuiTreeNodeFlags_None))
                {
                    // TODO: This is probably slow, profile if necessary
                    if (type == 0)
                    {
                        // Grab sphere (may throw, is the typeid matching enough?)
                        Sphere& sphere = std::any_cast<Sphere&>(shape);
                        ImGui::DragFloat3("Position", &sphere.position.x);
                        ImGui::DragFloat("Radius", &sphere.radius, 1.0f, 0.0f, INT32_MAX);
                    }
                }

                // Remove shape if requested
                if (!keepShape)
                {
                    volume.shapes.erase(volume.shapes.begin() + j);
                    j--;
                    keepShape = true;
                }

                // Pop the ID
                ImGui::PopID();
            }
            ImGui::Unindent();

            // TODO: Different material map types
            ImGui::Text("Material Map:");
            ImGui::Separator();
        }

        // Delete volume if requested
        if (!keepVolume)
        {
            volumes.erase(volumes.begin() + i);
            i--;
            keepVolume = true;
        }
        
        // Pop the volume ID
        ImGui::PopID();
    }

    // TODO: Biomes

    // TODO: Features

    // TODO: Serialization (saving / loading all volume, biome, and feature data to / from .vmap files)

    ImGui::End();
}