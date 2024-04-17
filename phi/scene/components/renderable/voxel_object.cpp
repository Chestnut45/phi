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
        if (splatMesh) delete splatMesh;
        if (instancedMesh) delete instancedMesh;
    }

    bool VoxelObject::Load(const std::string& path)
    {
        // Open the file
        File file(path, File::Mode::Read);
        if (file.is_open())
        {
            // Container for material ids
            std::vector<int> loadedMaterialIDs;
            std::vector<VertexVoxel> voxelData;

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
                    VertexVoxel v;

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

            // Create the internal meshes
            if (splatMesh) delete splatMesh;
            splatMesh = new VoxelMeshSplat(voxelData);
            if (instancedMesh) delete instancedMesh;
            instancedMesh = new VoxelMeshInstanced(voxelData);

            // Update size and return
            voxelCount = voxelData.size();
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

        switch (renderMode)
        {
            case RenderMode::Instanced:
                if (instancedMesh)
                {
                    instancedMesh->Render(transform ? transform->GetGlobalMatrix() : glm::mat4(1.0f));
                }
                break;
            
            case RenderMode::RayTraced:
                if (splatMesh)
                {
                    if (transform)
                        splatMesh->Render(transform->GetGlobalMatrix(), glm::mat3_cast(transform->GetGlobalRotation()));
                    else
                        splatMesh->Render(glm::mat4(1.0f), glm::mat3(1.0f));
                }
                break;
        }
    }

    void VoxelObject::FlushRenderQueue()
    {
        VoxelMeshSplat::FlushRenderQueue();
        VoxelMeshInstanced::FlushRenderQueue();
    }

    void VoxelObject::Reset()
    {
        if (splatMesh) delete splatMesh;
        splatMesh = nullptr;
        if (instancedMesh) delete instancedMesh;
        instancedMesh = nullptr;
    }
}