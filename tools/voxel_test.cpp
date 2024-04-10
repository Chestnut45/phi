#include "voxel_test.hpp"

VoxelTest* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelTest();
    app->Run();
    delete app;
    return 0;
}

VoxelTest::VoxelTest() : App("Voxel Test", 4, 6)
{
    // Initialize mouse input
    input.EnableRawMouseMotion();
    input.CaptureMouse();
    
    // Initialize the scene

    // Load test materials
    scene.LoadMaterials("data://basic_materials.yaml");

    // Add a camera
    Phi::Camera& camera = scene.CreateNode()->AddComponent<Phi::Camera>();
    camera.SetPosition({0, 32, 64});
    scene.SetActiveCamera(camera);

    // Add a skybox
    Phi::Skybox& skybox = camera.GetNode()->AddComponent<Phi::Skybox>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveSkybox(skybox);

    // Add a directional light
    Phi::DirectionalLight& light = skybox.GetNode()->AddComponent<Phi::DirectionalLight>();

    // Configure the light
    light.SetColor(glm::vec4(1.0f));
    light.SetDirection(glm::normalize(glm::vec3(-0.5f, -0.5f, 0.5f)));
    light.SetAmbient(0.1f);

    // Attach it to the current scene
    scene.AttachLight(light, Phi::Scene::LightSlot::SLOT_0);

    // Add a test voxel mesh
    light.GetNode()->AddComponent<Phi::VoxelMesh>();

    // Give it a transform component
    voxelMeshTransform = &light.GetNode()->AddComponent<Phi::Transform>();

    // Log
    Phi::Log("Voxel Test initialized");
}

VoxelTest::~VoxelTest()
{
    Phi::Log("Voxel Test shutdown");
}

void VoxelTest::Update(float delta)
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
    
    // Rotate and bob the voxel mesh
    voxelMeshTransform->SetPositionXYZ(0, sin(programLifetime), 0);
    voxelMeshTransform->RotateXYZDeg(0.0f, 45.0f * delta, 0.0f);

    // Update all nodes / components in the scene
    scene.Update(delta);
    
    // Display debug window
    ShowDebug();
}

void VoxelTest::Render()
{
    scene.Render();
}