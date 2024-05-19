#pragma once

#include <any>
#include <string>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>
#include <phi/core/math/aggregate_volume.hpp>

// Forward declaration
class VoxelWorldEditor;

namespace Phi
{
    // Represents a procedural mass of voxels used for terrain generation
    class VoxelMass
    {
        // Interface
        public:

            // Material map type
            enum class MaterialType
            {
                SingleMaterial,
                // DensityMap,
                // LayeredMap
            };

            // Creates an empty voxel mass
            VoxelMass();
            ~VoxelMass();

            // Default copy constructor/assignment
            VoxelMass(const VoxelMass&) = default;
            VoxelMass& operator=(const VoxelMass&) = default;

            // Default move constructor/assignment
            VoxelMass(VoxelMass&& other) = default;
            VoxelMass& operator=(VoxelMass&& other) = default;

            // Accessors
            unsigned char GetLayer() const { return layer; }
            const std::string& GetName() const { return name; }
            AggregateVolume& GetVolume() { return volume; }
            const MaterialType& GetMaterialType() const { return materialType; }
            const std::string& GetMaterial() const { return materialName; }
            Noise& GetNoise() { return noise; }

            // Mutators
            void SetLayer(unsigned char layer) { this->layer = layer; }
            void SetName(const std::string& name) { this->name = name; }
            void SetMaterialType(const MaterialType& type) { materialType = type; }
            void SetMaterial(const std::string&  material) { materialName = material; }

        // Data / implementation
        private:

            // The layer used for world generation order (0 = bottom layer)
            unsigned char layer = 0;

            // Name
            std::string name{"New Mass"};

            // Type of material mapping
            MaterialType materialType{MaterialType::SingleMaterial};

            // Name of single material
            std::string materialName{"default"};

            // The bounding volume of the mass
            AggregateVolume volume;

            // The noise to describe the mass
            Noise noise;

            // Friend so the editor can access
            friend class ::VoxelWorldEditor;
    };
}