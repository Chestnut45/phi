#include "voxel_editor.hpp"

// File dialogues
#include <portable-file-dialogs.h>

VoxelEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelEditor();
    app->Run();
    delete app;
    return 0;
}

VoxelEditor::VoxelEditor() : App("Voxel Editor", 4, 6)
{   
    // Initialize the scene
    scene.SetRenderMode(Scene::RenderMode::CustomViewport);

    // Add a camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 32, 96});
    camera.SetMode(Camera::Mode::Target);
    camera.LookAt(glm::vec3(0, 0, 0));
    scene.SetActiveCamera(camera);
    camera.GetNode()->AddComponent<PointLight>();

    // Add a skybox
    Skybox& skybox = camera.GetNode()->AddComponent<Skybox>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveSkybox(skybox);

    // Load test materials
    scene.LoadMaterials("data://materials.yaml");

    // Add the test voxel object and load the model
    voxelObject = &scene.CreateNode3D()->AddComponent<VoxelObject>();
    voxelObject->Load("data://models/teapot.pvox");

    DirectionalLight& dl = skybox.GetNode()->AddComponent<DirectionalLight>();
    dl.SetDirection(glm::vec3(-0.5f, -0.5f, 0.5f));
    // dl.Activate(DirectionalLight::Slot::SLOT_0);

    // DEBUG: A bunch of models
    // for (int i = 0; i < 35; ++i)
    // {
    //     auto& v = scene.CreateNode3D()->AddComponent<VoxelObject>();
    //     v.Load("data://models/teapot.pvox");
    //     Transform* t = v.GetNode()->Get<Transform>();
    //     t->SetPosition(rng.RandomPosition(glm::vec3(-100), glm::vec3(100)));
    //     t->SetRotation(rng.RandomRotation());
    // }

    // Log
    Log("Voxel Editor initialized");
}

VoxelEditor::~VoxelEditor()
{
    Log("Voxel Editor shutdown");
}

void VoxelEditor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        scene.SetResolution(wWidth - 256, wHeight);
        scene.SetViewport(256, 0, wWidth - 256, wHeight);
        windowResized = false;
    }

    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        if (input.IsMouseCaptured())
        {
            input.ReleaseMouse();
        }
        else
        {
            input.CaptureMouse();
        }
    }
    
    // Rotate the voxel mesh
    if (rotateModel) voxelObject->GetNode()->Get<Transform>()->RotateXYZDeg(0.0f, 45.0f * delta, 0.0f);

    // Update all nodes / components in the scene
    scene.Update(delta);
    
    // Display debug windows
    ShowDebug();
    scene.ShowDebug();
    ShowInterface();
}

void VoxelEditor::Render()
{
    scene.Render();
}

void VoxelEditor::ShowInterface()
{
    ImGui::Begin("Voxel Editor", nullptr, ImGuiWindowFlags_MenuBar);

    // Main menu bar
    bool loadModelFlag = false;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Model (.pvox)"))
            {
                // Load a new model from disk
                auto modelFile = pfd::open_file("Load Voxel Model File", Phi::File::GetDataPath() + "models", {"Voxel Model Files (.pvox)", "*.pvox"}, pfd::opt::none);
                if (modelFile.result().size() > 0)
                {
                    // Grab the model path, and convert it to the proper format
                    auto modelPath = std::filesystem::path(modelFile.result()[0]).generic_string();

                    // Load the data into the current voxel object
                    voxelObject->Load(modelPath);

                    // Reset rotation
                    voxelObject->GetNode()->Get<Transform>()->SetRotationXYZ(0, 0, 0);
                }
                lastTime = glfwGetTime();   
            }
            if (ImGui::MenuItem("Save"))
            {
                // TODO
            }
            if (ImGui::MenuItem("Reset"))
            {
                // Reset voxel object
                voxelObject->Reset();

                // Reset rotation
                voxelObject->GetNode()->Get<Transform>()->SetRotationXYZ(0, 0, 0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    // Statistics
    ImGui::SeparatorText("Statistics");
    ImGui::Text("# Voxels: %ld", voxelObject->mesh->Vertices().size());
    ImGui::NewLine();

    // Settings
    ImGui::SeparatorText("Settings");
    ImGui::Checkbox("Rotate Model", &rotateModel);
    if (ImGui::Checkbox("Free Camera", &freeCamera))
    {
        Camera* c = scene.GetActiveCamera();
        if (freeCamera)
        {
            c->SetMode(Camera::Mode::FirstPerson);
        }
        else
        {
            c->SetMode(Camera::Mode::Target);
            c->LookAt(glm::vec3(0.0f));
        }
    }
    bool v = vsync;
    bool f = fullscreen;
    if (ImGui::Checkbox("Fullscreen", &f))
    {
        ToggleFullscreen();
        
    }
    if (ImGui::Checkbox("Vsync", &v)) ToggleVsync();


    ImGui::End();
}