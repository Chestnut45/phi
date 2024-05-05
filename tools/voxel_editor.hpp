#pragma once

#include <chrono>
#include <string>

#include <imgui/imgui.h>

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

class VoxelEditor : public Phi::App
{
    // Interface
    public:

        VoxelEditor();
        ~VoxelEditor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:
        
        // Internal voxel world
        VoxelWorld world;

        // Settings
        bool showGUI = true;

        // Displays the main interface
        void ShowInterface();
};