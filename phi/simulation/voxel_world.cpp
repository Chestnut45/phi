#include "voxel_world.hpp"

#include <phi/scene/node.hpp>
#include <phi/scene/components/lighting/point_light.hpp>

namespace Phi
{
    VoxelWorld::VoxelWorld()
    {
        // DEBUG: Default test scene setup

        // Add a camera
        Camera& camera = scene.CreateNode3D()->AddComponent<Camera>();
        camera.SetPosition({0, 16, 128});
        scene.SetActiveCamera(camera);

        // DEBUG: Add a point light to the camera
        camera.GetNode()->AddComponent<PointLight>();

        // Add a sky
        Sky& sky = camera.GetNode()->AddComponent<Sky>("data://textures/skybox_day", "data://textures/skybox_night_old");
        scene.SetActiveSky(sky);

        // Load test materials
        scene.LoadMaterials("data://materials.yaml");

        // Add the test voxel object and load the model
        testObj = &scene.CreateNode3D()->AddComponent<VoxelObject>();
        testObj->Load("data://models/dragon.pvox");
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
        // DEBUG: Spin ya boi
        // testObj->GetNode()->Get<Transform>()->RotateXYZDeg(0, delta * 45, 0);

        // Calculate the current chunk
        Camera* camera = scene.GetActiveCamera();
        glm::ivec3 currentChunk = camera->GetPosition() / (float)VoxelChunk::CHUNK_DIM;

        // Create a sphere to check against in chunk space (1 unit = 1 chunk)
        Sphere loadSphere = Sphere(currentChunk, renderDistance);

        // Check each chunk in an AABB around the camera
        // TODO: Could use LUT here for speedup (relative chunk IDs cached for each level of render distance)
        chunksToLoad.clear();
        for (int z = -renderDistance; z <= renderDistance; ++z)
        {
            for (int y = -renderDistance; y <= renderDistance; ++y)
            {
                for (int x = -renderDistance; x <= renderDistance; ++x)
                {
                    // Generate chunk ID
                    glm::ivec3 chunkID = glm::ivec3(x, y, z) + currentChunk;

                    // Add to queue if within render distance (and not already loaded)
                    if (loadedChunks.count(chunkID) == 0 && loadSphere.Intersects(chunkID)) chunksToLoad.push_back(chunkID);
                }
            }
        }

        // DEBUG: Generate all chunks at once for now
        // TODO: Use queue system to generate over multiple frames
        // TODO: Stream from disk if already generated
        for (auto& chunkID : chunksToLoad)
        {
            GenerateChunk(chunkID);
        }

        // Update the internal scene
        scene.Update(delta);
    }

    void VoxelWorld::ReloadChunks()
    {
        loadedChunks.clear();
    }

    void VoxelWorld::Render()
    {
        // Render the internal scene
        scene.Render();
    }

    void VoxelWorld::GenerateChunk(const glm::ivec3& chunkID)
    {
        // TODO: Much optimization needed, naive implementation for testing

        // Create the chunk
        VoxelChunk& chunk = loadedChunks[chunkID];

        // TODO: Build map of voxel mass shapes for testing

        // Iterate all voxels in the chunk
        for (int z = 0; z < VoxelChunk::CHUNK_DIM; ++z)
        {
            for (int y = 0; y < VoxelChunk::CHUNK_DIM; ++y)
            {
                for (int x = 0; x < VoxelChunk::CHUNK_DIM; ++x)
                {
                    // TODO: Pre-generation steps?
                }
            }
        }
    }
}