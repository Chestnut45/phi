#pragma once

#include <chrono>
#include <string>

#include <imgui/imgui.h>

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

class PBRMaterialEditor : public Phi::App
{
    // Interface
    public:

        PBRMaterialEditor();
        ~PBRMaterialEditor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:
        
        // Internal scene
        Scene scene;

        // Material name
        std::string materialName{"new_material"};

        // Settings
        bool showGUI = true;

        // Displays the main interface
        void ShowInterface();
};