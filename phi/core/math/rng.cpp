#include "rng.hpp"

namespace Phi
{
    RNG::RNG(uint32_t seed)
        : seed(seed), engine(seed)
    {
    }

    RNG::~RNG()
    {
    }

    glm::vec4 RNG::RandomColorOpaque()
    {
        return std::move(glm::vec4(floatDist(engine), floatDist(engine), floatDist(engine), 1.0f));
    }

    glm::vec4 RNG::RandomColorTransparent()
    {
        return std::move(glm::vec4(floatDist(engine), floatDist(engine), floatDist(engine), floatDist(engine)));
    }

    glm::vec3 RNG::RandomDirection()
    {
        // Generate direction
        glm::vec3 dir(floatNDist(engine), floatNDist(engine), floatNDist(engine));

        // Protect against divide by zero
        if (glm::length(dir) < 0.0001f)
            return std::move(glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Normalize before returning
        return std::move(glm::normalize(dir));
    }

    glm::vec3 RNG::RandomPosition(const glm::vec3& min, const glm::vec3& max)
    {
        return std::move(glm::vec3(NextFloat(min.x, max.x), NextFloat(min.y, max.y), NextFloat(min.z, max.z)));
    }

    glm::quat RNG::RandomRotation()
    {
        return std::move(glm::quat(RandomDirection() * glm::vec3(180)));
    }

    float RNG::NextFloat(float min, float max)
    {
        if (max < min) return min;
        std::uniform_real_distribution<float> dist(min, max);
        return dist(engine);
    }

    int RNG::NextInt(int min, int max)
    {
        if (max < min) return min;
        std::uniform_int_distribution<int> dist(min, max);
        return dist(engine);
    }
}