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
    ImGui::Begin("Voxel World Editor");

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

    // Display all voxel volumes
    ImGui::SeparatorText("Volumes");
    if (ImGui::Button("Add")) world.AddVolume(VoxelVolume());

    bool keepVolume = true;
    auto& volumes = world.GetVolumes();
    for (int i = 0; i < volumes.size(); ++i)
    {
        // Grab a reference to the volume
        VoxelVolume& volume = volumes[i];

        // Push unique ID to allow for duplicate names
        ImGui::PushID(&volume);

        if (ImGui::CollapsingHeader((volume.name + "###").c_str(), &keepVolume, ImGuiTreeNodeFlags_None))
        {
            // TODO: Display all shapes
            for (auto& shape : volume.shapes)
            {
                // TODO: This is most likely slow. Profile if necessary.
                if (shape.type() == typeid(Sphere))
                {
                    
                }
            }
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

    // Regenerate the world's terrain
    ImGui::Button("Regenerate");

    ImGui::End();
}