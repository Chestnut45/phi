#pragma once

#include <algorithm>
#include <cstdint>
#include <algorithm>
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
            Grid3D(int width, int height, int depth);
            ~Grid3D();

            // Delete copy constructor/assignment
            Grid3D(const Grid3D&) = delete;
            Grid3D& operator=(const Grid3D&) = delete;

            // Delete move constructor/assignment
            Grid3D(Grid3D&& other) = delete;
            Grid3D& operator=(Grid3D&& other) = delete;

            // Data access / modification

            // Fast read-write access, no bounds checking
            inline T& operator()(int x, int y, int z)
            {
                return data[Index(x, y, z)];
            }

            // Clears the grid (default initializes each entry)
            void Clear();

            // Resizes and clears the grid
            void Resize(int width, int height, int depth);

            // Dimension accessors
            int GetWidth() const { return width; }
            int GetHeight() const { return height; }
            int GetDepth() const { return depth; }

        // Data / implementation
        private:

            // Grid dimension boundaries
            int width, height, depth;
            size_t totalElementSize;

            // Data
            std::vector<T> data;

            // Calculate index into the internal array from 3D position
            inline uint32_t Index(int x, int y, int z) const
            {
                return x + width * (y + height * z);
            }
    };

    // Template implementation

    template <typename T>
    Grid3D<T>::Grid3D(int width, int height, int depth)
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
        std::fill(data.begin(), data.end(), T());
    }

    template <typename T>
    void Grid3D<T>::Resize(int width, int height, int depth)
    {
        // Set new dimensions
        this->width = width;
        this->height = height;
        this->depth = depth;

        // Calculate new data element size and default construct each object
        totalElementSize = width * height * depth;
        data.resize(totalElementSize);
        Clear();
    }
}