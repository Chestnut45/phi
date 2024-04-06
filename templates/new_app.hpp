#pragma once

// Phi engine
#include <phi/phi.hpp>

class NewApp : public Phi::App
{
    // Interface
    public:

        NewApp();
        ~NewApp();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:

        // TODO: Private app globals
};