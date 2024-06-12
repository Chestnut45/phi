#include "voxel_object.hpp"

#include <phi/core/file.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    VoxelObject::VoxelObject(int width, int height, int depth, const glm::ivec3& offset)
        : voxelGrid(width, height, depth, -1), offset(offset), flags(Flags::None)
    {
        aabb.min = offset;
        aabb.max = glm::ivec3(width + offset.x, height + offset.y, depth + offset.z);
    }

    VoxelObject::~VoxelObject()
    {
    }

    void VoxelObject::Update(float delta)
    {
        // Update timer
        timeAccum += delta;
        if (timeAccum < updateRate) return;
        
        // Reset timer on each successful update
        timeAccum = 0.0f;
        simulationTurn = !simulationTurn;

        // Grab reference to scene material data
        const auto& voxelMaterials = GetNode()->GetScene()->GetVoxelMaterials();

        // Fluid simulation step
        if (flags & Flags::SimulateFluids)
        {
            // Grab empty grid value
            int empty = voxelGrid.GetEmptyValue();
            int h = voxelGrid.GetHeight();

            // Iterate all voxels
            for (auto& voxel : voxels)
            {
                // Only simulate voxels with a fluid material
                if (!(voxelMaterials[voxel.material].flags & VoxelMaterial::Flags::Liquid)) continue;

                // Update pressure if necessary
                if (voxel.turn != simulationTurn)
                {
                    voxel.turn = simulationTurn;
                    voxel.pressure = voxel.newPressure;
                }

                // Calculate grid position
                int gridX = voxel.position.x - offset.x;
                int gridY = voxel.position.y - offset.y;
                int gridZ = voxel.position.z - offset.z;

                // Grab current voxel index reference
                int& index = voxelGrid(gridX, gridY, gridZ);

                // Initialize neighbour index pointers
                int* pBelow = (voxel.position.y > aabb.min.y) ? &voxelGrid(gridX, gridY - 1, gridZ) : nullptr;
                int* pAbove = (voxel.position.y < aabb.max.y - 1) ? &voxelGrid(gridX, gridY + 1, gridZ) : nullptr;
                int* pLeft = (voxel.position.x > aabb.min.x) ? &voxelGrid(gridX - 1, gridY, gridZ) : nullptr;
                int* pRight = (voxel.position.x < aabb.max.x - 1) ? &voxelGrid(gridX + 1, gridY, gridZ) : nullptr;
                int* pForward = (voxel.position.z > aabb.min.z) ? &voxelGrid(gridX, gridY, gridZ - 1) : nullptr;
                int* pBack = (voxel.position.z < aabb.max.z - 1) ? &voxelGrid(gridX, gridY, gridZ + 1) : nullptr;

                int* pNeighbours[] =
                {
                    pBelow, pAbove, pLeft, pRight, pForward, pBack
                };

                // Calculate and distribute pressure

                // TODO:
                // - don't prefer any direction
                // - don't start with any pressure in the system
                // - add implicit pressure based on gridY value (guaranteed positive)
                // - flow modelling

                // DEBUG: Constants that may become material properties
                static const int basePressure = 1.0f;
                static const int maxPressure = 0.01f;

                // METHOD 03: Pathfinding Pressure
                
                // Is moving down possible?
                // if (pBelow)
                // {
                //     // Can we move immediately?
                //     if (*pBelow == empty)
                //     {
                        
                //     }
                //     else
                //     {
                //         // Grab below neighbour
                //         Voxel* vBelow = &voxels[*pBelow];
                //     }
                // }

                // METHOD 02: Biased branching (bad but fast)
                // Index to move to (if any)
                // int* pMoveIndex = nullptr;
                // int fluidNeighbours = 0;

                // for (int i = 0; i < 6; ++i)
                // {
                //     int* pNeighbour = pNeighbours[i];
                //     if (pNeighbour)
                //     {
                //         // Grab neighbour data if it exists
                //         Voxel* vNeighbour = *pNeighbour == empty ? nullptr : &voxels[*pNeighbour];

                //         // If neighbour exists
                //         if (vNeighbour)
                //         {
                //             // Neighbour does exist, continue if fluid
                //             if (voxelMaterials[vNeighbour->material].flags & VoxelMaterial::Flags::Liquid)
                //             {
                //                 // Update count
                //                 fluidNeighbours++;
                //             }
                //         }
                //         else
                //         {
                //             // Neighbour does not exist, should we move?
                //             if (pNeighbour == pBelow)
                //             {
                //                 // Move down if possible
                //                 pMoveIndex = pNeighbour;
                //             }
                //             else if (pNeighbour == pAbove)
                //             {
                //                 // Move up due to pressure?
                //             }
                //             else
                //             {
                //                 // Spread out horizontally (unless we can move down)
                //                 pMoveIndex = (pMoveIndex && pMoveIndex == pBelow) ? pMoveIndex : pNeighbour;
                //             }
                //         }
                //     }
                // }

                // // Move if we can
                // if (pMoveIndex && (pMoveIndex == pBelow || fluidNeighbours > 0))
                // {
                //     // Update voxel
                //     voxel.position.x += pMoveIndex == pLeft ? -1 : pMoveIndex == pRight ? 1 : 0;
                //     voxel.position.y += pMoveIndex == pBelow ? -1 : pMoveIndex == pAbove ? 1 : 0;
                //     voxel.position.z += pMoveIndex == pForward ? -1 : pMoveIndex == pBack ? 1 : 0;
                //     std::swap(*pMoveIndex, index);
                // }

                // METHOD 01: Path of least resistance / pressure distribution
                float maxFlow = 0.0f;
                int* pBestPath = nullptr;

                for (int i = 0; i < 6; ++i)
                {
                    int* pNeighbour = pNeighbours[i];
                    if (pNeighbour)
                    {
                        float flow = 0.0f;
                        Voxel* vNeighbour = *pNeighbour == empty ? nullptr : &voxels[*pNeighbour];
                        
                        // Only distribute pressure between fluids
                        if (vNeighbour && !(voxelMaterials[vNeighbour->material].flags & VoxelMaterial::Flags::Liquid)) continue;

                        // Update pressure
                        if (vNeighbour && vNeighbour->turn != simulationTurn)
                        {
                            vNeighbour->turn = simulationTurn;
                            vNeighbour->pressure = vNeighbour->newPressure;
                        }

                        // TODO: Flow should only be calculated when vNeighbour is true
                        // otherwise, only move based on pressure gradient

                        // Calculate pressure
                        float voxelPressure = voxel.pressure + basePressure;
                        float neighbourPressure = vNeighbour ? vNeighbour->pressure + basePressure : 0.0f;
                        float deltaPressure = voxelPressure - neighbourPressure;

                        if (pNeighbour == pAbove)
                        {
                            // Neighbour is above
                            if (voxelPressure < basePressure || neighbourPressure < basePressure)
                            {
                                flow = voxelPressure - basePressure;
                            }
                            else
                            {
                                flow = (voxelPressure + neighbourPressure) - maxPressure;
                                flow *= 0.5f;
                            }
                        }
                        else if (pNeighbour == pBelow)
                        {
                            // Neighbour is below
                            if (voxelPressure < basePressure || neighbourPressure < basePressure)
                            {
                                flow = basePressure - neighbourPressure;
                            }
                            else
                            {
                                flow = (voxelPressure + neighbourPressure) + maxPressure;
                                flow *= 0.5f;
                            }
                        }
                        else
                        {
                            // Neighbour is same height
                            flow = (voxelPressure + neighbourPressure) * 0.5f;
                        }
                        
                        // Distribute flow
                        // flow *= 0.1666f;
                        flow = glm::clamp(flow, voxelPressure * 0.1666f, -neighbourPressure * 0.1666f);
                        
                        if (vNeighbour)
                        {
                            voxel.newPressure -= flow;
                            vNeighbour->newPressure += flow;
                        }

                        // Update best path
                        if (pNeighbour == pBelow || (flow > maxFlow && pNeighbour != pAbove && !vNeighbour))
                        {
                            maxFlow = flow;
                            pBestPath = pNeighbour;
                        }
                    }
                }

                // Move to area with the greatest pressure difference
                if (pBestPath && *pBestPath == empty)
                {
                    // Update voxel
                    voxel.position.x += pBestPath == pLeft ? -1 : pBestPath == pRight ? 1 : 0;
                    voxel.position.y += pBestPath == pBelow ? -1 : pBestPath == pAbove ? 1 : 0;
                    voxel.position.z += pBestPath == pForward ? -1 : pBestPath == pBack ? 1 : 0;
                    // voxel.newPressure = 0.0f;
                    
                    // Update grid
                    std::swap(index, *pBestPath);
                }
            }

            // DEBUG: Always update mesh
            UpdateMesh();
        }
    }

    bool VoxelObject::Load(const std::string& path)
    {
        // Open the file
        File file(path, File::Mode::Read);
        if (file.is_open())
        {
            // Container for material ids
            std::vector<int> loadedMaterialIDs;
            std::vector<Voxel> newVoxels;

            // Material translation access
            Scene* scene = GetNode()->GetScene();

            // Parse the file
            std::string line;
            int phase = 0;
            bool zAxisVertical = false;
            glm::ivec3 min(0);
            glm::ivec3 max(0);
            while (std::getline(file, line))
            {
                // Ignore comments and empty lines
                if (line[0] == '#' || line.size() < 1) continue;

                // Setup phase
                if (line == ".materials")
                {
                    phase = 1;
                    continue;
                }
                if (line == ".voxels")
                {
                    phase = 2;
                    continue;
                }
                if (line == ".z_axis_vertical") zAxisVertical = true;

                // Material parsing
                if (phase == 1)
                {
                    // Parse index
                    int id;
                    std::istringstream(line) >> id;

                    // Extract the name and load the proper ID for it
                    std::string name = line.substr(line.find_first_of(':') + 2);
                    loadedMaterialIDs.push_back(scene->GetVoxelMaterialID(name));
                }
                
                // Voxel data parsing
                if (phase == 2)
                {
                    // Parse the voxel data
                    Voxel voxel;

                    if (zAxisVertical)
                    {
                        std::istringstream(line) >> voxel.position.x >> voxel.position.z >> voxel.position.y >> voxel.material;
                    }
                    else
                    {
                        std::istringstream(line) >> voxel.position.x >> voxel.position.y >> voxel.position.z >> voxel.material;
                    }

                    // Translate to the currently loaded ID
                    voxel.material = loadedMaterialIDs[voxel.material];
                    
                    // Update min and max coords
                    // TODO: Safety loading empty models?
                    min.x = voxel.position.x < min.x ? voxel.position.x : min.x;
                    min.y = voxel.position.y < min.y ? voxel.position.y : min.y;
                    min.z = voxel.position.z < min.z ? voxel.position.z : min.z;
                    max.x = voxel.position.x > max.x ? voxel.position.x : max.x;
                    max.y = voxel.position.y > max.y ? voxel.position.y : max.y;
                    max.z = voxel.position.z > max.z ? voxel.position.z : max.z;

                    // Add to voxel data
                    newVoxels.emplace_back(voxel);
                }
            }
            
            // Update all internal voxel data
            voxelGrid.Resize(max.x - min.x + 1, max.y - min.y + 1, max.z - min.z + 1);
            offset = min;
            for (const auto& voxel : newVoxels)
            {
                SetVoxel(voxel.position.x, voxel.position.y, voxel.position.z, voxel.material);
            }

            // Update AABB
            aabb.min = min;
            aabb.max = max + 1;

            UpdateMesh();
            return true;
        }
        else
        {
            // Give an error message and return
            Error("File could not be opened: ", file.GetGlobalPath());
            return false;
        }
    }

    void VoxelObject::Reset()
    {
        voxelGrid.Clear();
        if (mesh) mesh->Vertices().clear();
    }

    VoxelObject::RaycastInfo VoxelObject::Raycast(const Ray& ray, int maxSteps)
    {
        RaycastInfo result;

        // Create a copy of the ray since we have to offset it
        Ray r = ray;

        // Determine intersection with object
        glm::vec2 tNearFar = r.Slabs(aabb);
        if (tNearFar.x < tNearFar.y)
        {
            // Calculate starting position (with fractional component)
            glm::vec3 start = r.origin + r.direction * (tNearFar.x > 0.0f ? tNearFar.x : 0.0f);

            // Calculate step directions
            glm::ivec3 step = glm::ivec3(glm::sign(r.direction.x), glm::sign(r.direction.y), glm::sign(r.direction.z));

            // Calculate starting and ending voxel
            glm::ivec3 xyz = glm::floor(start);
            glm::ivec3 oob = glm::floor(r.origin + r.direction * tNearFar.y);

            // Avoid infinite loop
            if (step == glm::ivec3(0))
            {
                Error("Bad raycast (0 direction!)");
                return std::move(result);
            }

            // Calculate tMax and tDelta
            glm::vec3 tMax;
            tMax.x = (r.direction.x > 0 ? glm::ceil(start.x) - start.x : start.x - glm::floor(start.x)) / glm::abs(r.direction.x);
            tMax.y = (r.direction.y > 0 ? glm::ceil(start.y) - start.y : start.y - glm::floor(start.y)) / glm::abs(r.direction.y);
            tMax.z = (r.direction.z > 0 ? glm::ceil(start.z) - start.z : start.z - glm::floor(start.z)) / glm::abs(r.direction.z);
            
            glm::vec3 tDelta = glm::vec3(step) / r.direction;

            // Grid traversal (Amanatides & Woo)
            int steps = 0;
            do
            {
                steps++;

                // Calculate grid coordinate from object local
                glm::ivec3 gridXYZ = xyz - offset;

                // Check if coordinate is in bounds of the grid
                if (gridXYZ.x >= 0 &&
                    gridXYZ.y >= 0 &&
                    gridXYZ.z >= 0 &&
                    gridXYZ.x < voxelGrid.GetWidth() &&
                    gridXYZ.y < voxelGrid.GetHeight() &&
                    gridXYZ.z < voxelGrid.GetDepth())
                {
                    // Add to visited list
                    result.visitedVoxels.push_back(xyz);

                    // Check for voxel at current position
                    int voxel = voxelGrid(gridXYZ.x, gridXYZ.y, gridXYZ.z);
                    if (voxel != voxelGrid.GetEmptyValue())
                    {
                        result.firstHit = result.visitedVoxels.size() - 1;
                        break;
                    }
                }

                // Step to next voxel
                if (tMax.x < tMax.y)
                {
                    if (tMax.x < tMax.z)
                    {
                        xyz.x += step.x;
                        if (xyz.x == oob.x) break;
                        tMax.x += tDelta.x;
                    }
                    else
                    {
                        xyz.z += step.z;
                        if (xyz.z == oob.z) break;
                        tMax.z += tDelta.z;
                    }
                }
                else
                {
                    if (tMax.y < tMax.z)
                    {
                        xyz.y += step.y;
                        if (xyz.y == oob.y) break;
                        tMax.y += tDelta.y;
                    }
                    else
                    {
                        xyz.z += step.z;
                        if (xyz.z == oob.z) break;
                        tMax.z += tDelta.z;
                    }
                }
                
            } while (steps <= maxSteps);
        }

        return std::move(result);
    }

    void VoxelObject::UpdateMesh()
    {
        // Create the mesh if it doesn't exist yet
        if (!mesh)
        {
            mesh = GetNode()->Get<VoxelMesh>();
            if (!mesh)
            {
                mesh = &GetNode()->AddComponent<VoxelMesh>();
            }
        }

        // Clear vertices and update mesh
        auto& verts = mesh->Vertices();
        verts.clear();
        const auto& voxelMaterials = GetNode()->GetScene()->GetVoxelMaterials();
        for (const auto& voxel : voxels)
        {
            // TODO: Profile, is visibility culling worth the cost in mesh construction time?
            VertexVoxelHalfPrecision vert;
            vert.x = voxel.position.x;
            vert.y = voxel.position.y;
            vert.z = voxel.position.z;
            vert.material = voxelMaterials[voxel.material].pbrID;
            verts.push_back(vert);
        }
    }

    void VoxelObject::DestroyMesh()
    {
        GetNode()->RemoveComponent<VoxelMesh>();
        mesh = nullptr;
    }
}