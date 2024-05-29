#include "voxel_material.hpp"

namespace Phi
{
    VoxelMaterial::VoxelMaterial(const std::string& name, SimulationFlags::type simulationFlags, int pbrID)
        : name(name), simulationFlags(simulationFlags), pbrID(pbrID)
    {
    }

    VoxelMaterial::~VoxelMaterial()
    {
    }
}