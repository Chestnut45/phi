#include "pbr_material_editor.hpp"

// File dialogues
#include <portable-file-dialogs.h>

PBRMaterialEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new PBRMaterialEditor();
    app->Run();
    delete app;
    return 0;
}

PBRMaterialEditor::PBRMaterialEditor() : App("PBR Material Editor", 4, 6)
{
    // Initialize scene

    // Setup camera
    Camera& camera = scene.CreateNode()->AddComponent<Camera>();
    camera.SetPosition(glm::vec3(0, 0, 16));
    scene.SetActiveCamera(camera);

    // Setup environment
    Environment& environment = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_turquoise");
    environment.StopTime();
    environment.SetTime(0.365f);
    environment.SetSunRotation(1.150f);
    scene.SetActiveEnvironment(environment);

    // Add a mesh
    BasicMesh& mesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    mesh.AddIcosphere(5.0f, 3);

    Log(name, " initialized");
}

PBRMaterialEditor::~PBRMaterialEditor()
{
    Log(name, " shutdown");
}

void PBRMaterialEditor::Update(float delta)
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

    // Update the scene
    scene.Update(delta);
    
    // Display GUI windows
    if (showGUI)
    {
        ShowDebug();
        scene.ShowDebug();
        ShowInterface();
    }
}

void PBRMaterialEditor::Render()
{
    scene.Render();
}

void PBRMaterialEditor::ShowInterface()
{
    ImGui::SetNextWindowPos(ImVec2(4, 4));
    ImGui::SetNextWindowSize(ImVec2(320, wHeight - 8));
    ImGui::Begin("PBR Material Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::SeparatorText("Material");

    // Edit material name
    ImGui::InputText("Name", &materialName);

    // Editable data
    PBRMaterial mat = scene.GetPBRMaterial(0);
    float colorData[4];
    colorData[0] = mat.color.r;
    colorData[1] = mat.color.g;
    colorData[2] = mat.color.b;
    colorData[3] = mat.color.a;
    float metallic = mat.metallic;
    float roughness = mat.roughness;

    // Edit material data
    ImGui::ColorEdit4("Color", colorData);
    ImGui::DragFloat("Metallic", &metallic, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Roughness", &roughness, 0.001f, 0.0f, 1.0f);

    // Update material data
    mat.color.r = colorData[0];
    mat.color.g = colorData[1];
    mat.color.b = colorData[2];
    mat.color.a = colorData[3];
    mat.metallic = metallic;
    mat.roughness = roughness;
    scene.RegisterMaterial("default", mat);

    ImGui::End();
}