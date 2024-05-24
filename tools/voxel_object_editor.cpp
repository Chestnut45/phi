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
    // Create the object
    object = &world.GetScene().CreateNode()->AddComponent<VoxelObject>();

    // DEBUG: Load default object
    object->Load("data://models/teapot.vobj");

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

    // Grab camera and cast ray from mouse position
    Camera* cam = world.GetScene().GetActiveCamera();
    Ray ray = cam->CastRay(input.GetMousePos().x, input.GetMousePos().y);

    // Determine intersection with object
    const AABB& aabb = object->GetAABB();
    glm::vec2 tNearFar = ray.Slabs(aabb);
    if (tNearFar.x < tNearFar.y)
    {
        // Calculate starting / ending positions for traversal
        glm::ivec3 xyz = glm::ceil(ray.origin + ray.direction * tNearFar.x);
        glm::ivec3 oob = glm::floor(ray.origin + ray.direction * tNearFar.y);

        // Calculate step directions
        glm::ivec3 step = glm::ivec3(glm::sign(ray.direction.x), glm::sign(ray.direction.y), glm::sign(ray.direction.z));

        // Calculate tMax and tDelta
        glm::vec3 tMax = (glm::vec3(xyz) - ray.origin) / ray.direction;
        glm::vec3 tDelta = glm::vec3(1.0f) / ray.direction;

        // Grid traversal (Amanatides & Woo)
        while (true)
        {
            if (tMax.x < tMax.y)
            {
                if (tMax.x < tMax.z)
                {
                    xyz.x += step.x;
                    if (xyz.x == oob.x) break;
                    tMax.x = tMax.x + tDelta.x;
                }
                else
                {
                    xyz.z += step.z;
                    if (xyz.z == oob.z) break;
                    tMax.z = tMax.z + tDelta.z;
                }
            }
            else
            {
                if (tMax.y < tMax.z)
                {
                    xyz.y += step.y;
                    if (xyz.y == oob.y) break;
                    tMax.y = tMax.y + tDelta.y;
                }
                else
                {
                    xyz.z += step.z;
                    if (xyz.z == oob.z) break;
                    tMax.z = tMax.z + tDelta.z;
                }
            }

            // Check for voxel at next position
            int voxel = object->GetVoxel(xyz.x, xyz.y, xyz.z);
            if (voxel != 0)
            {
                selectedPosition = xyz;
                break;
            }
        }
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