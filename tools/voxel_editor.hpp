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

        // Main scene
        Scene scene;

        // Voxel object pointer
        VoxelObject* voxelObject = nullptr;

        // Settings
        bool rotateModel = true;
        bool freeCamera = false;

        // Displays the main interface
        void ShowInterface();
};