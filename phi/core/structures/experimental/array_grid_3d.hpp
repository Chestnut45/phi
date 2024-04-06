#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Phi
{
    // Represents a dense regular 3D grid of arbitrary data and size
    // Fast, consistent O(1) lookups at the cost of dense storage for elements
    template <typename T>
    class ArrayGrid3D
    {
        // Interface
        public:

            // Creates a 3D array with the following bounds:
            // [xOrigin, xOrigin + width)
            // [yOrigin, yOrigin + height)
            // [zOrigin, zOrigin + depth)
            ArrayGrid3D(size_t width, size_t height, size_t depth, int32_t xOrigin = 0, int32_t yOrigin = 0, int32_t zOrigin = 0);
            ~ArrayGrid3D();

            // Default copy constructor/assignment
            ArrayGrid3D(const ArrayGrid3D&) = default;
            ArrayGrid3D& operator=(const ArrayGrid3D&) = default;

            // Default move constructor/assignment
            ArrayGrid3D(ArrayGrid3D&& other) = default;
            ArrayGrid3D& operator=(ArrayGrid3D&& other) = default;

            // Data access / modification

            // Fast read-write access, no bounds checking
            T& operator()(int32_t x, int32_t y, int32_t z)
            {

            }

            // Resizes the grid, 

        // Data / implementation
        private:

            // Grid dimension boundaries
            size_t width, height, depth;
            int32_t xOrigin, yOrigin, zOrigin;

            // Calculate index into the internal array from 3D position
            inline uint32_t Index(int32_t x, int32_t y, int32_t z) const
            {
                return (x - xOrigin) + width * ((y - yOrigin) + depth * (z - zOrigin));
            }
    };
}