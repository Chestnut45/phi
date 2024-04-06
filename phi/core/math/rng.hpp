#pragma once

#include <cstdint>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Phi
{
    // Represents a seedable instance of a pseudo random number generator
    class RNG
    {
        // Interface
        public:

            RNG(uint32_t seed = 0);
            ~RNG();

            // Default copy constructor/assignment
            RNG(const RNG&) = default;
            RNG& operator=(const RNG&) = default;

            // Default move constructor/assignment
            RNG(RNG&& other) = default;
            RNG& operator=(RNG&& other) = default;

            // Seed management

            // Sets the seed of this RNG instance
            inline void SetSeed(uint32_t seed) { this->seed = seed; engine.seed(seed); };

            // Gets the seed of this RNG instance
            inline uint32_t GetSeed() const { return seed; }

            // Resets to the initial value for the current seed
            inline void Reseed() { engine.seed(seed); }

            // Basic RNG
            
            // Generates a uniformly distributed boolean
            inline bool FlipCoin() { return boolDist(engine); };

            // Dice roll methods
            // Returns a uniformly distributed integer in the range [1, N]
            inline int RollD4() { return d4Dist(engine); };
            inline int RollD6() { return d6Dist(engine); };
            inline int RollD8() { return d8Dist(engine); };
            inline int RollD10() { return d10Dist(engine); };
            inline int RollD12() { return d12Dist(engine); };
            inline int RollD20() { return d20Dist(engine); };
            inline int RollD100() { return d100Dist(engine); };

            // Vector generation

            // Returns a random floating point color vector with full opacity
            glm::vec4 RandomColorOpaque();

            // Returns a random floating point color vector with random opacity
            glm::vec4 RandomColorTransparent();

            // Returns a normalized 3D direction vector
            glm::vec3 RandomDirection();

            // Returns a random position within the minimum and maximum bounds given
            glm::vec3 RandomPosition(const glm::vec3& min, const glm::vec3& max);

            // Quaternion generation

            // Returns a random rotation quaternion
            glm::quat RandomRotation();

            // Custom generation

            // Generates a uniformly distributed float within the range [min, max]
            // NOTE: If max < min, min is always returned as a fail-safe
            float NextFloat(float min, float max);

            // Generates a uniformly distributed int within the range [min, max]
            // NOTE: If max < min, min is always returned as a fail-safe
            int NextInt(int min, int max);

            // TODO: Weighted distributions
        
        // Data / implementation
        private:
            
            // Seed
            uint32_t seed;

            // Random engine
            std::default_random_engine engine;

            // Builtin distributions
            std::uniform_int_distribution<uint8_t> boolDist{0, 1};
            std::uniform_real_distribution<float> floatDist{0.0f, 1.0f};
            std::uniform_real_distribution<float> floatNDist{-1.0f, 1.0f};
            std::uniform_int_distribution<int> d4Dist{1, 4};
            std::uniform_int_distribution<int> d6Dist{1, 6};
            std::uniform_int_distribution<int> d8Dist{1, 8};
            std::uniform_int_distribution<int> d10Dist{1, 10};
            std::uniform_int_distribution<int> d12Dist{1, 12};
            std::uniform_int_distribution<int> d20Dist{1, 20};
            std::uniform_int_distribution<int> d100Dist{1, 100};
    };
}