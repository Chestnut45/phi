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
        if (mesh) delete mesh;
    }

    bool VoxelObject::Load(const std::string& path)
    {
        // Open the file
        File file(path, File::Mode::Read);
        if (file.is_open())
        {
            // Container for material ids
            std::vector<int> loadedMaterialIDs;
            std::vector<VertexVoxelHalfPrecision> voxelData;

            // Parse the file
            std::string line;
            int phase = 0;
            bool zAxisVertical = false;
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

                // Actual material parsing
                if (phase == 1)
                {
                    // Parse index
                    int id;
                    std::istringstream(line) >> id;

                    // Extract the name and load the proper ID for it
                    std::string name = line.substr(line.find_first_of(':') + 2);
                    loadedMaterialIDs.push_back(GetNode()->GetScene()->GetVoxelMaterialID(name));
                }
                
                // Actual voxel data parsing
                if (phase == 2)
                {
                    // Parse the voxel data
                    VertexVoxelHalfPrecision v;

                    if (zAxisVertical)
                    {
                        std::istringstream(line) >> v.x >> v.z >> v.y >> v.material;
                    }
                    else
                    {
                        std::istringstream(line) >> v.x >> v.y >> v.z >> v.material;
                    }

                    // Translate to the currently loaded ID and add to the vector
                    v.material = loadedMaterialIDs[v.material];
                    voxelData.push_back(v);
                }
            }

            // Create the internal mesh
            if (mesh) delete mesh;
            mesh = new VoxelMeshImplicit(voxelData);

            return true;
        }
        else
        {
            // Give an error message and return
            Error("File could not be opened: ", file.GetGlobalPath());
            return false;
        }
    }

    void VoxelObject::Render()
    {
        // Grab the current transform if it exists
        Transform* transform = GetNode()->Get<Transform>();

        // Render the mesh if it exists
        if (mesh) mesh->Render(transform ? transform->GetGlobalMatrix() : glm::mat4(1.0f));
    }

    void VoxelObject::FlushRenderQueue()
    {
        VoxelMeshImplicit::FlushRenderQueue();
    }

    void VoxelObject::Reset()
    {
        if (mesh) delete mesh;
        mesh = nullptr;
    }
}