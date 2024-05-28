#pragma once

#include <chrono>
#include <string>

#include <imgui/imgui.h>

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

class VoxelMapEditor : public Phi::App
{
    // Interface
    public:

        VoxelMapEditor();
        ~VoxelMapEditor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:
        
        // Main scene
        Scene scene;

        // Settings
        bool showGUI = true;

        // Displays the main interface
        void ShowInterface();
};