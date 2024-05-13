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

    // Add sky
    Sky& sky = camera.GetNode()->AddComponent<Sky>("data://textures/skybox_day", "data://textures/skybox_night_turquoise");
    sky.StopTime();
    sky.SetTime(0.365f);
    sky.SetSunRotation(1.150f);
    scene.SetActiveSky(sky);

    // Add a mesh
    BasicMesh& mesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    mesh.AddIcosphere(5.0f, 3);

    Log("PBR Material Editor initialized");
}

PBRMaterialEditor::~PBRMaterialEditor()
{
    Log("PBR Material Editor shutdown");
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
    PBRMaterial mat = scene.GetMaterial(0);
    glm::vec3 color = mat.color;
    float metallic = mat.metallic;
    float roughness = mat.roughness;

    // Edit material data
    ImGui::ColorEdit3("Color", &color.x);
    ImGui::DragFloat("Metallic", &metallic, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Roughness", &roughness, 0.001f, 0.0f, 1.0f);

    // Update material data
    if (color != mat.color || metallic != mat.metallic || roughness != mat.roughness)
    {
        mat.color = color;
        mat.metallic = metallic;
        mat.roughness = roughness;
        scene.RegisterMaterial("default", mat);
    }

    ImGui::End();
}