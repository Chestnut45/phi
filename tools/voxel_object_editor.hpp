#pragma once

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

class VoxelObjectEditor : public Phi::App
{
    // Interface
    public:

        VoxelObjectEditor();
        ~VoxelObjectEditor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:
        
        // The main voxel world
        VoxelWorld world;

        // The voxel object instance to edit
        VoxelObject* object = nullptr;

        // Settings
        bool showGUI = false;
};