#include "voxel_world.hpp"

#include <phi/scene/node.hpp>

namespace Phi
{
    VoxelWorld::VoxelWorld()
    {
        // DEBUG: Default test scene setup

        // Add a camera
        Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
        camera.SetPosition({0, 32, 96});
        scene.SetActiveCamera(camera);

        // Add a sky
        Sky& sky = camera.GetNode()->AddComponent<Sky>("data://textures/skybox_day", "data://textures/skybox_night_old");
        scene.SetActiveSkybox(sky);

        // Load test materials
        scene.LoadMaterials("data://materials.yaml");

        // Add the test voxel object and load the model
        VoxelObject* voxelObject = &scene.CreateNode3D()->AddComponent<VoxelObject>();
        voxelObject->Load("data://models/teapot.pvox");

        // TODO: Test chunk system
    }

    VoxelWorld::~VoxelWorld()
    {
    }

    void VoxelWorld::LoadMaterials(const std::string& path)
    {
        scene.LoadMaterials(path);
    }

    void VoxelWorld::AddVolume(const VoxelVolume& volume)
    {
        terrainVolumes.push_back(volume);
    }

    void VoxelWorld::Update(float delta)
    {
        scene.Update(delta);
    }

    void VoxelWorld::Render()
    {
        scene.Render();
    }
}