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

    // Create the object
    object = &world.GetScene().CreateNode()->AddComponent<VoxelObject>();

    // DEBUG: Load default object
    object->Load("data://models/dragon.vobj");

    // Create brush mesh
    brushMesh = &world.GetScene().CreateNode()->AddComponent<VoxelMesh>();

    // DEBUG: Single voxel brush for now
    brushMesh->Vertices().push_back(VoxelMesh::Vertex());
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
    }

    // Grab camera and mouse position
    Camera* cam = world.GetScene().GetActiveCamera();
    const glm::vec2& mousePos = input.GetMousePos();

    // Cast ray towards voxel object
    Ray ray = cam->GenerateRay(mousePos.x, mousePos.y);
    VoxelObject::RaycastInfo result = object->Raycast(ray);
    
    // Update selected position if we hit a solid voxel
    if (result.firstHit != -1)
    {
        selectedPosition = result.visitedVoxels[result.firstHit];
    }

    // Update brush mesh
    auto& brushVert = brushMesh->Vertices()[0];
    brushVert.x = selectedPosition.x;
    brushVert.y = selectedPosition.y;
    brushVert.z = selectedPosition.z;
    brushVert.material = selectedMaterial;

    // Update the voxel world
    world.Update(delta);
}

void VoxelObjectEditor::Render()
{
    world.Render();
}