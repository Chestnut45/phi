#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <FastNoiseLite.h>

namespace Phi
{
    // Noise wrapper
    class Noise
    {
        // Interface
        public:

            Noise(uint32_t seed = 0);
            ~Noise();

            // Default copy constructor/assignment
            Noise(const Noise&) = default;
            Noise& operator=(const Noise&) = default;

            // Default move constructor/assignment
            Noise(Noise&& other) = default;
            Noise& operator=(Noise&& other) = default;

            // Accessors / mutators

            // Seed
            inline uint32_t GetSeed() const { return seed; }
            inline void SetSeed(uint32_t seed) { this->seed = seed; }

            // Frequency
            inline float GetFrequency() const { return noise.mFrequency; }
            inline void SetFrequency(float frequency) { noise.SetFrequency(frequency); }

            // TODO: Other noise parameters

            // Sampling

            // Samples the noise at given 2D location
            float Sample(float x, float y) const;

            // Samples the noise at given 3D location
            float Sample(float x, float y, float z) const;

            // GLM sampling helpers
            inline float Sample(const glm::vec2& pos) const { return Sample(pos.x, pos.y); }
            inline float Sample(const glm::vec3& pos) const { return Sample(pos.x, pos.y, pos.z); }
        
        // Data / implementation
        private:
            
            // Seed
            uint32_t seed;

            // Main instance
            FastNoiseLite noise;
    };
}