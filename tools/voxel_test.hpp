#pragma once

#include <chrono>
#include <string>

#include <imgui/imgui.h>

// Phi engine
#include <phi/phi.hpp>

class VoxelTest : public Phi::App
{
    // Interface
    public:

        VoxelTest();
        ~VoxelTest();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:

        // Main scene
        Phi::Scene scene;

        // Random number generator
        Phi::RNG rng{4545};

        // DEBUG: Transform for test voxel mesh
        Phi::Transform* voxelMeshTransform = nullptr;
};