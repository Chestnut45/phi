#include "aggregate_volume.hpp"

#include <algorithm>

namespace Phi
{
    AggregateVolume::AggregateVolume()
    {
    }

    AggregateVolume::~AggregateVolume()
    {
    }

    bool AggregateVolume::Intersects(const glm::vec3& point) const
    {
        for (const Sphere& sphere : spheres)
        {
            if (sphere.Intersects(point)) return true;
        }

        for (const AABB& aabb : aabbs)
        {
            if (aabb.Intersects(point)) return true;
        }

        return false;
    }

    void AggregateVolume::AddSphere(const Sphere& sphere)
    {
        spheres.push_back(sphere);
    }

    void AggregateVolume::AddAABB(const AABB& aabb)
    {
        aabbs.push_back(aabb);
    }

    void AggregateVolume::Reset()
    {
        spheres.clear();
        aabbs.clear();
    }
}