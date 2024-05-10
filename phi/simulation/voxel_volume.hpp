#pragma once

#include <any>
#include <string>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>
#include <phi/core/math/shapes.hpp>

// Forward declaration
class VoxelEditor;

namespace Phi
{
    // Represents a procedural volume of voxels of arbitrary shape and material
    // Used for voxel world generation
    // All coordinates and shapes are in volume-local coordinates
    // TODO: Generalize for use with voxel objects?
    class VoxelVolume
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

            // Creates an empty voxel volume
            VoxelVolume();
            ~VoxelVolume();

            // IDs
            const std::string& GetName() const { return name; }
            void SetName(const std::string& name) { this->name = name; }

            // Shapes

            // Adds a sphere shape to the list of shapes that bound the volume
            void AddSphere(const Sphere& sphere) { shapes.push_back(std::any(sphere)); }

            // Materials

            // Set / get the material mapping type
            const MaterialType& GetMaterialType() const { return materialType; }
            void SetMaterialType(const MaterialType& type) { materialType = type; }
            
            // Set / get the single material value
            const unsigned int& GetSingleMaterial() const { return materialID; }
            void SetSingleMaterial(unsigned int material) { materialID = material; }

        // Data / implementation
        private:

            // IDs
            std::string name{"New Volume"};

            // Lists of shapes that define the volume
            std::vector<std::any> shapes;

            // Material mapping
            MaterialType materialType{MaterialType::SingleMaterial};
            unsigned int materialID = 0;

            // Friend so the editor can access
            friend class ::VoxelEditor;
    };
}