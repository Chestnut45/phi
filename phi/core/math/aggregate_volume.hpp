#pragma once

#include <phi/core/math/shapes.hpp>
#include <phi/core/structures/free_list.hpp>

namespace Phi
{
    // Represents an intersection-testable volume composed of many 3D shapes
    class AggregateVolume
    {
        // Interface
        public:

            AggregateVolume();
            ~AggregateVolume();

            // Default copy constructor/assignment
            AggregateVolume(const AggregateVolume&) = default;
            AggregateVolume& operator=(const AggregateVolume&) = default;

            // Default move constructor/assignment
            AggregateVolume(AggregateVolume&& other) = default;
            AggregateVolume& operator=(AggregateVolume&& other) = default;

            // Intersection tests
            bool Intersects(const glm::vec3& point);
            
            // TODO: shape intersections as well

            // Shape / volume management

            // Adds a shape to the volume
            void AddSphere(const Sphere& sphere);
            void AddAABB(const AABB& aabb);

            // Gets a reference to the lists of shapes that define the volume
            std::vector<Sphere>& GetSpheres() { return spheres; }
            std::vector<AABB>& GetAABBs() { return aabbs; }

            // Removes all internal shapes
            void Reset();
        
        // Data / implementation
        private:

            // Volume storage
            std::vector<Sphere> spheres;
            std::vector<AABB> aabbs;
    };
}