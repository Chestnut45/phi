#pragma once

#include <glm/glm.hpp>
#include <FastNoiseLite.h>

namespace Phi
{
    // Noise wrapper
    class Noise
    {
        // Interface
        public:

            Noise(int seed = 0);
            ~Noise();

            // Default copy constructor/assignment
            Noise(const Noise&) = default;
            Noise& operator=(const Noise&) = default;

            // Default move constructor/assignment
            Noise(Noise&& other) = default;
            Noise& operator=(Noise&& other) = default;

            // Accessors / mutators

            // Seed
            inline int GetSeed() const { return noise.mSeed; }
            inline void SetSeed(int seed) { noise.SetSeed(seed); }

            // Frequency
            inline float GetFrequency() const { return noise.mFrequency; }
            inline void SetFrequency(float frequency) { noise.SetFrequency(frequency); }

            // TODO: Other noise parameters

            // Sampling
            inline float Sample(float x, float y) const { return noise.GetNoise(x, y); }
            inline float Sample(float x, float y, float z) const { return noise.GetNoise(x, y, z); }

            // GLM sampling helpers
            inline float Sample(const glm::vec2& pos) const { return Sample(pos.x, pos.y); }
            inline float Sample(const glm::vec3& pos) const { return Sample(pos.x, pos.y, pos.z); }
        
        // Data / implementation
        private:

            // Main instance
            FastNoiseLite noise;
    };
}