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
        glm::vec3 start = ray.origin + ray.direction * tNearFar.x;
        glm::ivec3 xyz = glm::floor(start);
        glm::ivec3 oob = glm::floor(ray.origin + ray.direction * tNearFar.y);

        // Calculate step directions
        glm::ivec3 step = glm::ivec3(glm::sign(ray.direction.x), glm::sign(ray.direction.y), glm::sign(ray.direction.z));

        // Avoid infinite loop
        if (step == glm::ivec3(0)) { /* Return to caller */ };

        // Calculate tMax and tDelta
        glm::vec3 tMax = glm::vec3(
            (ray.direction.x > 0 ? glm::ceil(start.x) - start.x : start.x - glm::floor(start.x)) / glm::abs(ray.direction.x),
            (ray.direction.y > 0 ? glm::ceil(start.y) - start.y : start.y - glm::floor(start.y)) / glm::abs(ray.direction.y),
            (ray.direction.z > 0 ? glm::ceil(start.z) - start.z : start.z - glm::floor(start.z)) / glm::abs(ray.direction.z)
        );
        glm::vec3 tDelta = glm::vec3(step) / ray.direction;

        // Grid traversal (Amanatides & Woo)
        do
        {
            // Check for voxel at current position
            int voxel = object->GetVoxel(xyz.x, xyz.y, xyz.z);
            if (voxel != 0)
            {
                selectedPosition = xyz;
                break;
            }

            // Step to next voxel
            if (tMax.x < tMax.y)
            {
                if (tMax.x < tMax.z)
                {
                    xyz.x += step.x;
                    if (xyz.x == oob.x) break;
                    tMax.x += tDelta.x;
                }
                else
                {
                    xyz.z += step.z;
                    if (xyz.z == oob.z) break;
                    tMax.z += tDelta.z;
                }
            }
            else
            {
                if (tMax.y < tMax.z)
                {
                    xyz.y += step.y;
                    if (xyz.y == oob.y) break;
                    tMax.y += tDelta.y;
                }
                else
                {
                    xyz.z += step.z;
                    if (xyz.z == oob.z) break;
                    tMax.z += tDelta.z;
                }
            }
            
        } while (true);
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