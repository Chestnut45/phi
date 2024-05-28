#pragma once

#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <phi/core/math/aggregate_volume.hpp>
#include <phi/core/math/noise.hpp>
#include <phi/core/math/shapes.hpp>
#include <phi/scene/components/simulation/voxel_chunk.hpp>
#include <phi/scene/components/simulation/voxel_object.hpp>

// Forward declaration
class VoxelMapEditor;

namespace Phi
{
    // A component for loading / simulating voxel terrain and objects
    class VoxelMap : public BaseComponent
    {
        // Interface
        public:

            // Inner class representing a procedural mass of voxels used for terrain generation
            struct VoxelMass
            {
                // Material map type
                enum class MaterialType
                {
                    SingleMaterial,
                    // DensityMap,
                    // LayeredMap
                };
                
                unsigned char layer = 0;
                std::string name{"New Mass"};
                std::string materialName{"default"};
                MaterialType materialType{MaterialType::SingleMaterial};
                AggregateVolume volume;
                Noise noise;
            };

            // Creates an empty voxel map
            VoxelMap();
            ~VoxelMap();

            // Delete copy constructor/assignment
            VoxelMap(const VoxelMap&) = delete;
            VoxelMap& operator=(const VoxelMap&) = delete;

            // Delete move constructor/assignment
            VoxelMap(VoxelMap&& other) = delete;
            VoxelMap& operator=(VoxelMap&& other) = delete;

            // Generation

            // Adds a voxel mass to the map
            void AddVoxelMass(const VoxelMass& voxelMass);

            // Gets the list of voxel masses
            std::vector<VoxelMass>& GetVoxelMasses() { return voxelMasses; }

            // Simulation

            // Updates the voxel world with the given elapsed time in seconds
            void Update(float delta);

        // Data / implementation
        private:

            // Map data

            // The masses that make up the terrain
            std::vector<VoxelMass> voxelMasses;

            // TODO: Biomes, features, structures, etc.

            // Simulation data

            // Map of loaded chunks
            std::unordered_map<glm::ivec3, VoxelChunk*> loadedChunks;

            // Queues
            std::vector<glm::ivec3> chunksToLoad;
            std::vector<glm::ivec3> chunksToUnload;

            // Settings

            // Whether or not to update / load new chunks around the camera
            bool updateChunks = true;

            // The approximate radius (in VoxelChunks) to load around the active camera
            int renderDistance = 6;

            // DEBUG: Counters
            size_t voxelsRendered = 0;

            // Updates which chunks should be loaded / unloaded around the active camera
            void UpdateChunks();

            // Generates the given chunk and loads it into the world
            void GenerateChunk(const glm::ivec3& chunkID);

            // Unloads all currently loaded chunks
            void UnloadChunks();

            // Needed for editor to work
            friend class ::VoxelMapEditor;
    };
}