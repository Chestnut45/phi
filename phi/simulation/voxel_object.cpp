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
                    loadedMaterialIDs.push_back(GetNode()->GetScene()->GetMaterialID(name));
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
        if (!mesh) return;
        GetNode()->RemoveComponent<VoxelMesh>();
        mesh = nullptr;
    }
}