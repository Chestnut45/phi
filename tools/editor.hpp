#pragma once

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

class Editor : public Phi::App
{
    // Interface
    public:

        Editor();
        ~Editor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:

        // Main scene
        Scene scene;

        // Settings
        bool showDebug = true;

        // Window rendering methods
        void DisplayHierarchy();
        void DisplayInspector();
        void DisplayCamera();
        void DisplayResources();
};