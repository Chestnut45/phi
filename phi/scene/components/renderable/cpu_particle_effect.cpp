#include "cpu_particle_effect.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>

#include <yaml-cpp/yaml.h>
#include <phi/core/file.hpp>
#include <phi/core/logging.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    CPUParticleEffect::CPUParticleEffect()
    {
    }

    CPUParticleEffect::CPUParticleEffect(const std::string& path)
    {
        if (!Load(path))
        {
            Phi::Error("Invalid Effect File: ", path);
        }
    }

    CPUParticleEffect::~CPUParticleEffect()
    {
    }

    void CPUParticleEffect::Update(float delta)
    {
        // Grab sibling transform component
        glm::mat4 transform = glm::mat4(1.0f);
        Transform* t = GetNode()->GetComponent<Transform>();
        if (t) transform = t->GetGlobalMatrix();

        switch (state)
        {
            case State::Play:

                // Update all emitters
                for (auto& emitter : loadedEmitters)
                {
                    emitter.Update(delta, true, spawnRelativeTransform, transform);
                }

                break;
            
            case State::Stopped:

                // Update all emitters
                for (auto& emitter : loadedEmitters)
                {
                    emitter.Update(delta, false, spawnRelativeTransform, transform);
                }

                break;
            
            default:
                break;
        }
    }

    void CPUParticleEffect::Render(const glm::mat4& transform)
    {
        // Render all emitters
        for (auto& emitter : loadedEmitters)
        {
            emitter.Render(renderRelativeTransform ? transform : glm::mat4(1.0f));
        }
    }

    void CPUParticleEffect::FlushRenderQueue()
    {
        CPUParticleEmitter::FlushRenderQueue();
    }

    void CPUParticleEffect::Play()
    {
        state = State::Play;
    }

    void CPUParticleEffect::Pause()
    {
        state = State::Paused;
    }

    void CPUParticleEffect::Stop()
    {
        state = State::Stopped;

        // Reset counters in all emitters
        for (auto& emitter : loadedEmitters)
        {
            emitter.particleProperties.burstDone = false;
        }
    }

    void CPUParticleEffect::Restart()
    {
        state = State::Play;

        // Reset all emitters
        for (auto& emitter : loadedEmitters)
        {
            emitter.Reset();
        }
    }

    bool CPUParticleEffect::Load(const std::string& path)
    {
        try
        {
            // Load the file using yaml-cpp
            YAML::Node effect = YAML::LoadFile(File::GlobalizePath(path));

            // Check validity
            if (!effect) return false;

            // Reset if we're to continue loading
            Reset();

            // Grab the effect name
            name = effect["effect_name"] ? effect["effect_name"].as<std::string>() : name;
            spawnRelativeTransform = effect["spawn_relative"] ? effect["spawn_relative"].as<bool>() : spawnRelativeTransform;
            renderRelativeTransform = effect["render_relative"] ? effect["render_relative"].as<bool>() : renderRelativeTransform;

            // Grab the emitters from the file
            YAML::Node emitters = effect["emitters"];
            size_t numEmitters = emitters.size();

            // Parse all emitters
            for (int i = 0; i < numEmitters; ++i)
            {
                // Load the emitter from file
                if (emitters[i]["file"])
                {
                    // Load the emitter file path
                    std::string emitterFile = emitters[i]["file"].as<std::string>();

                    // Construct the emitter in place
                    loadedEmitters.emplace_back(File::GlobalizePath(emitterFile));
                }
                else
                {
                    // Load the emitter from the YAML node directly
                    loadedEmitters.emplace_back(emitters[i]);
                }
            }

            return true;
        }

        // Catch and handle exceptions
        catch (YAML::Exception& e)
        {
            Error("YAML parser exception: ", path, ": ", e.msg);
            Reset();
            return false;
        }
    }

    void CPUParticleEffect::Save(const std::string& path, bool singleFile) const
    {
        // Grab the global path
        std::string globalPath = File::GlobalizePath(path);
        
        if (singleFile)
        {
            // Output the entire effect contents to a single file

            // TODO: Change to using Phi::File for IO as well!

            // Create the effect file stream
            std::ofstream effectStream(globalPath);

            // Name, spawnRelative and renderRelative
            effectStream << "effect_name: " << name.c_str() << "\nspawn_relative: "
                << (spawnRelativeTransform ? "true" : "false") << "\nrender_relative: "
                << (renderRelativeTransform ? "true" : "false") << "\nemitters: [\n";
            
            // Output all emitters
            for (const auto& emitter : loadedEmitters)
            {
                // Main properties
                effectStream << "{\n\temitter_name: " << emitter.name.c_str() << ",\n";
                if (emitter.randomSeed)
                {
                    effectStream << "\tseed: random,\n";
                }
                else
                {
                    effectStream << "\tseed: " << emitter.rng.GetSeed() << ",\n";
                }
                effectStream << "\tduration: " << emitter.duration << ",\n";
                effectStream << "\tmax_particles: " << emitter.maxActiveParticles << ",\n";
                effectStream << "\toffset: {x: " << emitter.offset.x << ", y: " << emitter.offset.y << ", z: " << emitter.offset.z << "},\n";

                // Blend mode
                effectStream << "\tblend_mode: ";
                switch (emitter.blendMode)
                {
                    case CPUParticleEmitter::BlendMode::None:
                        effectStream << "none,\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Additive:
                        effectStream << "additive,\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Standard:
                        effectStream << "standard,\n";
                        break;
                }

                // Texture
                if (emitter.texture) effectStream << "\ttexture: " << emitter.texPath << ",\n";

                // Spawn / burst properties
                effectStream << "\tspawn_mode: ";
                switch (emitter.particleProperties.spawnMode)
                {
                    case CPUParticleEmitter::SpawnMode::Continuous:
                        effectStream << "continuous,\n";
                        effectStream << "\tspawn_rate: " << emitter.particleProperties.spawnRate << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::ContinuousBurst:
                        effectStream << "continuous_burst,\n";
                        effectStream << "\tspawn_rate: " << emitter.particleProperties.spawnRate << ",\n";
                        effectStream << "\tburst_count: " << emitter.particleProperties.burstCount << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::Random:
                        effectStream << "random,\n";
                        effectStream << "\tspawn_rate_min: " << emitter.particleProperties.spawnRateMin << ",\n";
                        effectStream << "\tspawn_rate_max: " << emitter.particleProperties.spawnRateMax << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::RandomBurst:
                        effectStream << "random_burst,\n";
                        effectStream << "\tspawn_rate_min: " << emitter.particleProperties.spawnRateMin << ",\n";
                        effectStream << "\tspawn_rate_max: " << emitter.particleProperties.spawnRateMax << ",\n";
                        effectStream << "\tburst_count_min: " << emitter.particleProperties.burstCountMin << ",\n";
                        effectStream << "\tburst_count_max: " << emitter.particleProperties.burstCountMax << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::SingleBurst:
                        effectStream << "single_burst,\n";
                        effectStream << "\tburst_count: " << emitter.particleProperties.burstCount << ",\n\n";
                        break;
                }

                // Particle properties
                effectStream << "\tparticle_properties: {\n";

                // Position
                effectStream << "\t\tposition: {type: ";
                switch (emitter.particleProperties.positionMode)
                {
                    case CPUParticleEmitter::PositionMode::Constant:
                        effectStream << "constant, value: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomMinMax:
                        effectStream << "random_min_max, min: {x: " << emitter.particleProperties.positionMin.x
                            << ", y: " << emitter.particleProperties.positionMin.y
                            << ", z: " << emitter.particleProperties.positionMin.z
                            << "}, max: {x: " << emitter.particleProperties.positionMax.x
                            << ", y: " << emitter.particleProperties.positionMax.y
                            << ", z: " << emitter.particleProperties.positionMax.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomSphere:
                        effectStream << "random_sphere, center: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}, radius: " << emitter.particleProperties.spawnRadius << "},\n";
                        break;
                }

                // Velocity
                effectStream << "\t\tvelocity: {type: ";
                switch (emitter.particleProperties.velocityMode)
                {
                    case CPUParticleEmitter::VelocityMode::Constant:
                        effectStream << "constant, value: {x: " << emitter.particleProperties.velocity.x
                            << ", y: " << emitter.particleProperties.velocity.y
                            << ", z: " << emitter.particleProperties.velocity.z;
                        break;
                    
                    case CPUParticleEmitter::VelocityMode::RandomMinMax:
                        effectStream << "random_min_max, min: {x: " << emitter.particleProperties.velocityMin.x
                            << ", y: " << emitter.particleProperties.velocityMin.y
                            << ", z: " << emitter.particleProperties.velocityMin.z
                            << "}, max: {x: " << emitter.particleProperties.velocityMax.x
                            << ", y: " << emitter.particleProperties.velocityMax.y
                            << ", z: " << emitter.particleProperties.velocityMax.z;
                        break;
                }

                // Output velocity damping no matter the mode
                effectStream << "}, damping: " << emitter.particleProperties.damping << "},\n";

                // Color
                effectStream << "\t\tcolor: {type: ";
                switch (emitter.particleProperties.colorMode)
                {
                    case CPUParticleEmitter::ColorMode::Constant:
                        effectStream << "constant, value: {r: " << emitter.particleProperties.color.r
                            << ", g: " << emitter.particleProperties.color.g
                            << ", b: " << emitter.particleProperties.color.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomMinMax:
                        effectStream << "random_min_max, min: {r: " << emitter.particleProperties.colorMin.r
                            << ", g: " << emitter.particleProperties.colorMin.g
                            << ", b: " << emitter.particleProperties.colorMin.b
                            << "}, max: {r: " << emitter.particleProperties.colorMax.r
                            << ", g: " << emitter.particleProperties.colorMax.g
                            << ", b: " << emitter.particleProperties.colorMax.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomLerp:
                        effectStream << "random_lerp, color_a: {r: " << emitter.particleProperties.colorA.r
                            << ", g: " << emitter.particleProperties.colorA.g
                            << ", b: " << emitter.particleProperties.colorA.b
                            << "}, color_b: {r: " << emitter.particleProperties.colorB.r
                            << ", g: " << emitter.particleProperties.colorB.g
                            << ", b: " << emitter.particleProperties.colorB.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::LerpOverLifetime:
                        effectStream << "lerp_over_lifetime, start_color: {r: " << emitter.particleProperties.startColor.r
                        << ", g: " << emitter.particleProperties.startColor.g << ", b: " << emitter.particleProperties.startColor.b
                        << "}, end_color: {r: " << emitter.particleProperties.endColor.r << ", g: " << emitter.particleProperties.endColor.g
                        << ", b: " << emitter.particleProperties.endColor.b << "}},\n";
                        break;
                }

                // Size
                effectStream << "\t\tsize: {type: ";
                switch (emitter.particleProperties.sizeMode)
                {
                    case CPUParticleEmitter::SizeMode::Constant:
                        effectStream << "constant, value: {x: " << emitter.particleProperties.size.x
                            << ", y: " << emitter.particleProperties.size.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomMinMax:
                        effectStream << "random_min_max, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomLerp:
                        effectStream << "random_lerp, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::LerpOverLifetime:
                        effectStream << "lerp_over_lifetime, start_size: {x: " << emitter.particleProperties.startSize.x
                            << ", y: " << emitter.particleProperties.startSize.y
                            << "}, end_size: {x: " << emitter.particleProperties.endSize.x
                            << ", y: " << emitter.particleProperties.endSize.y << "}},\n";
                        break;
                }

                // Opacity
                effectStream << "\t\topacity: {type: ";
                switch (emitter.particleProperties.opacityMode)
                {
                    case CPUParticleEmitter::OpacityMode::Constant:
                        effectStream << "constant, value: " << emitter.particleProperties.opacity << "},\n";
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::RandomMinMax:
                        effectStream << "random_min_max, min: " << emitter.particleProperties.opacityMin
                            << ", max: " << emitter.particleProperties.opacityMax << "},\n";
                        break;
                    case CPUParticleEmitter::OpacityMode::LerpOverLifetime:
                        effectStream << "lerp_over_lifetime, start_opacity: " << emitter.particleProperties.startOpacity
                            << ", end_opacity: " << emitter.particleProperties.endOpacity << "},\n";
                        break;
                }

                // Lifespan
                effectStream << "\t\tlifespan: {type: ";
                switch (emitter.particleProperties.lifespanMode)
                {
                    case CPUParticleEmitter::LifespanMode::Constant:
                        effectStream << "constant, value: " << emitter.particleProperties.lifespan << "},\n";
                        break;
                    
                    case CPUParticleEmitter::LifespanMode::RandomMinMax:
                        effectStream << "random_min_max, min: " << emitter.particleProperties.lifespanMin
                            << ", max: " << emitter.particleProperties.lifespanMax << "},\n";
                        break;
                }

                effectStream << "\t},\n\n";
                
                // Basic Affectors
                effectStream << "\taffectors: {\n";
                effectStream << "\t\tadd_velocity: " << (emitter.affectorProperties.addVelocity ? "true,\n" : "false,\n");
                effectStream << "\t\tgravity: " << (emitter.affectorProperties.gravityEnabled ? "true,\n" : "false,\n");
                effectStream << "\t},\n\n";

                // Attractors
                effectStream << "\tattractors: [";
                for (int i = 0; i < emitter.attractors.size(); ++i)
                {
                    const auto& a = emitter.attractors[i];
                    effectStream << "\n\t\t{position: {x: " << a.position.x << ", y: " << a.position.y << ", z: " << a.position.z
                        << "}, radius: " << a.radius << ", strength: " << a.strength << ", relative: " << (a.relativeToTransform ? "true" : "false") << "},";
                }
                effectStream << "\n\t]\n";

                effectStream << "},\n";
            }

            effectStream << "]\n";
        }
        else
        {
            // Output each emitter file separately

            // Create the effect file stream
            std::ofstream effectStream(globalPath);

            // Name, spawnRelative and renderRelative
            effectStream << "effect_name: " << name.c_str() << "\nspawn_relative: "
                << (spawnRelativeTransform ? "true" : "false") << "\nrender_relative: "
                << (renderRelativeTransform ? "true" : "false") << "\nemitters: [\n";
            
            // Find if an extension is given
            size_t pos = path.find_last_of('.');
            
            // Output all emitters
            for (const auto& emitter : loadedEmitters)
            {
                // Generate the emitter's global path
                std::string emitterGlobalPath;
                if (pos == std::string::npos)
                {
                    // No extension given
                    emitterGlobalPath = globalPath + "-" + emitter.name + ".emitter";
                }
                else
                {
                    // Extension supplied from user choice (remove extension)
                    emitterGlobalPath = globalPath.substr(0, pos) + "-" + emitter.name + ".emitter";
                }

                // Write the emitter reference to the effect file
                // Localizes path to preserve data:// or user:// tokens
                // TODO: Maybe make local paths a toggleable editor setting?
                effectStream << "\t{file: " << File::LocalizePath(emitterGlobalPath) << "},\n";

                // Create the emitter file stream
                std::ofstream emitterStream(emitterGlobalPath);

                // Main properties
                emitterStream << "emitter_name: " << emitter.name.c_str() << "\n";
                if (emitter.randomSeed)
                {
                    emitterStream << "seed: random\n";
                }
                else
                {
                    emitterStream << "seed: " << emitter.rng.GetSeed() << "\n";
                }
                emitterStream << "duration: " << emitter.duration << "\n";
                emitterStream << "max_particles: " << emitter.maxActiveParticles << "\n";
                emitterStream << "offset: {x: " << emitter.offset.x << ", y: " << emitter.offset.y << ", z: " << emitter.offset.z << "}\n";

                // Blend mode
                emitterStream << "blend_mode: ";
                switch (emitter.blendMode)
                {
                    case CPUParticleEmitter::BlendMode::None:
                        emitterStream << "none\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Additive:
                        emitterStream << "additive\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Standard:
                        emitterStream << "standard\n";
                        break;
                }

                // Texture
                if (emitter.texture) emitterStream << "texture: " << emitter.texPath << "\n";

                // Spawn / burst properties
                emitterStream << "spawn_mode: ";
                switch (emitter.particleProperties.spawnMode)
                {
                    case CPUParticleEmitter::SpawnMode::Continuous:
                        emitterStream << "continuous\n";
                        emitterStream << "spawn_rate: " << emitter.particleProperties.spawnRate << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::ContinuousBurst:
                        emitterStream << "continuous_burst\n";
                        emitterStream << "spawn_rate: " << emitter.particleProperties.spawnRate << "\n";
                        emitterStream << "burst_count: " << emitter.particleProperties.burstCount << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::Random:
                        emitterStream << "random\n";
                        emitterStream << "spawn_rate_min: " << emitter.particleProperties.spawnRateMin << "\n";
                        emitterStream << "spawn_rate_max: " << emitter.particleProperties.spawnRateMax << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::RandomBurst:
                        emitterStream << "random_burst\n";
                        emitterStream << "spawn_rate_min: " << emitter.particleProperties.spawnRateMin << "\n";
                        emitterStream << "spawn_rate_max: " << emitter.particleProperties.spawnRateMax << "\n";
                        emitterStream << "burst_count_min: " << emitter.particleProperties.burstCountMin << "\n";
                        emitterStream << "burst_count_max: " << emitter.particleProperties.burstCountMax << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::SingleBurst:
                        emitterStream << "single_burst\n";
                        emitterStream << "burst_count: " << emitter.particleProperties.burstCount << "\n\n";
                        break;
                }

                // Particle properties
                emitterStream << "particle_properties: {\n";

                // Position
                emitterStream << "\tposition: {type: ";
                switch (emitter.particleProperties.positionMode)
                {
                    case CPUParticleEmitter::PositionMode::Constant:
                        emitterStream << "constant, value: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomMinMax:
                        emitterStream << "random_min_max, min: {x: " << emitter.particleProperties.positionMin.x
                            << ", y: " << emitter.particleProperties.positionMin.y
                            << ", z: " << emitter.particleProperties.positionMin.z
                            << "}, max: {x: " << emitter.particleProperties.positionMax.x
                            << ", y: " << emitter.particleProperties.positionMax.y
                            << ", z: " << emitter.particleProperties.positionMax.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomSphere:
                        emitterStream << "random_sphere, center: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}, radius: " << emitter.particleProperties.spawnRadius << "},\n";
                        break;
                }

                // Velocity
                emitterStream << "\tvelocity: {type: ";
                switch (emitter.particleProperties.velocityMode)
                {
                    case CPUParticleEmitter::VelocityMode::Constant:
                        emitterStream << "constant, value: {x: " << emitter.particleProperties.velocity.x
                            << ", y: " << emitter.particleProperties.velocity.y
                            << ", z: " << emitter.particleProperties.velocity.z;
                        break;
                    
                    case CPUParticleEmitter::VelocityMode::RandomMinMax:
                        emitterStream << "random_min_max, min: {x: " << emitter.particleProperties.velocityMin.x
                            << ", y: " << emitter.particleProperties.velocityMin.y
                            << ", z: " << emitter.particleProperties.velocityMin.z
                            << "}, max: {x: " << emitter.particleProperties.velocityMax.x
                            << ", y: " << emitter.particleProperties.velocityMax.y
                            << ", z: " << emitter.particleProperties.velocityMax.z;
                        break;
                }

                // Output velocity damping no matter the mode
                emitterStream << "}, damping: " << emitter.particleProperties.damping << "},\n";

                // Color
                emitterStream << "\tcolor: {type: ";
                switch (emitter.particleProperties.colorMode)
                {
                    case CPUParticleEmitter::ColorMode::Constant:
                        emitterStream << "constant, value: {r: " << emitter.particleProperties.color.r
                            << ", g: " << emitter.particleProperties.color.g
                            << ", b: " << emitter.particleProperties.color.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomMinMax:
                        emitterStream << "random_min_max, min: {r: " << emitter.particleProperties.colorMin.r
                            << ", g: " << emitter.particleProperties.colorMin.g
                            << ", b: " << emitter.particleProperties.colorMin.b
                            << "}, max: {r: " << emitter.particleProperties.colorMax.r
                            << ", g: " << emitter.particleProperties.colorMax.g
                            << ", b: " << emitter.particleProperties.colorMax.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomLerp:
                        emitterStream << "random_lerp, color_a: {r: " << emitter.particleProperties.colorA.r
                            << ", g: " << emitter.particleProperties.colorA.g
                            << ", b: " << emitter.particleProperties.colorA.b
                            << "}, color_b: {r: " << emitter.particleProperties.colorB.r
                            << ", g: " << emitter.particleProperties.colorB.g
                            << ", b: " << emitter.particleProperties.colorB.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::LerpOverLifetime:
                        emitterStream << "lerp_over_lifetime, start_color: {r: " << emitter.particleProperties.startColor.r
                        << ", g: " << emitter.particleProperties.startColor.g << ", b: " << emitter.particleProperties.startColor.b
                        << "}, end_color: {r: " << emitter.particleProperties.endColor.r << ", g: " << emitter.particleProperties.endColor.g
                        << ", b: " << emitter.particleProperties.endColor.b << "}},\n";
                        break;
                }

                // Size
                emitterStream << "\tsize: {type: ";
                switch (emitter.particleProperties.sizeMode)
                {
                    case CPUParticleEmitter::SizeMode::Constant:
                        emitterStream << "constant, value: {x: " << emitter.particleProperties.size.x
                            << ", y: " << emitter.particleProperties.size.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomMinMax:
                        emitterStream << "random_min_max, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomLerp:
                        emitterStream << "random_lerp, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::LerpOverLifetime:
                        emitterStream << "lerp_over_lifetime, start_size: {x: " << emitter.particleProperties.startSize.x
                            << ", y: " << emitter.particleProperties.startSize.y
                            << "}, end_size: {x: " << emitter.particleProperties.endSize.x
                            << ", y: " << emitter.particleProperties.endSize.y << "}},\n";
                        break;
                }

                // Opacity
                emitterStream << "\topacity: {type: ";
                switch (emitter.particleProperties.opacityMode)
                {
                    case CPUParticleEmitter::OpacityMode::Constant:
                        emitterStream << "constant, value: " << emitter.particleProperties.opacity << "},\n";
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::RandomMinMax:
                        emitterStream << "random_min_max, min: " << emitter.particleProperties.opacityMin
                            << ", max: " << emitter.particleProperties.opacityMax << "},\n";
                        break;
                    case CPUParticleEmitter::OpacityMode::LerpOverLifetime:
                        emitterStream << "lerp_over_lifetime, start_opacity: " << emitter.particleProperties.startOpacity
                            << ", end_opacity: " << emitter.particleProperties.endOpacity << "},\n";
                        break;
                }

                // Lifespan
                emitterStream << "\tlifespan: {type: ";
                switch (emitter.particleProperties.lifespanMode)
                {
                    case CPUParticleEmitter::LifespanMode::Constant:
                        emitterStream << "constant, value: " << emitter.particleProperties.lifespan << "},\n";
                        break;
                    
                    case CPUParticleEmitter::LifespanMode::RandomMinMax:
                        emitterStream << "random_min_max, min: " << emitter.particleProperties.lifespanMin
                            << ", max: " << emitter.particleProperties.lifespanMax << "},\n";
                        break;
                }

                emitterStream << "}\n\n";
                
                // Basic Affectors
                emitterStream << "affectors: {\n";
                emitterStream << "\tadd_velocity: " << (emitter.affectorProperties.addVelocity ? "true,\n" : "false,\n");
                emitterStream << "\tgravity: " << (emitter.affectorProperties.gravityEnabled ? "true,\n" : "false,\n");
                emitterStream << "}\n\n";

                // Attractors
                emitterStream << "attractors: [";
                for (int i = 0; i < emitter.attractors.size(); ++i)
                {
                    const auto& a = emitter.attractors[i];
                    emitterStream << "\n\t{position: {x: " << a.position.x << ", y: " << a.position.y << ", z: " << a.position.z
                        << "}, radius: " << a.radius << ", strength: " << a.strength << ", relative: " << (a.relativeToTransform ? "true" : "false") << "},";
                }
                emitterStream << "\n]";
            }

            effectStream << "]\n";
        }
    }

    void CPUParticleEffect::Reset()
    {
        // Reset to default state
        state = State::Play;
        renderRelativeTransform = false;
        spawnRelativeTransform = false;

        // Remove all emitters
        loadedEmitters.clear();

        // Reset name
        name = "New Effect";
    }
}