#include "cpu_particle_emitter.hpp"

#include <iostream>
#include <chrono>
#include <glm/gtx/norm.hpp>

#include <phi/core/logging.hpp>

namespace Phi
{
    void CPUParticleEmitter::IncreaseReferences()
    {
        // Manage static resources
        if (refCount == 0)
        {

            // Load shaders
            untexturedShader = new Shader();
            untexturedShader->LoadShaderSource(GL_VERTEX_SHADER, "phi/graphics/shaders/untextured_particle_emitter.vs");
            untexturedShader->LoadShaderSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/untextured_particle_emitter.fs");
            untexturedShader->Link();

            texturedShader = new Shader();
            texturedShader->LoadShaderSource(GL_VERTEX_SHADER, "phi/graphics/shaders/textured_particle_emitter.vs");
            texturedShader->LoadShaderSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/textured_particle_emitter.fs");
            texturedShader->Link();

            // Create buffers
            quadBuffer = new GPUBuffer(BufferType::Static, sizeof(quadData), quadData);
            texturedIndirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawArraysCommand) * MAX_EMITTERS);
            untexturedIndirectBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(DrawArraysCommand) * MAX_EMITTERS);
            texturedParticleBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(Particle) * MAX_PARTICLES * MAX_EMITTERS);
            untexturedParticleBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(Particle) * MAX_PARTICLES * MAX_EMITTERS);
            texturedEmitterBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, (sizeof(glm::mat4) + sizeof(glm::vec4)) * MAX_EMITTERS);
            untexturedEmitterBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(glm::mat4) * MAX_EMITTERS);

            // Create vertex attributes objects
            texturedVAO = new VertexAttributes(VertexFormat::POS, quadBuffer);
            texturedVAO->Bind();
            texturedParticleBuffer->Bind(GL_ARRAY_BUFFER);
            texturedVAO->AddAttribute(3, GL_FLOAT, 1, sizeof(Particle), 0);
            texturedVAO->AddAttribute(4, GL_FLOAT, 1, sizeof(Particle), offsetof(Particle, color));
            texturedVAO->AddAttribute(2, GL_FLOAT, 1, sizeof(Particle), offsetof(Particle, size));

            untexturedVAO = new VertexAttributes(VertexFormat::POS, quadBuffer);
            untexturedVAO->Bind();
            untexturedParticleBuffer->Bind(GL_ARRAY_BUFFER);
            untexturedVAO->AddAttribute(3, GL_FLOAT, 1, sizeof(Particle), 0);
            untexturedVAO->AddAttribute(4, GL_FLOAT, 1, sizeof(Particle), offsetof(Particle, color));
            untexturedVAO->AddAttribute(2, GL_FLOAT, 1, sizeof(Particle), offsetof(Particle, size));

            // Debug logging
            Log("CPUParticleEmitter resources initialized");
        }

        // Update static reference counter
        refCount++;
    }

    CPUParticleEmitter::CPUParticleEmitter()
    {
        // Reference counter update
        IncreaseReferences();

        // Set random seed if necessary
        if (randomSeed)
        {
            rng.SetSeed(GLOBAL_RNG.NextInt(INT32_MIN, INT32_MAX));
        }

        // Initialize particle pool
        particlePool.resize(maxActiveParticles);
    }

    CPUParticleEmitter::CPUParticleEmitter(const std::string& filePath)
    {
        // Reference counter update
        IncreaseReferences();

        if (!Load(filePath))
        {
            Error("Invalid Emitter File: ", filePath);
        }
    }

    CPUParticleEmitter::CPUParticleEmitter(const YAML::Node& node)
    {
        // Reference counter update
        IncreaseReferences();

        if (!Load(node))
        {
            Error("Invalid Emitter Node: ", node);
        }
    }

    CPUParticleEmitter::CPUParticleEmitter(CPUParticleEmitter&& other)
    {
        // Needed so that the moved-from object doesn't remove static resources on destruction
        refCount++;

        // Steal all resources!
        name = std::move(other.name);
        randomSeed = std::move(other.randomSeed);
        texPath = std::move(other.texPath);
        texture = std::move(other.texture);
        blendMode = std::move(other.blendMode);
        duration = std::move(other.duration);
        maxActiveParticles = std::move(other.maxActiveParticles);
        particleProperties = std::move(other.particleProperties);
        affectorProperties = std::move(other.affectorProperties);
        attractors = std::move(other.attractors);
        particlePool = std::move(other.particlePool);
        activeParticles = std::move(other.activeParticles);
        oldest = std::move(other.oldest);
        totalElapsedTime = std::move(other.totalElapsedTime);
        spawnAccumulator = std::move(other.spawnAccumulator);
        rng = std::move(other.rng);
        offset = std::move(other.offset);
    }

    CPUParticleEmitter& CPUParticleEmitter::operator=(CPUParticleEmitter&& other)
    {
        // Steal all resources!
        name = std::move(other.name);
        randomSeed = std::move(other.randomSeed);
        texPath = std::move(other.texPath);
        texture = std::move(other.texture);
        blendMode = std::move(other.blendMode);
        duration = std::move(other.duration);
        maxActiveParticles = std::move(other.maxActiveParticles);
        particleProperties = std::move(other.particleProperties);
        affectorProperties = std::move(other.affectorProperties);
        attractors = std::move(other.attractors);
        particlePool = std::move(other.particlePool);
        activeParticles = std::move(other.activeParticles);
        oldest = std::move(other.oldest);
        totalElapsedTime = std::move(other.totalElapsedTime);
        spawnAccumulator = std::move(other.spawnAccumulator);
        rng = std::move(other.rng);
        offset = std::move(other.offset);

        // Return self for chaining
        return *this;
    }

    CPUParticleEmitter::~CPUParticleEmitter()
    {
        // Decrease reference counter
        if (refCount > 0) refCount--;

        // Cleanup static resources
        if (refCount <= 0)
        {
            delete quadBuffer;
            delete texturedShader;
            delete untexturedShader;
            delete texturedIndirectBuffer;
            delete untexturedIndirectBuffer;
            delete texturedParticleBuffer;
            delete untexturedParticleBuffer;
            delete texturedEmitterBuffer;
            delete untexturedEmitterBuffer;
            delete texturedVAO;
            delete untexturedVAO;

            // Debug logging
            Log("CPUParticleEmitter resources destroyed");
        }
    }

    void CPUParticleEmitter::Update(float delta, bool updateSpawns, const glm::mat4& transform)
    {
        // Only update counters if updating spawns and below duration or infinite duration
        if (updateSpawns)
        {
            // Update global time
            totalElapsedTime += delta;

            int numSpawns = 0;
            if (totalElapsedTime < duration || duration < 0)
            {
                // Calculate the number of spawns for this update
                switch (particleProperties.spawnMode)
                {
                    case SpawnMode::Continuous:
                        spawnAccumulator += delta * particleProperties.spawnRate;
                        numSpawns = (int)spawnAccumulator;
                        spawnAccumulator -= numSpawns;
                        break;
                    
                    case SpawnMode::Random:
                        spawnAccumulator += delta * particleProperties.spawnRateRandom;
                        numSpawns = (int)spawnAccumulator;
                        spawnAccumulator -= numSpawns;
                        if (numSpawns > 0)
                        {
                            // Calculate next spawnRate randomly
                            particleProperties.spawnRateRandom = rng.NextFloat(particleProperties.spawnRateMin, particleProperties.spawnRateMax);
                        }
                        break;
                    
                    case SpawnMode::ContinuousBurst:
                        spawnAccumulator += delta * particleProperties.spawnRate;
                        numSpawns = (int)spawnAccumulator * particleProperties.burstCount;
                        spawnAccumulator -= (int)spawnAccumulator;
                        break;
                    
                    case SpawnMode::RandomBurst:
                        spawnAccumulator += delta * particleProperties.spawnRateRandom;
                        numSpawns = (int)spawnAccumulator * particleProperties.burstCountRandom;
                        spawnAccumulator -= (int)spawnAccumulator;
                        if (numSpawns > 0)
                        {
                            // Calculate next spawnRate randomly
                            particleProperties.spawnRateRandom = rng.NextFloat(particleProperties.spawnRateMin, particleProperties.spawnRateMax);
                            particleProperties.burstCountRandom = rng.NextInt(particleProperties.burstCountMin, particleProperties.burstCountMax);
                        }
                        break;
                    
                    case SpawnMode::SingleBurst:
                        if (!particleProperties.burstDone)
                        {
                            // Set burst amount
                            numSpawns = particleProperties.burstCount;

                            // Update flag
                            particleProperties.burstDone = true;
                        }
                        break;
                }
            }

            // Spawn new particles
            for (int i = 0; i < numSpawns; ++i)
            {
                // Calculate the index of the particle to spawn
                int nextParticle = oldest;
                if (activeParticles < maxActiveParticles)
                {
                    // Use the next available particle if we can
                    nextParticle = activeParticles;

                    // Increase counter only if not recycling
                    activeParticles++;
                }

                // Ensure oldest is always valid when recycling
                if (nextParticle == oldest)
                {
                    if (oldest < activeParticles - 1)
                    {
                        oldest = particlePool[0].ageNormalized > particlePool[oldest + 1].ageNormalized ? 0 : oldest + 1;
                    }
                    else
                    {
                        oldest = 0;
                    }
                }

                // Grab the next particle
                Particle& spawned = particlePool[nextParticle];

                // Initialize properties
                spawned.ageNormalized = 0.0f;

                // Position
                switch (particleProperties.positionMode)
                {
                    case PositionMode::Constant:
                        spawned.position = particleProperties.position;
                        break;
                    
                    case PositionMode::RandomMinMax:
                        spawned.position = rng.RandomPosition(particleProperties.positionMin, particleProperties.positionMax);
                        break;
                    
                    case PositionMode::RandomSphere:
                        spawned.position = rng.RandomDirection() * rng.NextFloat(0.0f, 1.0f) * particleProperties.spawnRadius + particleProperties.position;
                        break;
                }

                // Transform position if requested
                if (transform != glm::mat4(1.0f))
                {
                    spawned.position = glm::vec3(transform * glm::vec4(spawned.position, 1.0f)) + offset;
                }
                else
                {
                    spawned.position += offset;
                }

                // Velocity
                switch (particleProperties.velocityMode)
                {
                    case VelocityMode::Constant:
                        spawned.velocity = particleProperties.velocity;
                        break;
                    
                    case VelocityMode::RandomMinMax:
                        spawned.velocity.x = rng.NextFloat(particleProperties.velocityMin.x, particleProperties.velocityMax.x);
                        spawned.velocity.y = rng.NextFloat(particleProperties.velocityMin.y, particleProperties.velocityMax.y);
                        spawned.velocity.z = rng.NextFloat(particleProperties.velocityMin.z, particleProperties.velocityMax.z);
                        break;
                }

                // Color
                switch (particleProperties.colorMode)
                {
                    case ColorMode::Constant:
                        spawned.color.r = particleProperties.color.r;
                        spawned.color.g = particleProperties.color.g;
                        spawned.color.b = particleProperties.color.b;
                        break;
                    
                    case ColorMode::RandomMinMax:
                        spawned.color.r = rng.NextFloat(particleProperties.colorMin.r, particleProperties.colorMax.r);
                        spawned.color.g = rng.NextFloat(particleProperties.colorMin.g, particleProperties.colorMax.g);
                        spawned.color.b = rng.NextFloat(particleProperties.colorMin.b, particleProperties.colorMax.b);
                        break;
                    
                    case ColorMode::RandomLerp:
                        glm::vec3 interpolated = glm::mix(particleProperties.colorA, particleProperties.colorB, rng.NextFloat(0.0f, 1.0f));
                        spawned.color.r = interpolated.r;
                        spawned.color.g = interpolated.g;
                        spawned.color.b = interpolated.b;
                        break;
                }

                // Size
                switch (particleProperties.sizeMode)
                {
                    case SizeMode::Constant:
                        spawned.size = particleProperties.size;
                        break;
                    
                    case SizeMode::RandomMinMax:
                        spawned.size.x = rng.NextFloat(particleProperties.sizeMin.x, particleProperties.sizeMax.x);
                        spawned.size.y = rng.NextFloat(particleProperties.sizeMin.y, particleProperties.sizeMax.y);
                        break;
                    
                    case SizeMode::RandomLerp:
                        spawned.size = glm::mix(particleProperties.sizeMin, particleProperties.sizeMax, rng.NextFloat(0.0f, 1.0f));
                        break;
                }

                // Opacity
                switch (particleProperties.opacityMode)
                {
                    case OpacityMode::Constant:
                        spawned.color.a = particleProperties.opacity;
                        break;
                    
                    case OpacityMode::RandomMinMax:
                        spawned.color.a = rng.NextFloat(particleProperties.opacityMin, particleProperties.opacityMax);
                        break;
                }
                
                // Lifetime
                switch (particleProperties.lifespanMode)
                {
                    case LifespanMode::Constant:
                        spawned.lifespanNormalized = 1 / particleProperties.lifespan;
                        break;
                    
                    case LifespanMode::RandomMinMax:
                        spawned.lifespanNormalized = 1 / rng.NextFloat(particleProperties.lifespanMin, particleProperties.lifespanMax);
                        break;
                }
            }
        }

        // Simulate all particles
        for (int i = 0; i < activeParticles; ++i)
        {
            // Grab the next active particle
            Particle& particle = particlePool[i];

            // Age particle
            particle.ageNormalized += delta * particle.lifespanNormalized;

            // Check for particles that should die
            if (particle.ageNormalized > 1.0f)
            {
                // Replace with last active particle
                std::swap(particle, particlePool[activeParticles - 1]);

                // Decrease counter
                activeParticles--;

                // Validate oldest
                // TODO: This is taking not the oldest particle sometimes. Switching to linked lists
                // is not worth the performance trade-off, so another solution must be found
                if (i == oldest)
                {
                    if (oldest < activeParticles - 1)
                    {
                        oldest = particlePool[0].ageNormalized > particlePool[oldest + 1].ageNormalized ? 0 : oldest + 1;
                    }
                    else
                    {
                        oldest = 0;
                    }
                }

                // Ensure we process the one we just swapped
                i--;
                continue;
            }

            // Over lifetime effects

            // Calculate next color
            if (particleProperties.colorMode == ColorMode::LerpOverLifetime)
            {
                glm::vec3 newColor = glm::mix(particleProperties.startColor, particleProperties.endColor, particle.ageNormalized);
                particle.color.r = newColor.r;
                particle.color.g = newColor.g;
                particle.color.b = newColor.b;
            }

            // Calculate next size
            if (particleProperties.sizeMode == SizeMode::LerpOverLifetime)
            {
                glm::vec2 newSize = glm::mix(particleProperties.startSize, particleProperties.endSize, particle.ageNormalized);
                particle.size.x = newSize.x;
                particle.size.y = newSize.y;
            }

            // Calculate next opacity value
            if (particleProperties.opacityMode == OpacityMode::LerpOverLifetime)
            {
                particle.color.a = glm::mix(particleProperties.startOpacity, particleProperties.endOpacity, particle.ageNormalized);
            }

            // Affectors

            // Add velocity
            if (affectorProperties.addVelocity) particle.position += particle.velocity * delta;

            // Apply gravity
            if (affectorProperties.gravityEnabled) particle.velocity += GRAVITATIONAL_ACCELERATION * delta;

            // Apply attractors
            for (int i = 0; i < attractors.size(); ++i)
            {
                // Grab the attractor
                const auto& attractor = attractors[i];

                // Get vector from particle to attractor, depending on relative setting
                glm::vec3 particleToAttractor;
                if (attractor.relativeToTransform)
                {
                    particleToAttractor = glm::vec3(transform * glm::vec4(attractor.position, 1.0f)) - particle.position;
                }
                else
                {
                    particleToAttractor = attractor.position - particle.position;
                }

                // Calculate effect on velocity
                float distance = glm::length(particleToAttractor);
                glm::vec3 n = glm::normalize(particleToAttractor) * glm::abs(attractor.strength);
                if (distance < attractor.radius)
                {
                    particle.velocity += glm::mix(attractor.strength >= 0 ? n : -n, glm::vec3(0.0f), distance / attractor.radius) * delta;
                }
            }

            // Apply damping
            if (particleProperties.damping > 0.0f)
            {
                particle.velocity *= 1 - (particleProperties.damping * delta);
            }
        }
    }

    void CPUParticleEmitter::Reset()
    {
        // Reset all counters and timers
        activeParticles = 0;
        totalElapsedTime = 0.0f;
        spawnAccumulator = 0.0f;
        particleProperties.burstDone = false;

        // Reset the seed
        if (randomSeed)
        {
            rng.SetSeed(GLOBAL_RNG.NextInt(INT32_MIN, INT32_MAX));
        }
        else
        {
            rng.Reseed();
        }

        // Reset to default random values for this seed
        particleProperties.spawnRateRandom = rng.NextFloat(particleProperties.spawnRateMin, particleProperties.spawnRateMax);
        particleProperties.burstCountRandom = rng.NextInt(particleProperties.burstCountMin, particleProperties.burstCountMax);
    }

    void CPUParticleEmitter::Render(const glm::mat4& transform)
    {
        // Early out if unnecessary
        if (activeParticles < 1) return;
        
        // Increase counter
        queuedEmitters++;

        // Add to correct queue
        if (texture)
        {
            switch (blendMode)
            {
                case BlendMode::None:
                    renderQueues[(int)RenderQueue::TexturedNoBlend].push_back(EmitterData(this, transform));
                    break;
                
                case BlendMode::Additive:
                    renderQueues[(int)RenderQueue::TexturedAdditiveBlend].push_back(EmitterData(this, transform));
                    break;
                
                case BlendMode::Standard:
                    renderQueues[(int)RenderQueue::TexturedStandardBlend].push_back(EmitterData(this, transform));
                    break;
            }
        }
        else
        {
            switch (blendMode)
            {
                case BlendMode::None:
                    renderQueues[(int)RenderQueue::UntexturedNoBlend].push_back(EmitterData(this, transform));
                    break;
                
                case BlendMode::Additive:
                    renderQueues[(int)RenderQueue::UntexturedAdditiveBlend].push_back(EmitterData(this, transform));
                    break;
                
                case BlendMode::Standard:
                    renderQueues[(int)RenderQueue::UntexturedStandardBlend].push_back(EmitterData(this, transform));
                    break;
            }
        }
    }

    void CPUParticleEmitter::FlushRenderQueue()
    {
        // Don't issue a draw call if there's nothing to render
        if (queuedEmitters == 0) return;

        // Iterate all queues
        for (int i = 0; i < (int)RenderQueue::COUNT; ++i)
        {
            // Grab a reference to the queue and the enum value
            auto queueType = (RenderQueue)i;
            auto& queue = renderQueues[i];

            // Early out if empty
            if (queue.size() == 0) continue;

            // Counters
            int queueCount = 0;

            // Init render state

            // Non-owning resource pointers for THIS queue
            Shader* shader = nullptr;
            GPUBuffer* iBuffer = nullptr;
            GPUBuffer* pBuffer = nullptr;
            GPUBuffer* eBuffer = nullptr;
            VertexAttributes* vao = nullptr;
            switch (queueType)
            {
                case RenderQueue::TexturedNoBlend:
                    // Setup blend and depth state
                    glDisable(GL_BLEND);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_TRUE);
                    
                    // Choose resources
                    shader = texturedShader;
                    iBuffer = texturedIndirectBuffer;
                    pBuffer = texturedParticleBuffer;
                    eBuffer = texturedEmitterBuffer;
                    vao = texturedVAO;
                    break;
                
                case RenderQueue::TexturedAdditiveBlend:
                    // Setup blend and depth state
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_FALSE);

                    // Choose resources
                    shader = texturedShader;
                    iBuffer = texturedIndirectBuffer;
                    pBuffer = texturedParticleBuffer;
                    eBuffer = texturedEmitterBuffer;
                    vao = texturedVAO;
                    break;
                
                case RenderQueue::TexturedStandardBlend:
                    // Setup blend and depth state
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_FALSE);

                    // Choose resources
                    shader = texturedShader;
                    iBuffer = texturedIndirectBuffer;
                    pBuffer = texturedParticleBuffer;
                    eBuffer = texturedEmitterBuffer;
                    vao = texturedVAO;
                    break;
                
                case RenderQueue::UntexturedNoBlend:
                    // Setup blend and depth state
                    glDisable(GL_BLEND);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_TRUE);

                    // Choose resources
                    shader = untexturedShader;
                    iBuffer = untexturedIndirectBuffer;
                    pBuffer = untexturedParticleBuffer;
                    eBuffer = untexturedEmitterBuffer;
                    vao = untexturedVAO;
                    break;
                
                case RenderQueue::UntexturedAdditiveBlend:
                    // Setup blend and depth state
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_ONE, GL_ONE);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_FALSE);

                    // Choose resources
                    shader = untexturedShader;
                    iBuffer = untexturedIndirectBuffer;
                    pBuffer = untexturedParticleBuffer;
                    eBuffer = untexturedEmitterBuffer;
                    vao = untexturedVAO;
                    break;
                
                case RenderQueue::UntexturedStandardBlend:
                    // Setup blend and depth state
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDepthFunc(GL_LESS);
                    glDepthMask(GL_FALSE);

                    // Choose resources
                    shader = untexturedShader;
                    iBuffer = untexturedIndirectBuffer;
                    pBuffer = untexturedParticleBuffer;
                    eBuffer = untexturedEmitterBuffer;
                    vao = untexturedVAO;
                    break;
            }

            // Render all emitters
            for (auto& emitterData : queue)
            {
                auto& emitter = emitterData.emitter;
                auto& transform = emitterData.transform;

                // Flush if we must
                if (queueCount >= MAX_EMITTERS || queuedTextures.size() >= MAX_TEXTURE_UNITS)
                {
                    // Bind resources
                    vao->Bind();
                    shader->Use();
                    iBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);
                    eBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 1, eBuffer->GetCurrentSection() * eBuffer->GetSize(), eBuffer->GetSize());

                    // Bind texture units if necessary
                    if (queuedTextures.size() > 0)
                    {
                        for (int i = 0; i < queuedTextures.size(); ++i)
                        {
                            queuedTextures[i]->Bind(i);
                        }
                        queuedTextures.clear();
                    }

                    // Issue draw call
                    glMultiDrawArraysIndirect(GL_TRIANGLES, (void*)(iBuffer->GetCurrentSection() * iBuffer->GetSize()), queueCount, sizeof(DrawArraysCommand));
                    vao->Unbind();

                    // Set a lock and reset buffers
                    iBuffer->Lock();
                    iBuffer->SwapSections();
                    pBuffer->SwapSections();
                    eBuffer->SwapSections();

                    // Reset counters
                    queuedParticles = 0;
                    queueCount = 0;
                }

                // Sync if it's the first write
                if (queueCount == 0) iBuffer->Sync();

                // Build up indirect draw command
                DrawArraysCommand cmd;
                cmd.count = 6;
                cmd.instanceCount = emitter->activeParticles;
                cmd.first = 0;
                cmd.baseInstance = queuedParticles + (pBuffer->GetCurrentSection() * MAX_PARTICLES * MAX_EMITTERS);

                // Write draw command
                iBuffer->Write(cmd);

                // Write emitter data
                eBuffer->Write(transform);
                if (eBuffer == texturedEmitterBuffer)
                {
                    // Write texture ID (padded to vec4 for alignment)
                    eBuffer->Write(glm::ivec4(queuedTextures.size()));

                    // Queue the proper texture
                    queuedTextures.push_back(emitter->texture);
                }

                // Write particles to proper buffer
                pBuffer->Write(emitter->particlePool.data(), emitter->activeParticles * sizeof(Particle));

                // Update counters
                queuedParticles += emitter->activeParticles;
                queueCount++;
            }
            
            // Final flush if necessary
            if (queueCount > 0)
            {
                // Bind resources
                vao->Bind();
                shader->Use();
                iBuffer->Bind(GL_DRAW_INDIRECT_BUFFER);
                eBuffer->BindRange(GL_SHADER_STORAGE_BUFFER, 1, eBuffer->GetCurrentSection() * eBuffer->GetSize(), eBuffer->GetSize());

                // Bind texture units if necessary
                if (queuedTextures.size() > 0)
                {
                    for (int i = 0; i < queuedTextures.size(); ++i)
                    {
                        queuedTextures[i]->Bind(i);
                    }
                    queuedTextures.clear();
                }

                // Issue draw call
                glMultiDrawArraysIndirect(GL_TRIANGLES, (void*)(iBuffer->GetCurrentSection() * iBuffer->GetSize()), queueCount, sizeof(DrawArraysCommand));
                vao->Unbind();

                // Set a lock and reset buffers
                iBuffer->Lock();
                iBuffer->SwapSections();
                pBuffer->SwapSections();
                eBuffer->SwapSections();

                // Reset counters
                queuedParticles = 0;
                queueCount = 0;
            }

            // Clear the queue after rendering
            queue.clear();
        }
    }

    bool CPUParticleEmitter::Load(const std::string& filePath)
    {
        // YAML parsing library throws exceptions... catch and handle
        try
        {
            // Load the file using yaml-cpp
            YAML::Node emitter = YAML::LoadFile(filePath);

            // Check validity
            if (!emitter) return false;
            
            // Load via the YAML node method
            return Load(emitter);
        }

        catch (YAML::Exception& e)
        {
            Error("YAML parser exception: ", filePath, ": ", e.msg);
            Reset();
            return false;
        }
    }

    bool CPUParticleEmitter::Load(const YAML::Node& node)
    {
        try
        {
            // Parse fields in order
            name = node["emitter_name"] ? node["emitter_name"].as<std::string>() : name;

            // Seed
            if (node["seed"])
            {
                // This is terrible, maybe time for a different parser...
                try
                {
                    // Attempt to convert to int
                    int seed = node["seed"].as<int>();

                    // These lines will only run if the conversion is successful (seed is explicit)
                    rng.SetSeed(seed);
                    randomSeed = false;
                }
                catch (YAML::BadConversion& e)
                {
                    // See if seed is meant to be random
                    std::string v = node["seed"].as<std::string>();
                    if (v == "random")
                    {
                        rng.SetSeed(GLOBAL_RNG.NextInt(INT32_MIN, INT32_MAX));
                    }
                }
            }

            // Set the offset if given
            YAML::Node offsetNode = node["offset"];
            if (offsetNode)
            {
                offset.x = offsetNode["x"] ? offsetNode["x"].as<float>() : offset.x;
                offset.y = offsetNode["y"] ? offsetNode["y"].as<float>() : offset.y;
                offset.z = offsetNode["z"] ? offsetNode["z"].as<float>() : offset.z;
            }
            
            if (node["texture"])
            {
                texPath = node["texture"].as<std::string>();
                texture = ResourceManager::Instance().LoadTexture2D(texPath);
            }

            YAML::Node blendModeNode = node["blend_mode"];
            if (blendModeNode)
            {
                std::string bs = blendModeNode.as<std::string>();
                if (bs == "none")
                {
                    blendMode = BlendMode::None;
                }
                else if (bs == "additive")
                {
                    blendMode = BlendMode::Additive;
                }
                else if (bs == "standard")
                {
                    blendMode = BlendMode::Standard;
                }
            }

            YAML::Node spawnTypeNode = node["spawn_mode"];
            if (spawnTypeNode)
            {
                std::string spawnTypeString = spawnTypeNode.as<std::string>();
                if (spawnTypeString == "continuous")
                {
                    particleProperties.spawnMode = SpawnMode::Continuous;
                    
                }
                else if (spawnTypeString == "continuous_burst")
                {
                    particleProperties.spawnMode = SpawnMode::ContinuousBurst;
                }
                else if (spawnTypeString == "random")
                {
                    particleProperties.spawnMode = SpawnMode::Random;

                }
                else if (spawnTypeString == "random_burst")
                {
                    particleProperties.spawnMode = SpawnMode::RandomBurst;
                }
                else if (spawnTypeString == "single_burst")
                {
                    particleProperties.spawnMode = SpawnMode::SingleBurst;
                }
            }

            particleProperties.spawnRate = node["spawn_rate"] ? node["spawn_rate"].as<float>() : particleProperties.spawnRate;
            particleProperties.spawnRateMin = node["spawn_rate_min"] ? node["spawn_rate_min"].as<float>() : particleProperties.spawnRateMin;
            particleProperties.spawnRateMax = node["spawn_rate_max"] ? node["spawn_rate_max"].as<float>() : particleProperties.spawnRateMax;
            particleProperties.spawnRateRandom = rng.NextFloat(particleProperties.spawnRateMin, particleProperties.spawnRateMax);
        
            particleProperties.burstCount = node["burst_count"] ? node["burst_count"].as<int>() : particleProperties.burstCount;
            particleProperties.burstCountMin = node["burst_count_min"] ? node["burst_count_min"].as<int>() : particleProperties.burstCountMin;
            particleProperties.burstCountMax = node["burst_count_max"] ? node["burst_count_max"].as<int>() : particleProperties.burstCountMax;
            particleProperties.burstCountRandom = rng.NextInt(particleProperties.burstCountMin, particleProperties.burstCountMax);

            duration = node["duration"] ? node["duration"].as<float>() : duration;
            maxActiveParticles = node["max_particles"] ? node["max_particles"].as<int>() : maxActiveParticles;

            YAML::Node props = node["particle_properties"];
            if (props)
            {
                // Position
                YAML::Node posProps = props["position"];
                if (posProps && posProps["type"])
                {
                    std::string posType = posProps["type"].as<std::string>();
                    if (posType == "constant")
                    {
                        particleProperties.positionMode = PositionMode::Constant;
                        particleProperties.position.x = posProps["value"]["x"] ? posProps["value"]["x"].as<float>() : particleProperties.position.x;
                        particleProperties.position.y = posProps["value"]["y"] ? posProps["value"]["y"].as<float>() : particleProperties.position.y;
                        particleProperties.position.z = posProps["value"]["z"] ? posProps["value"]["z"].as<float>() : particleProperties.position.z;
                    }
                    else if (posType == "random_min_max")
                    {
                        particleProperties.positionMode = PositionMode::RandomMinMax;
                        particleProperties.positionMin.x = posProps["min"]["x"] ? posProps["min"]["x"].as<float>() : particleProperties.positionMin.x;
                        particleProperties.positionMin.y = posProps["min"]["y"] ? posProps["min"]["y"].as<float>() : particleProperties.positionMin.y;
                        particleProperties.positionMin.z = posProps["min"]["z"] ? posProps["min"]["z"].as<float>() : particleProperties.positionMin.z;
                        particleProperties.positionMax.x = posProps["max"]["x"] ? posProps["max"]["x"].as<float>() : particleProperties.positionMax.x;
                        particleProperties.positionMax.y = posProps["max"]["y"] ? posProps["max"]["y"].as<float>() : particleProperties.positionMax.y;
                        particleProperties.positionMax.z = posProps["max"]["z"] ? posProps["max"]["z"].as<float>() : particleProperties.positionMax.z;
                    }
                    else if (posType == "random_sphere")
                    {
                        particleProperties.positionMode = PositionMode::RandomSphere;
                        particleProperties.position.x = posProps["center"]["x"] ? posProps["center"]["x"].as<float>() : particleProperties.position.x;
                        particleProperties.position.y = posProps["center"]["y"] ? posProps["center"]["y"].as<float>() : particleProperties.position.y;
                        particleProperties.position.z = posProps["center"]["z"] ? posProps["center"]["z"].as<float>() : particleProperties.position.z;
                        particleProperties.spawnRadius = posProps["radius"] ? posProps["radius"].as<float>() : particleProperties.spawnRadius;
                    }
                }

                // Velocity
                YAML::Node velProps = props["velocity"];
                if (velProps && velProps["type"])
                {
                    std::string velType = velProps["type"].as<std::string>();
                    if (velType == "constant")
                    {
                        particleProperties.velocityMode = VelocityMode::Constant;
                        particleProperties.velocity.x = velProps["value"]["x"] ? velProps["value"]["x"].as<float>() : particleProperties.velocity.x;
                        particleProperties.velocity.y = velProps["value"]["y"] ? velProps["value"]["y"].as<float>() : particleProperties.velocity.y;
                        particleProperties.velocity.z = velProps["value"]["z"] ? velProps["value"]["z"].as<float>() : particleProperties.velocity.z;
                    }
                    else if (velType == "random_min_max")
                    {
                        particleProperties.velocityMode = VelocityMode::RandomMinMax;
                        particleProperties.velocityMin.x = velProps["min"]["x"] ? velProps["min"]["x"].as<float>() : particleProperties.velocityMin.x;
                        particleProperties.velocityMin.y = velProps["min"]["y"] ? velProps["min"]["y"].as<float>() : particleProperties.velocityMin.y;
                        particleProperties.velocityMin.z = velProps["min"]["z"] ? velProps["min"]["z"].as<float>() : particleProperties.velocityMin.z;
                        particleProperties.velocityMax.x = velProps["max"]["x"] ? velProps["max"]["x"].as<float>() : particleProperties.velocityMax.x;
                        particleProperties.velocityMax.y = velProps["max"]["y"] ? velProps["max"]["y"].as<float>() : particleProperties.velocityMax.y;
                        particleProperties.velocityMax.z = velProps["max"]["z"] ? velProps["max"]["z"].as<float>() : particleProperties.velocityMax.z;
                    }

                    // Parse damping
                    particleProperties.damping = velProps["damping"] ? velProps["damping"].as<float>() : particleProperties.damping;
                }

                // Color
                YAML::Node colorProps = props["color"];
                if (colorProps && colorProps["type"])
                {
                    std::string colType = colorProps["type"].as<std::string>();
                    if (colType == "constant")
                    {
                        // Set the type
                        particleProperties.colorMode = ColorMode::Constant;

                        // Load the value
                        if (colorProps["value"])
                        {
                            particleProperties.color.r = colorProps["value"]["r"] ? colorProps["value"]["r"].as<float>() : particleProperties.color.r;
                            particleProperties.color.g = colorProps["value"]["g"] ? colorProps["value"]["g"].as<float>() : particleProperties.color.g;
                            particleProperties.color.b = colorProps["value"]["b"] ? colorProps["value"]["b"].as<float>() : particleProperties.color.b;
                        }
                    }
                    else if (colType == "random_min_max")
                    {
                        // Set the type
                        particleProperties.colorMode = ColorMode::RandomMinMax;

                        // Load min and max
                        if (colorProps["min"])
                        {
                            particleProperties.colorMin.r = colorProps["min"]["r"] ? colorProps["min"]["r"].as<float>() : particleProperties.colorMin.r;
                            particleProperties.colorMin.g = colorProps["min"]["g"] ? colorProps["min"]["g"].as<float>() : particleProperties.colorMin.g;
                            particleProperties.colorMin.b = colorProps["min"]["b"] ? colorProps["min"]["b"].as<float>() : particleProperties.colorMin.b;
                        }
                        if (colorProps["max"])
                        {
                            particleProperties.colorMax.r = colorProps["max"]["r"] ? colorProps["max"]["r"].as<float>() : particleProperties.colorMax.r;
                            particleProperties.colorMax.g = colorProps["max"]["g"] ? colorProps["max"]["g"].as<float>() : particleProperties.colorMax.g;
                            particleProperties.colorMax.b = colorProps["max"]["b"] ? colorProps["max"]["b"].as<float>() : particleProperties.colorMax.b;
                        }
                    }
                    else if (colType == "random_lerp")
                    {
                        // Set the type
                        particleProperties.colorMode = ColorMode::RandomLerp;

                        // Load colors A and B
                        if (colorProps["color_a"])
                        {
                            particleProperties.colorA.r = colorProps["color_a"]["r"] ? colorProps["color_a"]["r"].as<float>() : particleProperties.colorA.r;
                            particleProperties.colorA.g = colorProps["color_a"]["g"] ? colorProps["color_a"]["g"].as<float>() : particleProperties.colorA.g;
                            particleProperties.colorA.b = colorProps["color_a"]["b"] ? colorProps["color_a"]["b"].as<float>() : particleProperties.colorA.b;
                        }
                        if (colorProps["color_b"])
                        {
                            particleProperties.colorB.r = colorProps["color_b"]["r"] ? colorProps["color_b"]["r"].as<float>() : particleProperties.colorB.r;
                            particleProperties.colorB.g = colorProps["color_b"]["g"] ? colorProps["color_b"]["g"].as<float>() : particleProperties.colorB.g;
                            particleProperties.colorB.b = colorProps["color_b"]["b"] ? colorProps["color_b"]["b"].as<float>() : particleProperties.colorB.b;
                        }
                    }
                    else if (colType == "lerp_over_lifetime")
                    {
                        particleProperties.colorMode = ColorMode::LerpOverLifetime;

                        // Load start and end colors
                        if (colorProps["start_color"])
                        {
                            particleProperties.startColor.r = colorProps["start_color"]["r"] ? colorProps["start_color"]["r"].as<float>() : particleProperties.startColor.r;
                            particleProperties.startColor.g = colorProps["start_color"]["g"] ? colorProps["start_color"]["g"].as<float>() : particleProperties.startColor.g;
                            particleProperties.startColor.b = colorProps["start_color"]["b"] ? colorProps["start_color"]["b"].as<float>() : particleProperties.startColor.b;
                        }
                        if (colorProps["end_color"])
                        {
                            particleProperties.endColor.r = colorProps["end_color"]["r"] ? colorProps["end_color"]["r"].as<float>() : particleProperties.endColor.r;
                            particleProperties.endColor.g = colorProps["end_color"]["g"] ? colorProps["end_color"]["g"].as<float>() : particleProperties.endColor.g;
                            particleProperties.endColor.b = colorProps["end_color"]["b"] ? colorProps["end_color"]["b"].as<float>() : particleProperties.endColor.b;
                        }
                    }
                }

                // Size
                YAML::Node sizeProps = props["size"];
                if (sizeProps && sizeProps["type"])
                {
                    std::string sizeType = sizeProps["type"].as<std::string>();
                    if (sizeType == "constant")
                    {
                        particleProperties.sizeMode = SizeMode::Constant;
                        particleProperties.size.x = sizeProps["value"]["x"] ? sizeProps["value"]["x"].as<float>() : particleProperties.size.x;
                        particleProperties.size.y = sizeProps["value"]["y"] ? sizeProps["value"]["y"].as<float>() : particleProperties.size.y;
                    }
                    else if (sizeType == "random_min_max")
                    {
                        particleProperties.sizeMode = SizeMode::RandomMinMax;
                        particleProperties.sizeMin.x = sizeProps["min"]["x"] ? sizeProps["min"]["x"].as<float>() : particleProperties.sizeMin.x;
                        particleProperties.sizeMin.y = sizeProps["min"]["y"] ? sizeProps["min"]["y"].as<float>() : particleProperties.sizeMin.y;
                        particleProperties.sizeMax.x = sizeProps["max"]["x"] ? sizeProps["max"]["x"].as<float>() : particleProperties.sizeMax.x;
                        particleProperties.sizeMax.y = sizeProps["max"]["y"] ? sizeProps["max"]["y"].as<float>() : particleProperties.sizeMax.y;
                    }
                    else if (sizeType == "random_lerp")
                    {
                        particleProperties.sizeMode = SizeMode::RandomLerp;
                        particleProperties.sizeMin.x = sizeProps["min"]["x"] ? sizeProps["min"]["x"].as<float>() : particleProperties.sizeMin.x;
                        particleProperties.sizeMin.y = sizeProps["min"]["y"] ? sizeProps["min"]["y"].as<float>() : particleProperties.sizeMin.y;
                        particleProperties.sizeMax.x = sizeProps["max"]["x"] ? sizeProps["max"]["x"].as<float>() : particleProperties.sizeMax.x;
                        particleProperties.sizeMax.y = sizeProps["max"]["y"] ? sizeProps["max"]["y"].as<float>() : particleProperties.sizeMax.y;
                    }
                    else if (sizeType == "lerp_over_lifetime")
                    {
                        particleProperties.sizeMode = SizeMode::LerpOverLifetime;
                        particleProperties.startSize.x = sizeProps["start_size"]["x"] ? sizeProps["start_size"]["x"].as<float>() : particleProperties.startSize.x;
                        particleProperties.startSize.y = sizeProps["start_size"]["y"] ? sizeProps["start_size"]["y"].as<float>() : particleProperties.startSize.y;
                        particleProperties.endSize.x = sizeProps["end_size"]["x"] ? sizeProps["end_size"]["x"].as<float>() : particleProperties.endSize.x;
                        particleProperties.endSize.y = sizeProps["end_size"]["y"] ? sizeProps["end_size"]["y"].as<float>() : particleProperties.endSize.y;
                    }
                }

                // Opacity
                YAML::Node opacityProps = props["opacity"];
                if (opacityProps && opacityProps["type"])
                {
                    std::string opacityType = opacityProps["type"].as<std::string>();
                    if (opacityType == "constant")
                    {
                        particleProperties.opacityMode = OpacityMode::Constant;
                        particleProperties.opacity = opacityProps["value"] ? opacityProps["value"].as<float>() : particleProperties.opacity;
                    }
                    else if (opacityType == "random_min_max")
                    {
                        particleProperties.opacityMode = OpacityMode::RandomMinMax;
                        particleProperties.opacityMin = opacityProps["min"] ? opacityProps["min"].as<float>() : particleProperties.opacityMin;
                        particleProperties.opacityMax = opacityProps["max"] ? opacityProps["max"].as<float>() : particleProperties.opacityMax;
                    }
                    else if (opacityType == "lerp_over_lifetime")
                    {
                        particleProperties.opacityMode = OpacityMode::LerpOverLifetime;
                        particleProperties.startOpacity = opacityProps["start_opacity"] ? opacityProps["start_opacity"].as<float>() : particleProperties.startOpacity;
                        particleProperties.endOpacity = opacityProps["end_opacity"] ? opacityProps["end_opacity"].as<float>() : particleProperties.endOpacity;
                    }
                }

                // Lifespan
                YAML::Node lifespanProps = props["lifespan"];
                if (lifespanProps && lifespanProps["type"])
                {
                    std::string lst = lifespanProps["type"].as<std::string>();
                    if (lst == "constant")
                    {
                        particleProperties.lifespanMode = LifespanMode::Constant;
                        particleProperties.lifespan = lifespanProps["value"] ? lifespanProps["value"].as<float>() : particleProperties.lifespan;
                    }
                    else if (lst == "random_min_max")
                    {
                        particleProperties.lifespanMode = LifespanMode::RandomMinMax;
                        particleProperties.lifespanMin = lifespanProps["min"] ? lifespanProps["min"].as<float>() : particleProperties.lifespanMin;
                        particleProperties.lifespanMax = lifespanProps["max"] ? lifespanProps["max"].as<float>() : particleProperties.lifespanMax;
                    }
                }
            }

            // Load affectors
            YAML::Node affectors = node["affectors"];
            if (affectors)
            {
                // Simple affectors
                affectorProperties.gravityEnabled = affectors["gravity"] ? affectors["gravity"].as<bool>() : affectorProperties.gravityEnabled;
                affectorProperties.addVelocity = affectors["add_velocity"] ? affectors["add_velocity"].as<bool>() : affectorProperties.addVelocity;
            }

            // Load attractors
            YAML::Node attractorsNode = node["attractors"];
            if (attractorsNode)
            {
                for (int i = 0; i < attractorsNode.size(); ++i)
                {
                    YAML::Node attractorNode = attractorsNode[i];

                    // Initialize attractor
                    Attractor a;
                    a.position.x = attractorNode["position"]["x"] ? attractorNode["position"]["x"].as<float>() : a.position.x;
                    a.position.y = attractorNode["position"]["y"] ? attractorNode["position"]["y"].as<float>() : a.position.y;
                    a.position.z = attractorNode["position"]["z"] ? attractorNode["position"]["z"].as<float>() : a.position.z;
                    a.radius = attractorNode["radius"] ? attractorNode["radius"].as<float>() : a.radius;
                    a.strength = attractorNode["strength"] ? attractorNode["strength"].as<float>() : a.strength;
                    a.relativeToTransform = attractorNode["relative"] ? attractorNode["relative"].as<bool>() : a.relativeToTransform;

                    // Add to our list
                    attractors.push_back(a);
                }
            }

            // Initialize particle pool
            particlePool.resize(maxActiveParticles);
            return true;
        }
        
        // Catch exceptions
        catch (YAML::Exception& e)
        {
            Error("YAML parser exception: ", e.msg);
            Reset();
            return false;
        }
    }

    void CPUParticleEmitter::SetTexture(const std::string& texPath)
    {
        Texture2D* newTex = ResourceManager::Instance().LoadTexture2D(texPath);
        if (newTex)
        {
            if (this->texture) ResourceManager::Instance().UnloadTexture2D(this->texPath);
            this->texPath = texPath;
            this->texture = newTex;
        }
    }

    void CPUParticleEmitter::RemoveTexture()
    {
        if (texture)
        {
            ResourceManager::Instance().UnloadTexture2D(texPath);
            texture = nullptr;
            texPath = "";
        }
    }
}