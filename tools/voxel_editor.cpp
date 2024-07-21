#include "voxel_editor.hpp"

VoxelEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new VoxelEditor();
    app->Run();
    delete app;
    return 0;
}

VoxelEditor::VoxelEditor() : App("Voxel Editor", 1280, 720)
{
    // Enable raw mouse if accepted
    input.EnableRawMouseMotion();

    // Initialize scene
    scene.SetRenderMode(Scene::RenderMode::Texture);

    // Materials
    scene.LoadMaterials("data://materials.yaml");

    // Main camera
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
    camera.SetPosition({0, 16, 128});
    scene.SetActiveCamera(camera);

    // Add a point light to the camera
    camera.GetNode()->AddComponent<PointLight>();

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
    // int grass = scene.GetVoxelMaterialID("grass");
    // int water = scene.GetVoxelMaterialID("water");
    // int lava = scene.GetVoxelMaterialID("lava");
    // const auto& aabb = object->GetAABB();
    // Noise noise;
    // noise.SetFrequency(0.032f);
    // for (int y = aabb.max.y - 1; y >= aabb.min.y; --y)
    // {
    //     for (int z = aabb.min.z; z < aabb.max.z; ++z)
    //     {
    //         for (int x = aabb.min.x; x < aabb.max.x; ++x)
    //         {
    //             Voxel v;
    //             v.x = x;
    //             v.y = y;
    //             v.z = z;
    //             if (y < aabb.max.y - 4)
    //             {
    //                 if (noise.Sample(x, y, z) < 0.0f)
    //                 {
    //                     continue;
    //                 }
    //                 else
    //                 {
    //                     continue;//v.material = grass;
    //                 }
    //             }
    //             else
    //             {
    //                 v.material = water;
    //             }
                
    //             object->SetVoxel(v.x, v.y, v.z, v.material);
    //         }
    //     }
    // }
    // object->UpdateMesh();

    // Testing different object configurations
    // object->Enable(VoxelObject::Flags::SimulateFluids);
    // object->Enable(VoxelObject::Flags::SimulateFire);

    // Default material
    selectedVoxel.material = scene.GetVoxelMaterialID("water");
}

VoxelEditor::~VoxelEditor()
{
    // TODO: Shutdown logic
}

void VoxelEditor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        scene.SetResolution(wWidth - toolBarWidth, wHeight);
        windowResized = false;
    }

    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE)) input.IsMouseCaptured() ? input.ReleaseMouse() : input.CaptureMouse();

    // Toggle debug GUI with tilde key
    if (input.IsKeyJustDown(GLFW_KEY_GRAVE_ACCENT)) showDebug = !showDebug;

    // TODO: Update editor window rectangle

    // Grab camera and mouse position
    Camera* cam = scene.GetActiveCamera();
    const glm::vec2& mousePos = input.GetMousePos();
    
    // Update selected position if we hit a solid voxel with the mouse
    Ray ray = cam->GenerateRay(mousePos.x - toolBarWidth, mousePos.y);
    VoxelObject::RaycastInfo result = object->Raycast(ray);
    if (result.firstHit != -1)
    {
        const Voxel& hitVoxel = result.visitedVoxels[result.firstHit > 0 ? result.firstHit - 1 : 0];
        selectedVoxel.x = hitVoxel.x;
        selectedVoxel.y = hitVoxel.y;
        selectedVoxel.z = hitVoxel.z;
    }

    glm::ivec3 selectedPosition = glm::ivec3(selectedVoxel.x, selectedVoxel.y, selectedVoxel.z);

    if (input.IsLMBJustDown())
    {
        // Initiate a brush stroke
        currentEdits[selectedPosition] = selectedVoxel;
    }
    else if (input.IsLMBHeld())
    {
        // Add voxel if not already added to current stroke
        if (currentEdits.count(selectedPosition) == 0)
        {
            currentEdits[selectedPosition] = selectedVoxel;
            VoxelMesh::Vertex v;
            v.x = selectedVoxel.x;
            v.y = selectedVoxel.y;
            v.z = selectedVoxel.z;
            v.material = selectedVoxel.material;
            brushMesh->Vertices().push_back(v);
        }
    }
    else if (input.IsLMBReleased())
    {
        // Flush brush stroke edits
        for (const auto& it : currentEdits)
        {
            const Voxel& v = it.second;
            object->SetVoxel(v.x, v.y, v.z, v.material);
        }
        currentEdits.clear();
        object->UpdateMesh();
        
        // Reset the brush mesh
        auto& verts = brushMesh->Vertices();
        verts.clear();
        VoxelMesh::Vertex v;
        v.x = selectedVoxel.x;
        v.y = selectedVoxel.y;
        v.z = selectedVoxel.z;
        v.material = selectedVoxel.material;
        verts.push_back(v);
    }
    else
    {
        // Update the brush mesh
        auto& v = brushMesh->Vertices()[0];
        v.x = selectedVoxel.x;
        v.y = selectedVoxel.y;
        v.z = selectedVoxel.z;
        v.material = selectedVoxel.material;
    }

    // Update the voxel world
    scene.Update(delta);
}

void VoxelEditor::Render()
{
    // Render the scene to internal texture
    scene.Render();

    // Display the main editor gui
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(toolBarWidth, wHeight));
    auto windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    ImGui::Begin("Toolbar", nullptr, windowFlags);

    // Main menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FOLDER " Load"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save"))
            {
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save As"))
            {
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }

    // Brush mode combo
    int iBrushMode = (int)brushMode;
    ImGui::Combo("Brush Mode", &iBrushMode,
                 ICON_FA_CUBE " Add\0"
                 ICON_FA_BRUSH " Paint\0"
                 ICON_FA_ERASER " Erase\0");
    brushMode = (BrushMode)iBrushMode;
    
    // DEBUG: Testing different imgui methods
    static bool showDemo = false;
    ImGui::Checkbox("Show Demo Window", &showDemo);
    if (showDemo)
    {
        ImGui::ShowDemoWindow();
    }

    // TODO: Material palette

    ImGui::End();

    Texture2D* sceneTex = scene.GetTexture();
    if (sceneTex)
    {
        sceneTex->BlitToScreen(toolBarWidth, 0);
    }
    
    if (showDebug)
    {
        ShowDebug();
        scene.ShowDebug(wWidth - 360, wHeight - 450, 360, 450);
    }
}