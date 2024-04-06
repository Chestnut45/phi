#include "noise.hpp"

namespace Phi
{
    Noise::Noise(uint32_t seed)
        : seed(seed), noise(seed)
    {

    }

    Noise::~Noise()
    {

    }

    float Noise::Sample(float x, float y) const
    {
        return noise.GetNoise(x, y);
    }

    float Noise::Sample(float x, float y, float z) const
    {
        return noise.GetNoise(x, y, z);
    }
}