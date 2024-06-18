#pragma once

#include <phi/core/math/rng.hpp>
#include <phi/core/math/shapes.hpp>
#include <phi/core/structures/grid_3d.hpp>
#include <phi/scene/components/base_component.hpp>
#include <phi/scene/components/renderable/voxel_mesh.hpp>

namespace Phi
{
    // Data for a single voxel
    struct Voxel
    {
        // Flags for voxel simulation
        struct Flags
        {
            Flags() = delete;
            typedef uint16_t type;
            enum : type
            {
                None = 0,
                OnFire = 1,
            };
        };
        
        // Position in object local space
        int16_t x = 0;
        int16_t y = 0;
        int16_t z = 0;

        // Voxel material index
        int16_t material = -1;

        // Simulation flags
        Flags::type flags = Flags::None;
    };

    // A custom component representing an object consisting of grid aligned voxels
    // Used for various simulations involving material interactions and physics
    class VoxelObject : public BaseComponent
    {
        // Interface
        public:

            // Creates an empty voxel object with the given dimensions
            VoxelObject(int width = 32, int height = 32, int depth = 32, const glm::ivec3& offset = glm::ivec3(-16));
            ~VoxelObject();

            // Delete copy constructor/assignment
            VoxelObject(const VoxelObject &) = delete;
            VoxelObject &operator=(const VoxelObject &) = delete;

            // Delete move constructor/assignment
            VoxelObject(VoxelObject &&other) = delete;
            VoxelObject &operator=(VoxelObject &&other) = delete;

            // Simulation flags for how the object will be updated
            struct Flags
            {
                Flags() = delete;
                typedef uint32_t type;
                enum : type
                {
                    None = 0,
                    SimulateFluids = 1,
                    SimulateFire = 1 << 1,
                    UpdateMesh = 1 << 2,
                };
            };

            // Structure for returning ray cast query data
            struct RaycastInfo
            {
                // Each voxel visited in order
                // Empty voxels are marked with the material index -1
                std::vector<Voxel> visitedVoxels;

                // Index into visitedVoxels of first voxel hit, or -1 if no intersection occurred
                int firstHit = -1;
            };

            // Simulation

            // Updates the object according to the simulation flags set
            void Update(float delta);

            // Sets the given simulation flags
            inline void Enable(Flags::type flags) { this->flags |= flags; }

            // Unsets the given simulation flags
            inline void Disable(Flags::type flags) { this->flags &= !flags; }

            // Voxel data management

            // Gets a pointer to the voxel at the object local coordinates provided,
            // or nullptr if the given position is empty
            // NOTE: Does not validate position
            inline const Voxel* GetVoxel(int16_t x, int16_t y, int16_t z)
            {
                int index = voxelGrid(x - offset.x, y - offset.y, z - offset.z);
                return index == -1 ? nullptr : &voxels[index];
            }

            // Sets the voxel data to a specific material
            // NOTE: Does not validate position
            inline void SetVoxel(int16_t x, int16_t y, int16_t z, int16_t material)
            {
                // Initialize voxel data
                Voxel voxel;
                voxel.x = x;
                voxel.y = y;
                voxel.z = z;
                voxel.material = material;

                // Place on grid (update existing or push back)
                int& index = voxelGrid(x - offset.x, y - offset.y, z - offset.z);
                if (index == -1)
                {
                    index = voxels.size();
                    voxels.push_back(voxel);
                }
                else
                {
                    voxels[index] = voxel;
                }
            }

            // TODO: Remove voxels without rebuilding all indices...

            // Loads voxel data from a .vobj file, replacing any existing data
            // Accepts local paths like data:// and user://
            bool Load(const std::string &path);

            // Resets and unloads all voxel data, including mesh vertices
            void Reset();

            // Spatial queries

            // Casts an object-local ray into the voxel object, returns voxel intersection information
            RaycastInfo Raycast(const Ray &ray, int maxSteps = 512);

            // Mesh management

            // Updates the internal mesh to match the voxel grid
            void UpdateMesh();

            // Returns a pointer to the internal mesh component,
            // or nullptr if none exists
            inline VoxelMesh *GetMesh() const { return mesh; }

            // Returns a const reference to the voxel-space integer AABB
            inline const IAABB &GetAABB() const { return aabb; }

        // Data / implementation
        private:

            // Spatial index for voxels
            // -1 indicates an empty spot on the grid
            // Any other non-negative value indicates an index into the internal voxel array
            Grid3D<int> voxelGrid;

            // Array of all voxel data
            std::vector<Voxel> voxels;

            // Offset to apply to obtain object-local space coordinates
            glm::ivec3 offset;

            // Timing
            int updatesPerSecond = 60;
            float timeAccum = 0.0f;
            float updateRate = 1.0f / updatesPerSecond;

            // Simulation flags
            Flags::type flags;

            // AABB that bounds voxels in object local space
            IAABB aabb;

            // Internal mesh component (NON-OWNING)
            VoxelMesh *mesh = nullptr;
    };
}