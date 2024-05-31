#include "voxel_object.hpp"

#include <phi/core/file.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    VoxelObject::VoxelObject()
    {
    }

    VoxelObject::~VoxelObject()
    {
    }

    VoxelObject::RaycastInfo VoxelObject::Raycast(const Ray& ray, int maxSteps)
    {
        RaycastInfo result;

        // Create a copy of the ray since we have to offset it
        Ray r = ray;

        // Add half voxel offset since voxels are rendered at their centers
        r.origin += glm::vec3(0.5f);

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
                    gridXYZ.x < voxels.GetWidth() &&
                    gridXYZ.y < voxels.GetHeight() &&
                    gridXYZ.z < voxels.GetDepth())
                {
                    // Add to visited list
                    result.visitedVoxels.push_back(xyz);

                    // Check for voxel at current position
                    int voxel = voxels(gridXYZ.x, gridXYZ.y, gridXYZ.z);
                    if (voxel != 0)
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

    bool VoxelObject::Load(const std::string& path)
    {
        // Open the file
        File file(path, File::Mode::Read);
        if (file.is_open())
        {
            // Container for material ids
            std::vector<int> loadedMaterialIDs;
            std::vector<int> voxelData;

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
                    loadedMaterialIDs.push_back(GetNode()->GetScene()->GetVoxelMaterialID(name));
                }
                
                // Voxel data parsing
                if (phase == 2)
                {
                    // Parse the voxel data
                    int x, y, z, material;

                    if (zAxisVertical)
                    {
                        std::istringstream(line) >> x >> z >> y >> material;
                    }
                    else
                    {
                        std::istringstream(line) >> x >> y >> z >> material;
                    }

                    // Translate to the currently loaded ID
                    material = loadedMaterialIDs[material];
                    
                    // Update min and max coords
                    // TODO: Safety loading empty models?
                    min.x = x < min.x ? x : min.x;
                    min.y = y < min.y ? y : min.y;
                    min.z = z < min.z ? z : min.z;
                    max.x = x > max.x ? x : max.x;
                    max.y = y > max.y ? y : max.y;
                    max.z = z > max.z ? z : max.z;

                    // Add to voxel data
                    voxelData.emplace_back(x);
                    voxelData.emplace_back(y);
                    voxelData.emplace_back(z);
                    voxelData.emplace_back(material);
                }
            }
            
            // Update all internal voxel data
            voxels.Resize(max.x - min.x + 1, max.y - min.y + 1, max.z - min.z + 1);
            offset = min;
            for (int i = 0; i < voxelData.size() - 4; i += 4)
            {
                voxels(voxelData[i] - offset.x, voxelData[i + 1] - offset.y, voxelData[i + 2] - offset.z) = voxelData[i + 3];
            }

            // Update AABB
            aabb.min = min;
            aabb.max = max;

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
        voxels.Clear();
        if (mesh) mesh->Vertices().clear();
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

        // Grab references
        auto& verts = mesh->Vertices();
        int w = voxels.GetWidth();
        int h = voxels.GetHeight();
        int d = voxels.GetDepth();
        
        // Add only visible voxels to mesh
        verts.clear();
        for (int z = 0; z < d; ++z)
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    const auto& v = voxels(x, y, z);
                    if (v == 0) continue;

                    // Get world-space position of this voxel
                    glm::ivec3 position = glm::ivec3(x, y, z) + offset;

                    if (x == 0 || y == 0 || z == 0 || x == w - 1 || y == h - 1 || z == d - 1)
                    {
                        VertexVoxelHalfPrecision vert;
                        vert.x = position.x;
                        vert.y = position.y;
                        vert.z = position.z;
                        vert.material = v;
                        verts.push_back(vert);
                        continue;
                    }
                    
                    if (voxels(x - 1, y, z) == 0 ||
                        voxels(x + 1, y, z) == 0 ||
                        voxels(x, y - 1, z) == 0 ||
                        voxels(x, y + 1, z) == 0 ||
                        voxels(x, y, z - 1) == 0 ||
                        voxels(x, y, z + 1) == 0)
                    {
                        VertexVoxelHalfPrecision vert;
                        vert.x = position.x;
                        vert.y = position.y;
                        vert.z = position.z;
                        vert.material = v;
                        verts.push_back(vert);
                    }
                }
            }
        }
    }

    void VoxelObject::DestroyMesh()
    {
        GetNode()->RemoveComponent<VoxelMesh>();
        mesh = nullptr;
    }
}