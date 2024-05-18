#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Phi
{
    // Represents a dense regular 3D grid of arbitrary data and size
    // Fast, consistent O(1) lookups at the cost of dense storage for elements
    template <typename T>
    class Grid3D
    {
        // Interface
        public:

            // Creates a 3D grid with the following bounds:
            // [0, width - 1]
            // [0, height - 1]
            // [0, depth - 1]
            Grid3D(int32_t width, int32_t height, int32_t depth);
            ~Grid3D();

            // Delete copy constructor/assignment
            Grid3D(const Grid3D&) = delete;
            Grid3D& operator=(const Grid3D&) = delete;

            // Delete move constructor/assignment
            Grid3D(Grid3D&& other) = delete;
            Grid3D& operator=(Grid3D&& other) = delete;

            // Data access / modification

            // Fast read-write access, no bounds checking
            inline T& operator()(int32_t x, int32_t y, int32_t z)
            {
                return data[Index(x, y, z)];
            }

            // Clears the grid (default initializes each entry)
            void Clear();

            // Resizes and clears the grid
            void Resize(int32_t width, int32_t height, int32_t depth);

        // Data / implementation
        private:

            // Grid dimension boundaries
            int32_t width, height, depth;

            // Data
            std::vector<T> data;

            // Calculate index into the internal array from 3D position
            inline uint32_t Index(int32_t x, int32_t y, int32_t z) const
            {
                return x + width * (y + depth * z);
            }
    };

    // Template implementation

    template <typename T>
    Grid3D<T>::Grid3D(int32_t width, int32_t height, int32_t depth)
    {
        // Initialize the grid
        Resize(width, height, depth);
    }

    template <typename T>
    Grid3D<T>::~Grid3D()
    {
    }

    template <typename T>
    void Grid3D<T>::Clear()
    {
        Resize(width, height, depth);
    }

    template <typename T>
    void Grid3D<T>::Resize(int32_t width, int32_t height, int32_t depth)
    {
        // Set new dimensions
        this->width = width;
        this->height = height;
        this->depth = depth;

        // Calculate new data element size and default construct each object
        size_t totalElementSize = width * height * depth;
        data.clear();
        data.insert(data.end(), totalElementSize, T());
    }
}