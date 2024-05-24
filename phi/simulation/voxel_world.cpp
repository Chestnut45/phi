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

        // Add environment
        Environment& environment = camera.GetNode()->AddComponent<Environment>("data://textures/skybox_day", "data://textures/skybox_night_old");
        scene.SetActiveEnvironment(environment);

        // Load test materials
        scene.LoadMaterials("data://materials.yaml");

        // DEBUG: Test model for benchmarking purposes
        scene.CreateNode()->AddComponent<VoxelObject>().Load("data://models/dragon.vobj");
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
                    if (loadSphere.Intersects(chunkID) && loadedChunks.count(chunkID) == 0)
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
        for (const glm::ivec3& chunkID : chunksToUnload)
        {
            VoxelChunk* chunk = loadedChunks[chunkID];
            const auto mesh = chunk->GetNode()->Get<VoxelMesh>();
            if (mesh)
            {
                voxelsRendered -= mesh->Vertices().size();
            }
            chunk->GetNode()->Delete();
            loadedChunks.erase(chunkID);
        }

        // DEBUG: Generate one chunk per frame for now
        // TODO: Use queue system to generate over multiple frames
        // TODO: Stream from disk if already generated
        if (chunksToLoad.size() > 0) GenerateChunk(chunksToLoad[0]);

        // Update the internal scene
        scene.Update(delta);
    }

    void VoxelWorld::ReloadChunks()
    {
        // Unload all chunks
        chunksToUnload.clear();
        for (const auto&[key, _] : loadedChunks)
        {
            VoxelChunk* chunk = loadedChunks[key];
            const auto mesh = chunk->GetNode()->Get<VoxelMesh>();
            if (mesh)
            {
                voxelsRendered -= mesh->Vertices().size();
            }
            chunk->GetNode()->Delete();
        }
        loadedChunks.clear();
        chunksToUnload.clear();
    }

    void VoxelWorld::Render()
    {
        // Render the internal scene
        scene.Render();
    }

    void VoxelWorld::GenerateChunk(const glm::ivec3& chunkID)
    {
        // TODO: Much optimization needed, naive implementation for testing

        // TODO: Unload chunk if it currently exists

        // Create the chunk
        VoxelChunk*& chunk = loadedChunks[chunkID];
        chunk = &scene.CreateNode()->AddComponent<VoxelChunk>();

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
                    glm::vec3 position = glm::vec3(x, y, z) + glm::vec3(chunkID * VoxelChunk::CHUNK_DIM);

                    // Check for intersection of each mass
                    for (auto& mass : voxelMasses)
                    {
                        if (mass.GetVolume().Intersects(position) && mass.GetNoise().Sample(position) > 0.0f)
                        {
                            // This is so insanely slow, obviously
                            // TODO: Preprocess masses, gather material IDs before iteration
                            chunk->voxelGrid(x, y, z) = scene.GetMaterialID(mass.GetMaterial());
                        }
                    }
                }
            }
        }

        // Add only visible voxels to mesh
        for (int z = 0; z < VoxelChunk::CHUNK_DIM; ++z)
        {
            for (int y = 0; y < VoxelChunk::CHUNK_DIM; ++y)
            {
                for (int x = 0; x < VoxelChunk::CHUNK_DIM; ++x)
                {
                    const auto& v = chunk->voxelGrid(x, y, z);
                    if (v == 0) continue;

                    // Get world-space position of this voxel
                    glm::vec3 position = glm::vec3(x, y, z) + glm::vec3(chunkID * VoxelChunk::CHUNK_DIM);

                    if (x == 0 || y == 0 || z == 0 || x == VoxelChunk::CHUNK_DIM - 1 || y == VoxelChunk::CHUNK_DIM - 1 || z == VoxelChunk::CHUNK_DIM - 1)
                    {
                        VertexVoxelHalfPrecision vert;
                        vert.x = position.x;
                        vert.y = position.y;
                        vert.z = position.z;
                        vert.material = v;
                        voxelData.push_back(vert);

                        continue;
                    }
                    
                    if (chunk->voxelGrid(x - 1, y, z) == 0 ||
                        chunk->voxelGrid(x + 1, y, z) == 0 ||
                        chunk->voxelGrid(x, y - 1, z) == 0 ||
                        chunk->voxelGrid(x, y + 1, z) == 0 ||
                        chunk->voxelGrid(x, y, z - 1) == 0 ||
                        chunk->voxelGrid(x, y, z + 1) == 0)
                    {
                        VertexVoxelHalfPrecision vert;
                        vert.x = position.x;
                        vert.y = position.y;
                        vert.z = position.z;
                        vert.material = v;
                        voxelData.push_back(vert);
                    }
                }
            }
        }

        if (voxelData.size() > 0)
        {
            // DEBUG: Regenerate mesh immediately
            VoxelMesh* mesh = chunk->GetNode()->Get<VoxelMesh>();
            if (!mesh) mesh = &chunk->GetNode()->AddComponent<VoxelMesh>();
            mesh->Vertices() = voxelData;
            voxelsRendered += voxelData.size();
        }
    }
}