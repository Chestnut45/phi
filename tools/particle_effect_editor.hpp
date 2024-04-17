#pragma once

// File dialogues
#include <portable-file-dialogs.h>

// Phi engine
#include <phi/phi.hpp>

class ParticleEffectEditor : public Phi::App
{
    // Interface
    public:

        ParticleEffectEditor();
        ~ParticleEffectEditor();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;

    // Data / implementation
    private:

        // Valid modes for the app to be in
        enum class Mode
        {
            Edit,
            View,
        };

        // Current state
        Mode mode;

        // Main scene
        Phi::Scene scene;

        // Window layout
        int editorWidth = 400;
        int sceneRenderWidth = wWidth - editorWidth;
        int sceneRenderHeight = wHeight;

        // The current particle effect to edit
        Phi::Node* node = nullptr;
        Phi::CPUParticleEffect* currentEffect = nullptr;

        // Displays the main effect editor window
        void ShowEditorWindow();

        static inline const char* blendOptions[] = {"None", "Additive", "Standard"};
        static inline const char* spawnOptions[] = {"Continuous", "Random", "Continuous Burst", "Random Burst", "Single Burst"};
        static inline const char* positionOptions[] = {"Point", "Random Min Max", "Random Sphere"};
        static inline const char* velocityOptions[] = {"Constant", "Random Min Max"};
        static inline const char* colorOptions[] = {"Constant", "Random Min Max", "Random Lerp", "Lerp Over Lifetime"};
        static inline const char* sizeOptions[] = {"Constant", "Random Min Max", "Random Lerp", "Lerp Over Lifetime"};
        static inline const char* opacityOptions[] = {"Constant", "Random Min Max", "Lerp Over Lifetime"};
        static inline const char* lifespanOptions[] = {"Constant", "Random Min Max"};
};