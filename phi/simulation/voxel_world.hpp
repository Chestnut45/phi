#pragma once

#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <phi/simulation/voxel_chunk.hpp>
#include <phi/simulation/voxel_mass.hpp>
#include <phi/simulation/voxel_object.hpp>
#include <phi/scene/scene.hpp>

// Forward declaration
class VoxelWorldEditor;

namespace Phi
{
    // Represents a voxel world and all of its nodes / components
    // Provides an interface to the internal scene instance
    class VoxelWorld
    {
        // Interface
        public:

            // Creates an empty voxel world with default settings
            VoxelWorld();
            ~VoxelWorld();

            // Delete copy constructor/assignment
            VoxelWorld(const VoxelWorld&) = delete;
            VoxelWorld& operator=(const VoxelWorld&) = delete;

            // Delete move constructor/assignment
            VoxelWorld(VoxelWorld&& other) = delete;
            VoxelWorld& operator=(VoxelWorld&& other) = delete;

            // Generation

            // Loads voxel materials from a YAML file
            // TODO: Should only accept voxel materials!
            void LoadMaterials(const std::string& path);

            // Adds a voxel mass to the world
            void AddVoxelMass(const VoxelMass& voxelMass);

            // Gets the list of voxel masses
            std::vector<VoxelMass>& GetVoxelMasses() { return voxelMasses; }

            // Simulation

            // Updates the voxel world with the given elapsed time in seconds
            void Update(float delta);

            // Reloads all currently loaded chunks
            void ReloadChunks();

            // Rendering

            // Renders the voxel world to the current framebuffer
            void Render();

            // Full access to the voxel world's internal scene instance
            // TODO: Restrict more? How much access is actually required by the user at this level?
            Scene& GetScene() { return scene; };

        // Data / implementation
        private:

            // Internal scene instance
            Scene scene;

            // DEBUG: Spinning object
            VoxelObject* testObj = nullptr;

            // Map of loaded chunks
            // TODO: Switch to Phi::HashGrid3D when impl finished (Profile!)
            std::unordered_map<glm::ivec3, VoxelChunk> loadedChunks;

            // Queues
            std::vector<glm::ivec3> chunksToLoad;
            std::vector<glm::ivec3> chunksToUnload;

            // Generation

            // List of all voxel masses in the world
            std::vector<VoxelMass> voxelMasses;

            // TODO: Biomes, features...

            // Settings

            // The approximate radius (in VoxelChunks) to load around the active camera
            int renderDistance = 5;

            // Helper functions

            // Generates the given chunk and loads it into the world
            void GenerateChunk(const glm::ivec3& chunkID);

            // Needed for editor to work
            friend class ::VoxelWorldEditor;
    };
}