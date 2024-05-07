#pragma once

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>

namespace Phi
{
    // Represents a single instance of a landmass for use in generating a voxel world
    class Landmass
    {
        // Interface
        public:

            Landmass();
            ~Landmass();

        // Data / implementation
        private:

            Noise noise;
    };
}