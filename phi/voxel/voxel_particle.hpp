#pragma once

#include <glm/glm.hpp>

#include <phi/voxel/voxel.hpp>

namespace Phi
{
    class VoxelParticle
    {
        // Interface
        public:

            VoxelParticle();
            ~VoxelParticle();

            // Delete copy constructor/assignment
            VoxelParticle(const VoxelParticle&) = delete;
            VoxelParticle& operator=(const VoxelParticle&) = delete;

            // Delete move constructor/assignment
            VoxelParticle(VoxelParticle&& other) = delete;
            void operator=(VoxelParticle&& other) = delete;
        
        // Data / implementation
        private:

            Voxel voxel;
            glm::vec3 position;
    };
}