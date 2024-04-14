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
    // Initialize the scene

    // Add a camera
    Camera& camera = scene.CreateNode()->AddComponent<Camera>();
    camera.SetPosition({0, 32, 128});
    // camera.SetMode(Camera::Mode::Target);
    // camera.LookAt(glm::vec3(0, 0, 0));
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
    VoxelObject& voxelObject = scene.CreateNode()->AddComponent<VoxelObject>();
    voxelObject.Load("data://models/teapot.pvox");

    // Give it a transform component
    voxelObjectTransform = &(voxelObject.GetNode()->AddComponent<Transform>());

    // Add basic mesh for visual debugging
    Phi::BasicMesh& mesh = voxelObject.GetNode()->AddComponent<Phi::BasicMesh>();
    mesh.AddIcosphere(1.0f, 2);
    mesh.AddBox(1.0f, 2.0f, 1.0, glm::vec3(0, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(1, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(-1, -1, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(-0.25f, -2, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(0.25f, -2, 0));
    mesh.SetMaterial("sapphire");

    // Add eyes
    Phi::BasicMesh& eyesMesh = scene.CreateNode()->AddComponent<Phi::BasicMesh>();
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(-0.5f, 0.25f, 0.5f));
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(0.5f, 0.25f, 0.5f));
    eyesMesh.SetMaterial("pearl");
    eyesMesh.GetNode()->AddComponent<Phi::Transform>();
    mesh.GetNode()->AddChild(eyesMesh.GetNode());

    // Add pupils to eyes (lol)
    Phi::BasicMesh& pupilsMesh = scene.CreateNode()->AddComponent<Phi::BasicMesh>();
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(0.5f, 0.25f, 0.9f));
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(-0.5f, 0.25f, 0.9f));
    pupilsMesh.SetMaterial("obsidian");
    pupilsMesh.GetNode()->AddComponent<Phi::Transform>();
    mesh.GetNode()->AddChild(pupilsMesh.GetNode());

    // Log
    Log("Voxel Test initialized");
}

VoxelTest::~VoxelTest()
{
    Log("Voxel Test shutdown");
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
    voxelObjectTransform->SetPositionXYZ(0, sin(programLifetime), 0);
    voxelObjectTransform->RotateXYZDeg(0.0f, 45.0f * delta, 0.0f);

    // Update all nodes / components in the scene
    scene.Update(delta);
    
    // Display debug window
    ShowDebug();
}

void VoxelTest::Render()
{
    scene.Render();
}