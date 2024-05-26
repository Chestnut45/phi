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
    object->Load("data://models/teapot.vobj");

    // Create meshes
    brushMesh = &world.GetScene().CreateNode()->AddComponent<VoxelMesh>();

    // DEBUG: Single voxel brush for now
    brushMesh->Vertices().push_back(VoxelMesh::Vertex());

    // Default material
    selectedMaterial = world.GetScene().GetMaterialID("gold");
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

    if (input.IsLMBJustDown())
    {
        // Initiate a brush stroke
        currentEdits.insert(selectedPosition);
    }
    else if (input.IsLMBHeld())
    {
        // Add voxel if not already added to current stroke
        if (currentEdits.count(selectedPosition) == 0)
        {
            currentEdits.insert(selectedPosition);
            VoxelMesh::Vertex v;
            v.x = selectedPosition.x;
            v.y = selectedPosition.y;
            v.z = selectedPosition.z;
            v.material = selectedMaterial;
            brushMesh->Vertices().push_back(v);
        }
    }
    else if (input.IsLMBReleased())
    {
        // Flush brush stroke edits
        for (const auto& pos : currentEdits)
        {
            object->SetVoxel(pos.x, pos.y, pos.z, selectedMaterial);
        }
        object->UpdateMesh();
        currentEdits.clear();
        
        // Reset the brush mesh
        auto& verts = brushMesh->Vertices();
        verts.clear();
        VoxelMesh::Vertex v;
        v.x = selectedPosition.x;
        v.y = selectedPosition.y;
        v.z = selectedPosition.z;
        v.material = selectedMaterial;
        verts.push_back(v);
    }
    else
    {
        // Update the brush mesh
        auto& v = brushMesh->Vertices()[0];
        v.x = selectedPosition.x;
        v.y = selectedPosition.y;
        v.z = selectedPosition.z;
        v.material = selectedMaterial;
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
        selectedPosition = result.visitedVoxels[result.firstHit > 0 ? result.firstHit - 1 : 0];
    }

    // Update the voxel world
    world.Update(delta);
}

void VoxelObjectEditor::Render()
{
    world.Render();
}