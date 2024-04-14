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

    // Add a camera
    Camera& camera = scene.CreateNode()->AddComponent<Camera>();
    camera.SetPosition({0, 32, 128});
    camera.SetMode(Camera::Mode::Target);
    camera.LookAt(glm::vec3(0, 0, 0));
    scene.SetActiveCamera(camera);

    // Add a skybox
    Skybox& skybox = camera.GetNode()->AddComponent<Skybox>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveSkybox(skybox);

    // Add a directional light
    DirectionalLight& light = skybox.GetNode()->AddComponent<DirectionalLight>();
    light.SetColor(glm::vec4(1.0f));
    light.SetDirection(glm::normalize(glm::vec3(0.5f, -0.5f, 0.0f)));
    scene.AttachLight(light, Scene::LightSlot::SLOT_0);

    // Load test materials
    scene.LoadMaterials("data://materials.yaml");

    // Add the test voxel object and load the model
    voxelObject = &scene.CreateNode()->AddComponent<VoxelObject>();
    voxelObject->GetNode()->AddComponent<Transform>();
    voxelObject->Load("data://models/teapot.pvox");

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
        scene.SetResolution(wWidth, wHeight);
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
    if (rotateModel) voxelObject->GetNode()->GetComponent<Transform>()->RotateXYZDeg(0.0f, 45.0f * delta, 0.0f);

    // Update all nodes / components in the scene
    scene.Update(delta);
    
    // Display debug window
    ShowDebug();
    ShowInterface();
}

void VoxelEditor::Render()
{
    scene.Render();
}

void VoxelEditor::ShowInterface()
{
    ImGui::Begin("Voxel Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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
                    voxelObject->GetNode()->GetComponent<Transform>()->SetRotationXYZ(0, 0, 0);
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
                voxelObject->GetNode()->GetComponent<Transform>()->SetRotationXYZ(0, 0, 0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    // Statistics
    ImGui::Text("# Voxels: %d", voxelObject->voxelCount);
    
    // Controls
    ImGui::SeparatorText("Controls");
    ImGui::Checkbox("Ray Traced", &voxelObject->rayTraced);
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

    ImGui::End();
}