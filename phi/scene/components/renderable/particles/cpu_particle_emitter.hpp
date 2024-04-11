#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

#include <phi/core/math/noise.hpp>
#include <phi/core/math/rng.hpp>
#include <phi/core/resource_manager.hpp>
#include <phi/graphics/texture_2d.hpp>
#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/shader.hpp>
#include <phi/graphics/indirect.hpp>
#include <phi/graphics/vertex_attributes.hpp>

// Forward declaration for editor access
class ParticleEffectEditor;

namespace Phi
{
    // Represents a single particle emitter that uses the CPU for simulation
    class CPUParticleEmitter
    {
        // Valid emitter spawning modes
        enum class SpawnMode : int
        {
            Continuous = 0,
            Random,
            ContinuousBurst,
            RandomBurst,
            SingleBurst
        };

        // Valid blend modes
        enum class BlendMode : int
        {
            None = 0,
            Additive,
            Standard
        };

        // Valid spawn position modes
        enum class PositionMode : int
        {
            Constant = 0,
            RandomMinMax,
            RandomSphere,
        };

        // Valid particle velocity modes
        enum class VelocityMode : int
        {
            Constant = 0,
            RandomMinMax
        };

        // Valid particle color modes
        enum class ColorMode : int
        {
            Constant = 0,
            RandomMinMax,
            RandomLerp,
            LerpOverLifetime
        };

        // Valid particle size modes
        enum class SizeMode : int
        {
            Constant = 0,
            RandomMinMax,
            RandomLerp,
            LerpOverLifetime
        };

        // Valid particle opacity modes
        enum class OpacityMode : int
        {
            Constant = 0,
            RandomMinMax,
            LerpOverLifetime
        };

        // Valid particle lifetime modes
        enum class LifespanMode : int
        {
            Constant = 0,
            RandomMinMax
        };

        // TODO: Benchmark against separating particle vertex data
        // Potentially saving PCIe bandwidth (and VRAM usage) at the
        // cost of (some) cache efficiency when iterating particles
        
        // Particle data structure
        struct Particle
        {
            glm::vec3 position;
            glm::vec3 velocity;
            glm::vec4 color;
            glm::vec2 size;
            float ageNormalized;
            float lifespanNormalized;
        };

        // Particle properties structure
        struct ParticleProperties
        {
            // Spawning mode and rates
            SpawnMode spawnMode{SpawnMode::Continuous};
            float spawnRate = 5.0f;
            float spawnRateRandom = 5.0f;
            float spawnRateMin = 1.0f;
            float spawnRateMax = 10.0f;

            // Burst amounts
            int burstCount = 5;
            int burstCountRandom = 5;
            int burstCountMin = 1;
            int burstCountMax = 10;
            bool burstDone = false;
            
            // Position
            PositionMode positionMode{PositionMode::RandomSphere};
            glm::vec3 position{0.0f};
            glm::vec3 positionMin{-1.0f};
            glm::vec3 positionMax{1.0f};
            float spawnRadius = 1.0f;

            // Velocity
            VelocityMode velocityMode{VelocityMode::RandomMinMax};
            glm::vec3 velocity{0.0f};
            glm::vec3 velocityMin{-1.0f};
            glm::vec3 velocityMax{1.0f};
            float damping = 0.0f;

            // Color
            ColorMode colorMode{ColorMode::Constant};
            glm::vec3 color{1.0f};
            glm::vec3 colorMin{0.0f};
            glm::vec3 colorMax{1.0f};
            glm::vec3 colorA{0.0f};
            glm::vec3 colorB{1.0f};
            glm::vec3 startColor{1.0f};
            glm::vec3 endColor{0.0f};

            // Size
            SizeMode sizeMode{SizeMode::Constant};
            glm::vec2 size{1.0f};
            glm::vec2 sizeMin{0.5f};
            glm::vec2 sizeMax{2.0f};
            glm::vec2 startSize{1.0f};
            glm::vec2 endSize{2.0f};

            // Opacity
            OpacityMode opacityMode{OpacityMode::RandomMinMax};
            float opacity = 1.0f;
            float opacityMin = 0.1f;
            float opacityMax = 1.0f;
            float startOpacity = 1.0f;
            float endOpacity = 0.0f;

            // Lifespan
            LifespanMode lifespanMode{LifespanMode::Constant};
            float lifespan = 5.0f;
            float lifespanMin = 1.0f;
            float lifespanMax = 10.0f;
        };

        // Affector properties structure
        struct AffectorProperties
        {
            // Simple affectors
            bool addVelocity = true;
            bool gravityEnabled = false;
        };

        // Attractor structure
        struct Attractor
        {
            // Constructors
            Attractor() {};
            Attractor(const glm::vec3& position, float radius, float strength, bool relative)
                : position(position), radius(radius), strength(strength), relativeToTransform(relative) {};
            
            ~Attractor() {};

            // Data
            glm::vec3 position{0.0f};
            float radius = 5.0f;
            float strength = 25.0f;
            bool relativeToTransform = false;
        };

        // Interface
        public:

            // Creates an emitter with default values
            CPUParticleEmitter();
            
            // Loads an emitter from a YAML file
            // Accepts local paths like data:// and user://
            CPUParticleEmitter(const std::string& path);

            // Loads an emitter from a YAML node containing the emitter data
            CPUParticleEmitter(const YAML::Node& node);

            ~CPUParticleEmitter();

            // Delete copy constructor/assignment
            CPUParticleEmitter(const CPUParticleEmitter&) = delete;
            CPUParticleEmitter& operator=(const CPUParticleEmitter&) = delete;

            // Safe move constructor
            CPUParticleEmitter(CPUParticleEmitter&& other);

            // Safe move assignment operator
            CPUParticleEmitter& operator=(CPUParticleEmitter&& other);

            // Simulation

            // Simulates all active particles
            void Update(float delta, bool updateSpawns = true, bool spawnRelative = false, const glm::mat4& transform = glm::mat4(1.0f));

            // Removes all active particles and resets counters
            void Reset();

            // Rendering

            // Adds the emitter's active particles to the render queue
            // Drawn particles won't be displayed to the screen until 
            // the next call to CPUParticleEmitter::FlushRenderQueue()
            void Render(const glm::mat4& transform = glm::mat4(1.0f));

            // Flushes internal render queues and displays all particles
            static void FlushRenderQueue();

            // Serialization

            // Loads the emitter data from a YAML file
            // Accepts local paths like data:// and user://
            bool Load(const std::string& path);

            // Loads the emitter data from a YAML node
            bool Load(const YAML::Node& node);

            // Mutators / Accessors

            // Sets this emitter's offset
            void SetOffset(const glm::vec3& offset) { this->offset = offset; }

            // Sets this emitter's texture
            // Accepts local paths like data:// and user://
            void SetTexture(const std::string& path);

            // Removes any existing texture
            void RemoveTexture();

            // Gets this emitter's texture, or nullptr if empty
            Texture2D* GetTexture() const { return texture; }

            // Gives read-write access to the list of attractors
            std::vector<Attractor>& Attractors() { return attractors; }

        // Data / implementation
        private:
            
            // Render queues
            enum class RenderQueue : int
            {
                TexturedNoBlend = 0,
                UntexturedNoBlend,
                TexturedStandardBlend,
                UntexturedStandardBlend,
                TexturedAdditiveBlend,
                UntexturedAdditiveBlend,
                COUNT
            };

            // Emitter queue data format
            struct EmitterData
            {
                EmitterData(CPUParticleEmitter* emitter, const glm::mat4& transform) : emitter(emitter), transform(transform) {};
                CPUParticleEmitter* emitter = nullptr;
                glm::mat4 transform{1.0f};
            };
            
            // Emitter properties
            std::string name{"New Emitter"};
            glm::vec3 offset{0.0f};
            float duration = -1.0f;
            int maxActiveParticles = 128;
            bool randomSeed = true;

            // Rendering properties
            BlendMode blendMode{BlendMode::Additive};
            Texture2D* texture = nullptr; // NON-OWNING! (From ResourceManager)
            std::string texPath{""};

            // Particle properties and affectors
            ParticleProperties particleProperties{};
            AffectorProperties affectorProperties{};

            // Attractors
            std::vector<Attractor> attractors;

            // Particle data
            std::vector<Particle> particlePool;
            int activeParticles = 0;
            int oldest = 0;

            // Internal counters
            float totalElapsedTime = 0.0f;
            float spawnAccumulator = 0.0f;

            // RNG Instance
            RNG rng{4545};

            // Static resources
            
            // Global RNG instance
            static inline RNG GLOBAL_RNG{4545};

            // Limits and constants

            // Gravitational acceleration
            static inline glm::vec3 GRAVITATIONAL_ACCELERATION = glm::vec3(0.0f, -9.81f, 0.0f);

            // Max number of particles per emitter
            static const int MAX_PARTICLES = 16'384;
            
            // TODO: Potentially retrieve this number from hardware to batch more when possible
            // The maximum number of textured emitters we can batch into a single call
            static const int MAX_TEXTURE_UNITS = 16;

            // The maximum number of emitters we can batch into a single call (textured or not)
            static const int MAX_EMITTERS = 16;

            // OpenGL resources
            static inline GPUBuffer* quadBuffer = nullptr;
            static inline Shader* texturedShader = nullptr;
            static inline Shader* untexturedShader = nullptr;
            static inline GPUBuffer* texturedIndirectBuffer = nullptr;
            static inline GPUBuffer* untexturedIndirectBuffer = nullptr;
            static inline GPUBuffer* texturedParticleBuffer = nullptr;
            static inline GPUBuffer* untexturedParticleBuffer = nullptr;
            static inline GPUBuffer* texturedEmitterBuffer = nullptr;
            static inline GPUBuffer* untexturedEmitterBuffer = nullptr;
            static inline VertexAttributes* texturedVAO = nullptr;
            static inline VertexAttributes* untexturedVAO = nullptr;

            // Render queues
            static inline std::vector<Texture2D*> queuedTextures; // TODO: Remove
            static inline std::vector<EmitterData> renderQueues[(int)RenderQueue::COUNT];

            // Reference counter for managing static resources
            static inline size_t refCount = 0;
            static inline size_t queuedParticles = 0;
            static inline int queuedEmitters = 0;

            // Unit billboarded quad verts
            static inline GLfloat quadData[] =
            {
                -0.5f, 0.5, 0.0f,
                -0.5f, -0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                0.5f, 0.5f, 0.0f,
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f
            };

            // Reference counting helpers
            static void IncreaseReferences();

            // Friends

            // Necessary for effects to edit our properties
            friend class CPUParticleEffect;
            
            // Necessary for the particle effect editor to work
            friend class ::ParticleEffectEditor;
    };
}