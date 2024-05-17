#pragma once

#include <any>
#include <string>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>
#include <phi/core/math/shapes.hpp>

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

            // Accessors
            unsigned char GetLayer() const { return layer; }
            const std::string& GetName() const { return name; }
            const MaterialType& GetMaterialType() const { return materialType; }
            const std::string& GetMaterial() const { return materialName; }

            // Mutators
            void SetLayer(unsigned char layer) { this->layer = layer; }
            void SetName(const std::string& name) { this->name = name; }
            void SetMaterialType(const MaterialType& type) { materialType = type; }
            void SetMaterial(const std::string&  material) { materialName = material; }

            // Shapes

            // Adds a sphere shape to the list of shapes that bound the volume
            void AddSphere(const Sphere& sphere) { shapes.push_back(std::any(sphere)); }

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

            // Lists of shapes that define the volume
            // TODO: AggregateVolume class
            std::vector<std::any> shapes;

            // Friend so the editor can access
            friend class ::VoxelWorldEditor;
    };
}