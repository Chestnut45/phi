#include "voxel_object_editor.hpp"

VoxelObjectEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelObjectEditor();
    app->Run();
    delete app;
    return 0;
}

VoxelObjectEditor::VoxelObjectEditor() : App("Voxel Object Editor", 4, 6)
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

    // DEBUG: Environment
    Environment& env = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveEnvironment(env);

    // Brush mesh
    brushMesh = &scene.CreateNode()->AddComponent<VoxelMesh>();
    brushMesh->Vertices().push_back(VoxelMesh::Vertex());

    // Main object
    object = &scene.CreateNode()->AddComponent<VoxelObject>();
    object->Load("data://models/teapot.vobj");

    // DEBUG: Add some water and enable simulation
    const auto& aabb = object->GetAABB();
    int grass = scene.GetVoxelMaterialID("grass");
    int water = scene.GetVoxelMaterialID("water");
    Noise noise;
    noise.SetFrequency(0.1f);
    for (int y = aabb.max.y - 1; y >= aabb.min.y; --y)
    {
        for (int z = aabb.min.z; z < aabb.max.z; ++z)
        {
            for (int x = aabb.min.x; x < aabb.max.x; ++x)
            {
                Voxel v;
                v.position.x = x;
                v.position.y = y;
                v.position.z = z;
                if (y < aabb.min.y + 4)
                {
                    if (noise.Sample(x, y, z) < 0.0f) continue;
                    v.material = grass;
                }
                else if (y < aabb.max.y - 3)
                {
                    continue;
                }
                else
                {
                    v.material = water;
                }
                object->SetVoxel(v);
            }
        }
    }
    object->Enable(VoxelObject::Flags::SimulateFluids);
    ToggleVsync();

    // Default material
    selectedVoxel.material = water;
}

VoxelObjectEditor::~VoxelObjectEditor()
{
    // TODO: Shutdown logic
}

void VoxelObjectEditor::Update(float delta)
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
        scene.ShowDebug();
    }

    // Grab camera and mouse position
    Camera* cam = scene.GetActiveCamera();
    const glm::vec2& mousePos = input.GetMousePos();
    
    // Update selected position if we hit a solid voxel with the mouse
    Ray ray = cam->GenerateRay(mousePos.x, mousePos.y);
    VoxelObject::RaycastInfo result = object->Raycast(ray);
    if (result.firstHit != -1)
    {
        selectedVoxel.position = result.visitedVoxels[result.firstHit > 0 ? result.firstHit - 1 : 0];
    }

    if (input.IsLMBJustDown())
    {
        // Initiate a brush stroke
        currentEdits[selectedVoxel.position] = selectedVoxel;
    }
    else if (input.IsLMBHeld())
    {
        // Add voxel if not already added to current stroke
        if (currentEdits.count(selectedVoxel.position) == 0)
        {
            currentEdits[selectedVoxel.position] = selectedVoxel;
            VoxelMesh::Vertex v;
            v.x = selectedVoxel.position.x;
            v.y = selectedVoxel.position.y;
            v.z = selectedVoxel.position.z;
            v.material = selectedVoxel.material;
            brushMesh->Vertices().push_back(v);
        }
    }
    else if (input.IsLMBReleased())
    {
        // Flush brush stroke edits
        for (const auto& it : currentEdits)
        {
            object->SetVoxel(it.second);
        }
        object->UpdateMesh();
        currentEdits.clear();
        
        // Reset the brush mesh
        auto& verts = brushMesh->Vertices();
        verts.clear();
        VoxelMesh::Vertex v;
        v.x = selectedVoxel.position.x;
        v.y = selectedVoxel.position.y;
        v.z = selectedVoxel.position.z;
        v.material = selectedVoxel.material;
        verts.push_back(v);
    }
    else
    {
        // Update the brush mesh
        auto& v = brushMesh->Vertices()[0];
        v.x = selectedVoxel.position.x;
        v.y = selectedVoxel.position.y;
        v.z = selectedVoxel.position.z;
        v.material = selectedVoxel.material;
    }

    // Update the voxel world
    scene.Update(delta);
}

void VoxelObjectEditor::Render()
{
    scene.Render();
}