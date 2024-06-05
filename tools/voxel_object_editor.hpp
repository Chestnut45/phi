#pragma once

#include <unordered_map>

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

        // Valid brush modes
        enum class BrushMode
        {
            Add,
            Subtract,
            Paint
        };

        // Main scene
        Scene scene;

        // The voxel object instance to edit
        VoxelObject* object = nullptr;

        // Brush mesh
        VoxelMesh* brushMesh = nullptr;

        // Brush
        BrushMode brushMode = BrushMode::Add;
        Voxel selectedVoxel;

        // Brush stroke edits
        std::unordered_map<glm::ivec3, Voxel> currentEdits;

        // Settings
        bool showGUI = false;
};