#pragma once

#include <string>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>
#include <phi/core/math/shapes.hpp>

namespace Phi
{
    // Represents a procedural volume of voxels of arbitrary shape and material
    // Used for voxel world / object generation
    // All coordinates and shapes are in volume-local coordinates
    // NOTE: Should not contain worldgen-specific data like mapLayer, may also be used for object creation?
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

            // Shape lists

            // Access to list of spheres
            std::vector<Sphere>& GetSpheres() { return spheres; }

            // Materials

            // Set / get the material mapping type
            const MaterialType& GetMaterialType() const { return materialType; }
            void SetMaterialType(const MaterialType& type) { materialType = type; }
            
            // Set / get the single material value
            const uint& GetSingleMaterial() const { return materialID; }
            void SetSingleMaterial(uint material) { materialID = material; }

        // Data / implementation
        private:

            // Lists of shapes that define the volume
            std::vector<Sphere> spheres;

            // Material mapping
            MaterialType materialType{MaterialType::SingleMaterial};
            uint materialID = 0;
    };
}