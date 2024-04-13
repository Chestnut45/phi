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
        if (singleFile)
        {
            // Output the entire effect contents to a single file

            // Create the effect file stream
            File outputFile(path, File::Mode::Write);

            // Name, spawnRelative and renderRelative
            outputFile << "effect_name: " << name.c_str() << "\nspawn_relative: "
                << (spawnRelativeTransform ? "true" : "false") << "\nrender_relative: "
                << (renderRelativeTransform ? "true" : "false") << "\nemitters: [\n";
            
            // Output all emitters
            for (const auto& emitter : loadedEmitters)
            {
                // Main properties
                outputFile << "{\n\temitter_name: " << emitter.name.c_str() << ",\n";
                if (emitter.randomSeed)
                {
                    outputFile << "\tseed: random,\n";
                }
                else
                {
                    outputFile << "\tseed: " << emitter.rng.GetSeed() << ",\n";
                }
                outputFile << "\tduration: " << emitter.duration << ",\n";
                outputFile << "\tmax_particles: " << emitter.maxActiveParticles << ",\n";
                outputFile << "\toffset: {x: " << emitter.offset.x << ", y: " << emitter.offset.y << ", z: " << emitter.offset.z << "},\n";

                // Blend mode
                outputFile << "\tblend_mode: ";
                switch (emitter.blendMode)
                {
                    case CPUParticleEmitter::BlendMode::None:
                        outputFile << "none,\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Additive:
                        outputFile << "additive,\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Standard:
                        outputFile << "standard,\n";
                        break;
                }

                // Texture
                if (emitter.texture) outputFile << "\ttexture: " << emitter.texPath << ",\n";

                // Spawn / burst properties
                outputFile << "\tspawn_mode: ";
                switch (emitter.particleProperties.spawnMode)
                {
                    case CPUParticleEmitter::SpawnMode::Continuous:
                        outputFile << "continuous,\n";
                        outputFile << "\tspawn_rate: " << emitter.particleProperties.spawnRate << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::ContinuousBurst:
                        outputFile << "continuous_burst,\n";
                        outputFile << "\tspawn_rate: " << emitter.particleProperties.spawnRate << ",\n";
                        outputFile << "\tburst_count: " << emitter.particleProperties.burstCount << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::Random:
                        outputFile << "random,\n";
                        outputFile << "\tspawn_rate_min: " << emitter.particleProperties.spawnRateMin << ",\n";
                        outputFile << "\tspawn_rate_max: " << emitter.particleProperties.spawnRateMax << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::RandomBurst:
                        outputFile << "random_burst,\n";
                        outputFile << "\tspawn_rate_min: " << emitter.particleProperties.spawnRateMin << ",\n";
                        outputFile << "\tspawn_rate_max: " << emitter.particleProperties.spawnRateMax << ",\n";
                        outputFile << "\tburst_count_min: " << emitter.particleProperties.burstCountMin << ",\n";
                        outputFile << "\tburst_count_max: " << emitter.particleProperties.burstCountMax << ",\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::SingleBurst:
                        outputFile << "single_burst,\n";
                        outputFile << "\tburst_count: " << emitter.particleProperties.burstCount << ",\n\n";
                        break;
                }

                // Particle properties
                outputFile << "\tparticle_properties: {\n";

                // Position
                outputFile << "\t\tposition: {type: ";
                switch (emitter.particleProperties.positionMode)
                {
                    case CPUParticleEmitter::PositionMode::Constant:
                        outputFile << "constant, value: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomMinMax:
                        outputFile << "random_min_max, min: {x: " << emitter.particleProperties.positionMin.x
                            << ", y: " << emitter.particleProperties.positionMin.y
                            << ", z: " << emitter.particleProperties.positionMin.z
                            << "}, max: {x: " << emitter.particleProperties.positionMax.x
                            << ", y: " << emitter.particleProperties.positionMax.y
                            << ", z: " << emitter.particleProperties.positionMax.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomSphere:
                        outputFile << "random_sphere, center: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}, radius: " << emitter.particleProperties.spawnRadius << "},\n";
                        break;
                }

                // Velocity
                outputFile << "\t\tvelocity: {type: ";
                switch (emitter.particleProperties.velocityMode)
                {
                    case CPUParticleEmitter::VelocityMode::Constant:
                        outputFile << "constant, value: {x: " << emitter.particleProperties.velocity.x
                            << ", y: " << emitter.particleProperties.velocity.y
                            << ", z: " << emitter.particleProperties.velocity.z;
                        break;
                    
                    case CPUParticleEmitter::VelocityMode::RandomMinMax:
                        outputFile << "random_min_max, min: {x: " << emitter.particleProperties.velocityMin.x
                            << ", y: " << emitter.particleProperties.velocityMin.y
                            << ", z: " << emitter.particleProperties.velocityMin.z
                            << "}, max: {x: " << emitter.particleProperties.velocityMax.x
                            << ", y: " << emitter.particleProperties.velocityMax.y
                            << ", z: " << emitter.particleProperties.velocityMax.z;
                        break;
                }

                // Output velocity damping no matter the mode
                outputFile << "}, damping: " << emitter.particleProperties.damping << "},\n";

                // Color
                outputFile << "\t\tcolor: {type: ";
                switch (emitter.particleProperties.colorMode)
                {
                    case CPUParticleEmitter::ColorMode::Constant:
                        outputFile << "constant, value: {r: " << emitter.particleProperties.color.r
                            << ", g: " << emitter.particleProperties.color.g
                            << ", b: " << emitter.particleProperties.color.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomMinMax:
                        outputFile << "random_min_max, min: {r: " << emitter.particleProperties.colorMin.r
                            << ", g: " << emitter.particleProperties.colorMin.g
                            << ", b: " << emitter.particleProperties.colorMin.b
                            << "}, max: {r: " << emitter.particleProperties.colorMax.r
                            << ", g: " << emitter.particleProperties.colorMax.g
                            << ", b: " << emitter.particleProperties.colorMax.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomLerp:
                        outputFile << "random_lerp, color_a: {r: " << emitter.particleProperties.colorA.r
                            << ", g: " << emitter.particleProperties.colorA.g
                            << ", b: " << emitter.particleProperties.colorA.b
                            << "}, color_b: {r: " << emitter.particleProperties.colorB.r
                            << ", g: " << emitter.particleProperties.colorB.g
                            << ", b: " << emitter.particleProperties.colorB.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::LerpOverLifetime:
                        outputFile << "lerp_over_lifetime, start_color: {r: " << emitter.particleProperties.startColor.r
                        << ", g: " << emitter.particleProperties.startColor.g << ", b: " << emitter.particleProperties.startColor.b
                        << "}, end_color: {r: " << emitter.particleProperties.endColor.r << ", g: " << emitter.particleProperties.endColor.g
                        << ", b: " << emitter.particleProperties.endColor.b << "}},\n";
                        break;
                }

                // Size
                outputFile << "\t\tsize: {type: ";
                switch (emitter.particleProperties.sizeMode)
                {
                    case CPUParticleEmitter::SizeMode::Constant:
                        outputFile << "constant, value: {x: " << emitter.particleProperties.size.x
                            << ", y: " << emitter.particleProperties.size.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomMinMax:
                        outputFile << "random_min_max, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomLerp:
                        outputFile << "random_lerp, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::LerpOverLifetime:
                        outputFile << "lerp_over_lifetime, start_size: {x: " << emitter.particleProperties.startSize.x
                            << ", y: " << emitter.particleProperties.startSize.y
                            << "}, end_size: {x: " << emitter.particleProperties.endSize.x
                            << ", y: " << emitter.particleProperties.endSize.y << "}},\n";
                        break;
                }

                // Opacity
                outputFile << "\t\topacity: {type: ";
                switch (emitter.particleProperties.opacityMode)
                {
                    case CPUParticleEmitter::OpacityMode::Constant:
                        outputFile << "constant, value: " << emitter.particleProperties.opacity << "},\n";
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::RandomMinMax:
                        outputFile << "random_min_max, min: " << emitter.particleProperties.opacityMin
                            << ", max: " << emitter.particleProperties.opacityMax << "},\n";
                        break;
                    case CPUParticleEmitter::OpacityMode::LerpOverLifetime:
                        outputFile << "lerp_over_lifetime, start_opacity: " << emitter.particleProperties.startOpacity
                            << ", end_opacity: " << emitter.particleProperties.endOpacity << "},\n";
                        break;
                }

                // Lifespan
                outputFile << "\t\tlifespan: {type: ";
                switch (emitter.particleProperties.lifespanMode)
                {
                    case CPUParticleEmitter::LifespanMode::Constant:
                        outputFile << "constant, value: " << emitter.particleProperties.lifespan << "},\n";
                        break;
                    
                    case CPUParticleEmitter::LifespanMode::RandomMinMax:
                        outputFile << "random_min_max, min: " << emitter.particleProperties.lifespanMin
                            << ", max: " << emitter.particleProperties.lifespanMax << "},\n";
                        break;
                }

                outputFile << "\t},\n\n";
                
                // Basic Affectors
                outputFile << "\taffectors: {\n";
                outputFile << "\t\tadd_velocity: " << (emitter.affectorProperties.addVelocity ? "true,\n" : "false,\n");
                outputFile << "\t\tgravity: " << (emitter.affectorProperties.gravityEnabled ? "true,\n" : "false,\n");
                outputFile << "\t},\n\n";

                // Attractors
                outputFile << "\tattractors: [";
                for (int i = 0; i < emitter.attractors.size(); ++i)
                {
                    const auto& a = emitter.attractors[i];
                    outputFile << "\n\t\t{position: {x: " << a.position.x << ", y: " << a.position.y << ", z: " << a.position.z
                        << "}, radius: " << a.radius << ", strength: " << a.strength << ", relative: " << (a.relativeToTransform ? "true" : "false") << "},";
                }
                outputFile << "\n\t]\n";

                outputFile << "},\n";
            }

            outputFile << "]\n";
        }
        else
        {
            // Output each emitter file separately

            // Create the effect file stream
            File effectFile(path, File::Mode::Write);

            // Name, spawnRelative and renderRelative
            effectFile << "effect_name: " << name.c_str() << "\nspawn_relative: "
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
                    emitterGlobalPath = effectFile.GetGlobalPath() + "-" + emitter.name + ".emitter";
                }
                else
                {
                    // Extension supplied from user choice (remove extension)
                    emitterGlobalPath = effectFile.GetGlobalPath().substr(0, pos) + "-" + emitter.name + ".emitter";
                }

                // Write the emitter reference to the effect file
                // Localizes path to preserve data:// or user:// tokens
                // TODO: Maybe make local paths a toggleable editor setting?
                effectFile << "\t{file: " << File::LocalizePath(emitterGlobalPath) << "},\n";

                // Create the emitter file stream
                File emitterFile(emitterGlobalPath, File::Mode::Write);

                // Main properties
                emitterFile << "emitter_name: " << emitter.name.c_str() << "\n";
                if (emitter.randomSeed)
                {
                    emitterFile << "seed: random\n";
                }
                else
                {
                    emitterFile << "seed: " << emitter.rng.GetSeed() << "\n";
                }
                emitterFile << "duration: " << emitter.duration << "\n";
                emitterFile << "max_particles: " << emitter.maxActiveParticles << "\n";
                emitterFile << "offset: {x: " << emitter.offset.x << ", y: " << emitter.offset.y << ", z: " << emitter.offset.z << "}\n";

                // Blend mode
                emitterFile << "blend_mode: ";
                switch (emitter.blendMode)
                {
                    case CPUParticleEmitter::BlendMode::None:
                        emitterFile << "none\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Additive:
                        emitterFile << "additive\n";
                        break;
                    
                    case CPUParticleEmitter::BlendMode::Standard:
                        emitterFile << "standard\n";
                        break;
                }

                // Texture
                if (emitter.texture) emitterFile << "texture: " << emitter.texPath << "\n";

                // Spawn / burst properties
                emitterFile << "spawn_mode: ";
                switch (emitter.particleProperties.spawnMode)
                {
                    case CPUParticleEmitter::SpawnMode::Continuous:
                        emitterFile << "continuous\n";
                        emitterFile << "spawn_rate: " << emitter.particleProperties.spawnRate << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::ContinuousBurst:
                        emitterFile << "continuous_burst\n";
                        emitterFile << "spawn_rate: " << emitter.particleProperties.spawnRate << "\n";
                        emitterFile << "burst_count: " << emitter.particleProperties.burstCount << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::Random:
                        emitterFile << "random\n";
                        emitterFile << "spawn_rate_min: " << emitter.particleProperties.spawnRateMin << "\n";
                        emitterFile << "spawn_rate_max: " << emitter.particleProperties.spawnRateMax << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::RandomBurst:
                        emitterFile << "random_burst\n";
                        emitterFile << "spawn_rate_min: " << emitter.particleProperties.spawnRateMin << "\n";
                        emitterFile << "spawn_rate_max: " << emitter.particleProperties.spawnRateMax << "\n";
                        emitterFile << "burst_count_min: " << emitter.particleProperties.burstCountMin << "\n";
                        emitterFile << "burst_count_max: " << emitter.particleProperties.burstCountMax << "\n\n";
                        break;
                    
                    case CPUParticleEmitter::SpawnMode::SingleBurst:
                        emitterFile << "single_burst\n";
                        emitterFile << "burst_count: " << emitter.particleProperties.burstCount << "\n\n";
                        break;
                }

                // Particle properties
                emitterFile << "particle_properties: {\n";

                // Position
                emitterFile << "\tposition: {type: ";
                switch (emitter.particleProperties.positionMode)
                {
                    case CPUParticleEmitter::PositionMode::Constant:
                        emitterFile << "constant, value: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomMinMax:
                        emitterFile << "random_min_max, min: {x: " << emitter.particleProperties.positionMin.x
                            << ", y: " << emitter.particleProperties.positionMin.y
                            << ", z: " << emitter.particleProperties.positionMin.z
                            << "}, max: {x: " << emitter.particleProperties.positionMax.x
                            << ", y: " << emitter.particleProperties.positionMax.y
                            << ", z: " << emitter.particleProperties.positionMax.z << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::PositionMode::RandomSphere:
                        emitterFile << "random_sphere, center: {x: " << emitter.particleProperties.position.x
                            << ", y: " << emitter.particleProperties.position.y
                            << ", z: " << emitter.particleProperties.position.z << "}, radius: " << emitter.particleProperties.spawnRadius << "},\n";
                        break;
                }

                // Velocity
                emitterFile << "\tvelocity: {type: ";
                switch (emitter.particleProperties.velocityMode)
                {
                    case CPUParticleEmitter::VelocityMode::Constant:
                        emitterFile << "constant, value: {x: " << emitter.particleProperties.velocity.x
                            << ", y: " << emitter.particleProperties.velocity.y
                            << ", z: " << emitter.particleProperties.velocity.z;
                        break;
                    
                    case CPUParticleEmitter::VelocityMode::RandomMinMax:
                        emitterFile << "random_min_max, min: {x: " << emitter.particleProperties.velocityMin.x
                            << ", y: " << emitter.particleProperties.velocityMin.y
                            << ", z: " << emitter.particleProperties.velocityMin.z
                            << "}, max: {x: " << emitter.particleProperties.velocityMax.x
                            << ", y: " << emitter.particleProperties.velocityMax.y
                            << ", z: " << emitter.particleProperties.velocityMax.z;
                        break;
                }

                // Output velocity damping no matter the mode
                emitterFile << "}, damping: " << emitter.particleProperties.damping << "},\n";

                // Color
                emitterFile << "\tcolor: {type: ";
                switch (emitter.particleProperties.colorMode)
                {
                    case CPUParticleEmitter::ColorMode::Constant:
                        emitterFile << "constant, value: {r: " << emitter.particleProperties.color.r
                            << ", g: " << emitter.particleProperties.color.g
                            << ", b: " << emitter.particleProperties.color.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomMinMax:
                        emitterFile << "random_min_max, min: {r: " << emitter.particleProperties.colorMin.r
                            << ", g: " << emitter.particleProperties.colorMin.g
                            << ", b: " << emitter.particleProperties.colorMin.b
                            << "}, max: {r: " << emitter.particleProperties.colorMax.r
                            << ", g: " << emitter.particleProperties.colorMax.g
                            << ", b: " << emitter.particleProperties.colorMax.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomLerp:
                        emitterFile << "random_lerp, color_a: {r: " << emitter.particleProperties.colorA.r
                            << ", g: " << emitter.particleProperties.colorA.g
                            << ", b: " << emitter.particleProperties.colorA.b
                            << "}, color_b: {r: " << emitter.particleProperties.colorB.r
                            << ", g: " << emitter.particleProperties.colorB.g
                            << ", b: " << emitter.particleProperties.colorB.b << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::ColorMode::LerpOverLifetime:
                        emitterFile << "lerp_over_lifetime, start_color: {r: " << emitter.particleProperties.startColor.r
                        << ", g: " << emitter.particleProperties.startColor.g << ", b: " << emitter.particleProperties.startColor.b
                        << "}, end_color: {r: " << emitter.particleProperties.endColor.r << ", g: " << emitter.particleProperties.endColor.g
                        << ", b: " << emitter.particleProperties.endColor.b << "}},\n";
                        break;
                }

                // Size
                emitterFile << "\tsize: {type: ";
                switch (emitter.particleProperties.sizeMode)
                {
                    case CPUParticleEmitter::SizeMode::Constant:
                        emitterFile << "constant, value: {x: " << emitter.particleProperties.size.x
                            << ", y: " << emitter.particleProperties.size.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomMinMax:
                        emitterFile << "random_min_max, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomLerp:
                        emitterFile << "random_lerp, min: {x: " << emitter.particleProperties.sizeMin.x
                            << ", y: " << emitter.particleProperties.sizeMin.y
                            << "}, max: {x: " << emitter.particleProperties.sizeMax.x
                            << ", y: " << emitter.particleProperties.sizeMax.y << "}},\n";
                        break;
                    
                    case CPUParticleEmitter::SizeMode::LerpOverLifetime:
                        emitterFile << "lerp_over_lifetime, start_size: {x: " << emitter.particleProperties.startSize.x
                            << ", y: " << emitter.particleProperties.startSize.y
                            << "}, end_size: {x: " << emitter.particleProperties.endSize.x
                            << ", y: " << emitter.particleProperties.endSize.y << "}},\n";
                        break;
                }

                // Opacity
                emitterFile << "\topacity: {type: ";
                switch (emitter.particleProperties.opacityMode)
                {
                    case CPUParticleEmitter::OpacityMode::Constant:
                        emitterFile << "constant, value: " << emitter.particleProperties.opacity << "},\n";
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::RandomMinMax:
                        emitterFile << "random_min_max, min: " << emitter.particleProperties.opacityMin
                            << ", max: " << emitter.particleProperties.opacityMax << "},\n";
                        break;
                    case CPUParticleEmitter::OpacityMode::LerpOverLifetime:
                        emitterFile << "lerp_over_lifetime, start_opacity: " << emitter.particleProperties.startOpacity
                            << ", end_opacity: " << emitter.particleProperties.endOpacity << "},\n";
                        break;
                }

                // Lifespan
                emitterFile << "\tlifespan: {type: ";
                switch (emitter.particleProperties.lifespanMode)
                {
                    case CPUParticleEmitter::LifespanMode::Constant:
                        emitterFile << "constant, value: " << emitter.particleProperties.lifespan << "},\n";
                        break;
                    
                    case CPUParticleEmitter::LifespanMode::RandomMinMax:
                        emitterFile << "random_min_max, min: " << emitter.particleProperties.lifespanMin
                            << ", max: " << emitter.particleProperties.lifespanMax << "},\n";
                        break;
                }

                emitterFile << "}\n\n";
                
                // Basic Affectors
                emitterFile << "affectors: {\n";
                emitterFile << "\tadd_velocity: " << (emitter.affectorProperties.addVelocity ? "true,\n" : "false,\n");
                emitterFile << "\tgravity: " << (emitter.affectorProperties.gravityEnabled ? "true,\n" : "false,\n");
                emitterFile << "}\n\n";

                // Attractors
                emitterFile << "attractors: [";
                for (int i = 0; i < emitter.attractors.size(); ++i)
                {
                    const auto& a = emitter.attractors[i];
                    emitterFile << "\n\t{position: {x: " << a.position.x << ", y: " << a.position.y << ", z: " << a.position.z
                        << "}, radius: " << a.radius << ", strength: " << a.strength << ", relative: " << (a.relativeToTransform ? "true" : "false") << "},";
                }
                emitterFile << "\n]";
            }

            effectFile << "]\n";
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