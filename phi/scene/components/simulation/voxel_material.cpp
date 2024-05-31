#include "voxel_material.hpp"

namespace Phi
{
    VoxelMaterial::VoxelMaterial(const std::string& name, Flags::type flags, int pbrID)
        : name(name), flags(flags), pbrID(pbrID)
    {
    }

    VoxelMaterial::~VoxelMaterial()
    {
    }
}