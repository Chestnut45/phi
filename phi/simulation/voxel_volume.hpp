#pragma once

#include <string>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>

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

            // Creates an empty voxel volume
            VoxelVolume();
            ~VoxelVolume();

        // Data / implementation
        private:

            // TODO: List of shapes that describe the volume (noise?)

            // TODO: Material mapping
    };
}