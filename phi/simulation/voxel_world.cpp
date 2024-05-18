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

        // Add environment
        Environment& environment = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
        scene.SetActiveEnvironment(environment);

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

    void VoxelWorld::AddVoxelMass(const VoxelMass& volume)
    {
        voxelMasses.push_back(volume);
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

                    // Add to queue if within render distance
                    if (loadSphere.Intersects(chunkID))
                    {
                        chunksToLoad.push_back(chunkID);
                    }
                }
            }
        }

        // Unload all chunks that are outside of the new load sphere
        chunksToUnload.clear();
        for (const auto&[key, _] : loadedChunks)
        {
            if (!loadSphere.Intersects(key)) chunksToUnload.push_back(key);
        }
        for (const glm::ivec3& chunk : chunksToUnload)
        {
            loadedChunks.erase(chunk);
        }

        // DEBUG: Generate all chunks at once for now
        // TODO: Use queue system to generate over multiple frames
        // TODO: Stream from disk if already generated
        for (auto& chunkID : chunksToLoad)
        {
            if (loadedChunks.count(chunkID) == 0) GenerateChunk(chunkID);
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

        // Mesh data container
        std::vector<VoxelMesh::Vertex> voxelData;

        // Iterate all voxels in the chunk
        for (int z = 0; z < VoxelChunk::CHUNK_DIM; ++z)
        {
            for (int y = 0; y < VoxelChunk::CHUNK_DIM; ++y)
            {
                for (int x = 0; x < VoxelChunk::CHUNK_DIM; ++x)
                {
                    // TODO: Pre-generation steps?

                    // Get world-space position of this voxel
                    glm::vec3 position = glm::vec3(x, y, z) * (float)VoxelChunk::CHUNK_DIM;

                    // Check for intersection of each mass
                    for (auto& mass : voxelMasses)
                    {
                        if (mass.GetVolume().Intersects(position))
                        {
                            // This is so insanely slow, obviously
                            // TODO: Preprocess masses, gather material IDs before iteration
                            int mat = scene.GetMaterialID(mass.GetMaterial());
                            chunk.voxelGrid(x, y, z) = mat;
                            VoxelMesh::Vertex v;
                            v.x = chunkID.x + x;
                            v.y = chunkID.y + y;
                            v.z = chunkID.z + z;
                            v.material = mat;
                            voxelData.push_back(v);
                        }
                    }
                }
            }
        }

        // DEBUG: Regenerate mesh immediately
        // VoxelMesh* mesh = &scene.CreateNode()->AddComponent<VoxelMesh>();
        // mesh->Vertices() = voxelData;
    }
}