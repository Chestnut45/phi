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

        // Scene editor globals
        static inline Node* selectedNode = nullptr;

        // Component name map
        std::map<entt::id_type, std::string> componentNames;

        // Registers all the components to be used in the editor
        // NOTE: Edit this when you make a new component type if you want to use it in the editor!
        void RegisterComponents();

        // GUI methods
        void GUIMainMenuBar();
        void GUISceneHierarchy();
        void GUIInspector();
        void GUISceneCamera();
        void GUIResources();
        void GUIConsole();
        void GUIPerformanceStats();

        // Flag to show debug window
        bool showDebug = false;
};