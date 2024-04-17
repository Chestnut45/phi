#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <phi/scene/components/base_component.hpp>
#include "cpu_particle_emitter.hpp"

// Forward declaration for editor access
class ParticleEffectEditor;

namespace Phi
{
    // Represents a single particle effect simulated on the CPU
    class CPUParticleEffect : public BaseComponent
    {
        // Interface
        public:

            // Valid states for particle effects
            enum class State
            {
                Play,
                Paused,
                Stopped
            };

            // Creates an empty particle effect
            CPUParticleEffect();

            // Loads a particle effect from a YAML file
            // Accepts local paths like data:// and user://
            CPUParticleEffect(const std::string& path);

            ~CPUParticleEffect();

            // Delete copy constructor/assignment
            CPUParticleEffect(const CPUParticleEffect&) = delete;
            CPUParticleEffect& operator=(const CPUParticleEffect&) = delete;

            // Delete move constructor/assignment
            CPUParticleEffect(CPUParticleEffect&& other) = delete;
            CPUParticleEffect& operator=(CPUParticleEffect&& other) = delete;

            // Simulation

            // Updates all particle emitters in the effect
            void Update(float delta);

            // Renders all emitters that belong to this effect
            void Render();

            // Flushes all effects / emitters queued for rendering by Render()
            static void FlushRenderQueue();

            // Controls

            // Changes the current state to play
            // Accumulators increment normally and all particles are simulated
            void Play();

            // Changes the current state to paused
            // Accumulators are halted and no particles will be spawned
            // Update does not simulate active particles
            void Pause();

            // Changes the current state to stopped
            // Accumulators are halted and no particles will be spawned
            // Update still simulates and despawns particles
            void Stop();

            // Clears all active particles and resets timers / accumulators for emitters
            void Restart();

            // Serialization

            // Loads the effect properties from a YAML file on disk
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Saves the effect to disk
            // Accepts local paths like data:// and user://
            void Save(const std::string& path, bool singleFile = false) const;

            // Removes all emitters and resets to default values
            void Reset();

            // Accessors

            // Returns the name of the effect
            inline const std::string& GetName() const { return name; }

        // Data / implementation
        private:

            // Name
            std::string name{"New Effect"};

            // Current state
            State state{State::Play};

            // Emitters
            std::vector<CPUParticleEmitter> loadedEmitters;

            // Settings
            bool renderRelativeTransform = false;
            bool spawnRelativeTransform = false;

            // Friends
            
            // Necessary for the particle effect editor to work
            friend class ::ParticleEffectEditor;
    };
}