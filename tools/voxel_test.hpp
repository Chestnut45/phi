#pragma once

#include <chrono>
#include <string>

#include <imgui/imgui.h>

// Phi engine
#include <phi/phi.hpp>

using namespace Phi;

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
        Scene scene;

        // Transform for test voxel mesh
        Transform* voxelMeshTransform = nullptr;
};