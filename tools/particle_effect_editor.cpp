#include "particle_effect_editor.hpp"

#include <filesystem>

ParticleEffectEditor* app = nullptr;

// Application entrypoint
int main(int, char**)
{
    app = new ParticleEffectEditor();
    app->Run();
    delete app;
    return 0;
}

ParticleEffectEditor::ParticleEffectEditor() : App("Particle Effect Editor", 4, 6)
{
    // Initialize mouse input
    input.EnableRawMouseMotion();
    
    // Initialize the scene

    // Enable rendering the scene to only part of the default framebuffer
    scene.SetRenderMode(Scene::RenderMode::CustomViewport);

    // Add a camera
    node = scene.CreateNode3D();
    Camera& camera = scene.CreateNode3D()->AddComponent<Camera>(sceneRenderWidth, sceneRenderHeight);
    scene.SetActiveCamera(camera);

    // Add a skybox
    Skybox& skybox = camera.GetNode()->AddComponent<Skybox>("data://textures/skybox_day", "data://textures/skybox_night_old");
    scene.SetActiveSkybox(skybox);

    // Add a directional light
    DirectionalLight& light = skybox.GetNode()->AddComponent<DirectionalLight>();
    //light.Activate(DirectionalLight::Slot::SLOT_0);

    // Load default fire effect
    currentEffect = &node->AddComponent<CPUParticleEffect>("data://effects/fire.effect");

    // Load materials
    scene.LoadMaterials("data://materials.yaml");

    // Create mesh
    BasicMesh& mesh = node->AddComponent<BasicMesh>();
    mesh.AddIcosphere(1.0f, 2);
    mesh.AddBox(1.0f, 2.0f, 1.0, glm::vec3(0, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(1, -1, 0));
    mesh.AddBox(2.0f, 0.5f, 0.5f, glm::vec3(-1, -1, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(-0.25f, -2, 0));
    mesh.AddBox(0.25f, 1.0f, 0.25f, glm::vec3(0.25f, -2, 0));
    mesh.SetMaterial("sapphire");

    // Add eyes
    BasicMesh& eyesMesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(-0.5f, 0.25f, 0.5f));
    eyesMesh.AddIcosphere(0.5f, 2, glm::vec3(0.5f, 0.25f, 0.5f));
    eyesMesh.SetMaterial("pearl");
    mesh.GetNode()->AddChild(eyesMesh.GetNode());

    // Add pupils to eyes (lol)
    BasicMesh& pupilsMesh = scene.CreateNode3D()->AddComponent<BasicMesh>();
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(0.5f, 0.25f, 0.9f));
    pupilsMesh.AddIcosphere(0.25f, 2, glm::vec3(-0.5f, 0.25f, 0.9f));
    pupilsMesh.SetMaterial("obsidian");
    mesh.GetNode()->AddChild(pupilsMesh.GetNode());

    // DEBUG: Add a voxel mesh to make sure scenes with custom viewports doesn't mess up stencil buffer transfers
    node->AddComponent<VoxelObject>().Load("data://models/mushroom.pvox");

    // DEBUG: Test point light
    PointLight& pointLight = node->AddComponent<PointLight>();
    pointLight.SetColor(glm::vec3(1.0f, 0.1f, 0.0f));
    pointLight.SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));

    // Log
    Log(name, " initialized");
}

ParticleEffectEditor::~ParticleEffectEditor()
{
    Log(name, " shutdown");
}

void ParticleEffectEditor::Update(float delta)
{
    // Manually update scene resolution on window resize
    if (windowResized)
    {
        // Calculate new scene size
        sceneRenderWidth = wWidth - editorWidth;
        sceneRenderHeight = wHeight;

        // Update rendering resolution and viewport
        scene.SetResolution(sceneRenderWidth, sceneRenderHeight);
        scene.SetViewport(editorWidth, 0, sceneRenderWidth, sceneRenderHeight);

        // Reset flag
        windowResized = false;
    }

    // Toggle mouse with escape key
    if (input.IsKeyJustDown(GLFW_KEY_ESCAPE))
    {
        if (input.IsMouseCaptured())
        {
            input.ReleaseMouse();
        }
        else
        {
            input.CaptureMouse();
        }
    }

    // Move the guy
    Transform* transform = node->Get<Transform>();
    // transform->SetPositionXYZ(cos(programLifetime) * 4, sin(programLifetime) * 4, sin(programLifetime) * 4);
    // transform->RotateXYZDeg(0.0f, -90.0f * delta, 0.0f);

    // Update all nodes / components in the scene
    scene.Update(delta);
    
    // Display GUI windows
    ShowEditorWindow();
    ShowDebug();
}

void ParticleEffectEditor::Render()
{
    scene.Render();
}

void ParticleEffectEditor::ShowEditorWindow()
{
    ImGui::Begin("Effect Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Main menu bar
    bool closePopupFlag = false;
    bool newEffectFlag = false;
    bool loadEffectFlag = false;
    bool loadEmitterFlag = false;
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New"))
            {
                // Create a new effect or reset
                if (currentEffect)
                {
                    newEffectFlag = true;
                }
                else
                {
                    currentEffect = &node->AddComponent<CPUParticleEffect>();
                }
            }
            if (ImGui::MenuItem("Load Effect"))
            {
                // Ensure unsaved changes are presented to user
                if (currentEffect)
                {
                    loadEffectFlag = true;
                }
                else
                {
                    // Load a new effect from disk
                    auto effectFile = pfd::open_file("Load Effect File", File::GetDataPath() + "effects", {"Effect Files (.effect)", "*.effect"}, pfd::opt::none);
                    if (effectFile.result().size() > 0)
                    {
                        // Grab the effect path, and convert it to the proper format
                        auto effectPath = std::filesystem::path(effectFile.result()[0]).generic_string();

                        // Ensure an effect is loaded
                        if (!currentEffect) currentEffect = &node->AddComponent<CPUParticleEffect>();

                        // Load from the file
                        currentEffect->Load(effectPath);
                    }
                    lastTime = glfwGetTime();
                }
            }
            if (ImGui::MenuItem("Load Emitter"))
            {   
                // Load the emitter
                auto emitterFile = pfd::open_file("Select Emitter File", File::GetDataPath() + "effects", {"Emitter Files (.emitter)", "*.emitter"}, pfd::opt::none);
                if (emitterFile.result().size() > 0)
                {
                    // Ensure there is an effect loaded
                    if (!currentEffect) currentEffect = &node->AddComponent<CPUParticleEffect>();

                    // Load the file
                    auto emitterPath = std::filesystem::path(emitterFile.result()[0]).generic_string();
                    currentEffect->loadedEmitters.emplace_back(emitterPath);
                }
                lastTime = glfwGetTime();
            }
            if (ImGui::MenuItem("Save (single file)"))
            {
                if (currentEffect)
                {
                    // Save the current effect to a single interleaved effect file containing emitter data
                    auto saveFile = pfd::save_file("Save Interleaved Effect File", File::GetDataPath() + "effects", {"Effect Files (.effect)", "*.effect"}, pfd::opt::none);
                    
                    if (saveFile.result().size() > 0)
                    {
                        // Grab the path in proper format
                        auto savePath = std::filesystem::path(saveFile.result()).generic_string();
                        currentEffect->Save(savePath, true);
                    }
                    lastTime = glfwGetTime();
                }
            }
            if (ImGui::MenuItem("Save (separate emitters)"))
            {
                if (currentEffect)
                {
                    // Save the current effect to a file that references separate emitter files
                    auto saveFile = pfd::save_file("Save Effect", File::GetDataPath() + "effects", {"Effect Files (.effect)", "*.effect"}, pfd::opt::none);

                    if (saveFile.result().size() > 0)
                    {
                        // Grab the path in proper format
                        auto savePath = std::filesystem::path(saveFile.result()).generic_string();
                        currentEffect->Save(savePath, false);
                    }
                    lastTime = glfwGetTime();
                }
            }
            if (ImGui::MenuItem("Close"))
            {
                // Close the current loaded effect
                if (currentEffect)
                {
                    closePopupFlag = true;
                }
                else
                {
                    node->RemoveComponent<CPUParticleEffect>();
                    currentEffect = nullptr;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Open the close or new modal if necessary
    if (closePopupFlag) ImGui::OpenPopup("Close");
    if (newEffectFlag) ImGui::OpenPopup("New");
    if (loadEffectFlag) ImGui::OpenPopup("Load");

    // Close confirmation popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Close", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Close the current effect?\nAny unsaved changes will be lost!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(128, 0)))
        {
            node->RemoveComponent<CPUParticleEffect>();
            currentEffect = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(128, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // New effect confirmation popup
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("New", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Create a new effect?\nAny unsaved changes will be lost!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(128, 0)))
        {
            currentEffect->Reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(128, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Load effect confirmation popup
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Load", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Load a new effect?\nAny unsaved changes will be lost!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(128, 0)))
        {
            // Load a new effect from disk
            auto effectFile = pfd::open_file("Select Effect File", File::GetDataPath() + "effects", {"Effect Files (.effect)", "*.effect"}, pfd::opt::none);

            // Grab the path in proper format and load
            if (effectFile.result().size() > 0)
            {
                auto effectPath = std::filesystem::path(effectFile.result()[0]).generic_string();
                currentEffect->Load(effectPath);
            }

            // Account for time lost and close popup
            lastTime = glfwGetTime();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(128, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // Main editor
    if (currentEffect)
    {
        // Controls
        ImGui::SeparatorText("Effect Controls");
        if (ImGui::Button("Play")) currentEffect->Play();
        ImGui::SameLine();
        if (ImGui::Button("Pause")) currentEffect->Pause();
        ImGui::SameLine();
        if (ImGui::Button("Stop")) currentEffect->Stop();
        ImGui::SameLine();
        if (ImGui::Button("Restart")) currentEffect->Restart();

        // Properties
        ImGui::SeparatorText("Effect Properties");
        ImGui::InputText("Name", &currentEffect->name);
        ImGui::Checkbox("Spawn Relative to Transform", &currentEffect->spawnRelativeTransform);
        ImGui::Checkbox("Render Relative to Transform", &currentEffect->renderRelativeTransform);

        // Emitters
        ImGui::SeparatorText("Emitters");

        // Add emitter button
        if (ImGui::Button("Add"))
        {
            currentEffect->loadedEmitters.emplace_back();
        }

        // Emitter list
        bool keepEmitter = true;
        for (int i = 0; i < currentEffect->loadedEmitters.size(); ++i)
        {
            // Grab a reference to the emitter
            auto& emitter = currentEffect->loadedEmitters[i];

            // Push a unique identifier to the ID stack
            ImGui::PushID(&emitter);

            if (ImGui::CollapsingHeader((emitter.name + "###").c_str(), &keepEmitter, ImGuiTreeNodeFlags_None))
            {
                // Load default selections
                const char* blendSelection = blendOptions[(int)emitter.blendMode];
                const char* spawnSelection = spawnOptions[(int)emitter.particleProperties.spawnMode];
                const char* positionSelection = positionOptions[(int)emitter.particleProperties.positionMode];
                const char* velocitySelection = velocityOptions[(int)emitter.particleProperties.velocityMode];
                const char* colorSelection = colorOptions[(int)emitter.particleProperties.colorMode];
                const char* sizeSelection = sizeOptions[(int)emitter.particleProperties.sizeMode];
                const char* opacitySelection = opacityOptions[(int)emitter.particleProperties.opacityMode];
                const char* lifespanSelection = lifespanOptions[(int)emitter.particleProperties.lifespanMode];
                
                // Display global properties
                ImGui::SeparatorText("Emitter Properties");
                ImGui::InputText("Name", &emitter.name);
                ImGui::DragFloat("Duration", &emitter.duration, 0.001f, -1.0f, 65'536.0f);
                if (ImGui::DragInt("Max Particles", &emitter.maxActiveParticles, 1, 0, CPUParticleEmitter::MAX_PARTICLES))
                {
                    // Adjust particle pool and ensure stable simulation
                    emitter.particlePool.resize(emitter.maxActiveParticles);
                    if (emitter.activeParticles > emitter.maxActiveParticles)
                    {
                        emitter.activeParticles = emitter.maxActiveParticles;
                    }
                }
                ImGui::DragFloat3("Offset", &emitter.offset[0], 0.001f, 0.0f, 0.0f);

                // Seed
                ImGui::Checkbox("Random Seed", &emitter.randomSeed);
                if (!emitter.randomSeed)
                {
                    int seed = emitter.rng.GetSeed();
                    int oldSeed = seed;
                    ImGui::DragInt("Seed", &seed, 1.0f, INT32_MIN, INT32_MAX);
                    if (seed != oldSeed) emitter.rng.SetSeed(seed);
                }

                // Display rendering properties
                ImGui::SeparatorText("Rendering Properties");
                if (ImGui::BeginCombo("Blend Mode", blendSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(blendOptions); n++)
                    {
                        bool is_selected = (blendSelection == blendOptions[n]);
                        if (ImGui::Selectable(blendOptions[n], is_selected))
                        {
                            emitter.blendMode = (CPUParticleEmitter::BlendMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::InputText("Texture", &emitter.texPath, ImGuiInputTextFlags_ReadOnly);
                if (ImGui::Button("Load"))
                {
                    // Load a new texture
                    auto startingPath = File::GetDataPath() + "textures/particles";
                    auto texFile = pfd::open_file("Select Texture File", startingPath, {"Textures (.png)", "*.png"}, pfd::opt::none);
                    
                    if (texFile.result().size() > 0)
                    {
                        // Grab the path in proper format, localize if possible, and set the texture
                        auto texPath = File::LocalizePath(std::filesystem::path(texFile.result()[0]).generic_string());
                        emitter.SetTexture(texPath);
                    }

                    lastTime = glfwGetTime();
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    emitter.RemoveTexture();
                }

                // Display particle properties
                ImGui::SeparatorText("Particle Properties");
                if (ImGui::BeginCombo("Spawn Mode", spawnSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(spawnOptions); n++)
                    {
                        bool is_selected = (spawnSelection == spawnOptions[n]);
                        if (ImGui::Selectable(spawnOptions[n], is_selected))
                        {
                            emitter.particleProperties.spawnMode = (CPUParticleEmitter::SpawnMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Show spawn rates if necessary
                if (emitter.particleProperties.spawnMode == CPUParticleEmitter::SpawnMode::Continuous ||
                    emitter.particleProperties.spawnMode == CPUParticleEmitter::SpawnMode::ContinuousBurst)
                {
                    ImGui::DragFloat("Spawn Rate", &emitter.particleProperties.spawnRate, 0.001f, 0.0f, 65'536.0f);
                }
                else if (emitter.particleProperties.spawnMode != CPUParticleEmitter::SpawnMode::SingleBurst)
                {
                    // Only display min and max if the mode is one of the random ones
                    ImGui::DragFloat("Spawn Rate Min", &emitter.particleProperties.spawnRateMin, 0.001f, 0.0f, 65'536.0f);
                    ImGui::DragFloat("Spawn Rate Max", &emitter.particleProperties.spawnRateMax, 0.001f, 0.0f, 65'536.0f);
                }

                // Show the burst counts if necessary
                if (emitter.particleProperties.spawnMode == CPUParticleEmitter::SpawnMode::ContinuousBurst ||
                    emitter.particleProperties.spawnMode == CPUParticleEmitter::SpawnMode::SingleBurst)
                {
                    ImGui::DragInt("Burst Count", &emitter.particleProperties.burstCount, 1, 1, CPUParticleEmitter::MAX_PARTICLES);
                }
                else if (emitter.particleProperties.spawnMode == CPUParticleEmitter::SpawnMode::RandomBurst)
                {
                    ImGui::DragInt("Burst Count Min", &emitter.particleProperties.burstCountMin, 1, 1, CPUParticleEmitter::MAX_PARTICLES);
                    ImGui::DragInt("Burst Count Max", &emitter.particleProperties.burstCountMax, 1, 1, CPUParticleEmitter::MAX_PARTICLES);
                }

                // Position properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Position Mode", positionSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(positionOptions); n++)
                    {
                        bool is_selected = (positionSelection == positionOptions[n]);
                        if (ImGui::Selectable(positionOptions[n], is_selected))
                        {
                            emitter.particleProperties.positionMode = (CPUParticleEmitter::PositionMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (emitter.particleProperties.positionMode == CPUParticleEmitter::PositionMode::Constant)
                {
                    ImGui::DragFloat3("Position", &emitter.particleProperties.position[0], 0.001f, 0.0f, 0.0f);
                }
                else if (emitter.particleProperties.positionMode == CPUParticleEmitter::PositionMode::RandomMinMax)
                {
                    ImGui::DragFloat3("Position Min", &emitter.particleProperties.positionMin[0], 0.001f, 0.0f, 0.0f);
                    ImGui::DragFloat3("Position Max", &emitter.particleProperties.positionMax[0], 0.001f, 0.0f, 0.0f);
                }
                else
                {
                    ImGui::DragFloat3("Position", &emitter.particleProperties.position[0], 0.001f, 0.0f, 0.0f);
                    ImGui::DragFloat("Radius", &emitter.particleProperties.spawnRadius, 0.001f, 0.0f, 0.0f);
                }

                // Velocity properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Velocity Mode", velocitySelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(velocityOptions); n++)
                    {
                        bool is_selected = (velocitySelection == velocityOptions[n]);
                        if (ImGui::Selectable(velocityOptions[n], is_selected))
                        {
                            emitter.particleProperties.velocityMode = (CPUParticleEmitter::VelocityMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (emitter.particleProperties.velocityMode == CPUParticleEmitter::VelocityMode::Constant)
                {
                    ImGui::DragFloat3("Velocity", &emitter.particleProperties.velocity[0], 0.001f, 0.0f, 0.0f);
                }
                else if (emitter.particleProperties.velocityMode == CPUParticleEmitter::VelocityMode::RandomMinMax)
                {
                    ImGui::DragFloat3("Velocity Min", &emitter.particleProperties.velocityMin[0], 0.001f, 0.0f, 0.0f);
                    ImGui::DragFloat3("Velocity Max", &emitter.particleProperties.velocityMax[0], 0.001f, 0.0f, 0.0f);
                }
                ImGui::DragFloat("Damping", &emitter.particleProperties.damping, 0.001f, 0.0f, 1.0f);

                // Color properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Color Mode", colorSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(colorOptions); n++)
                    {
                        bool is_selected = (colorSelection == colorOptions[n]);
                        if (ImGui::Selectable(colorOptions[n], is_selected))
                        {
                            emitter.particleProperties.colorMode = (CPUParticleEmitter::ColorMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                switch (emitter.particleProperties.colorMode)
                {
                    case CPUParticleEmitter::ColorMode::Constant:
                        ImGui::ColorEdit3("Color", &emitter.particleProperties.color[0]);
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomMinMax:
                        ImGui::ColorEdit3("Color Min", &emitter.particleProperties.colorMin[0]);
                        ImGui::ColorEdit3("Color Max", &emitter.particleProperties.colorMax[0]);
                        break;
                    
                    case CPUParticleEmitter::ColorMode::RandomLerp:
                        ImGui::ColorEdit3("Color A", &emitter.particleProperties.colorA[0]);
                        ImGui::ColorEdit3("Color B", &emitter.particleProperties.colorB[0]);
                        break;
                    
                    case CPUParticleEmitter::ColorMode::LerpOverLifetime:
                        ImGui::ColorEdit3("Start Color", &emitter.particleProperties.startColor[0]);
                        ImGui::ColorEdit3("End Color", &emitter.particleProperties.endColor[0]);
                        break;
                }

                // Size properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Size Mode", sizeSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(sizeOptions); n++)
                    {
                        bool is_selected = (sizeSelection == sizeOptions[n]);
                        if (ImGui::Selectable(sizeOptions[n], is_selected))
                        {
                            emitter.particleProperties.sizeMode = (CPUParticleEmitter::SizeMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                switch (emitter.particleProperties.sizeMode)
                {
                    case CPUParticleEmitter::SizeMode::Constant:
                        ImGui::DragFloat2("Size", &emitter.particleProperties.size[0], 0.001f, 0.0f, 1024.0f);
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomMinMax:
                        ImGui::DragFloat2("Size Min", &emitter.particleProperties.sizeMin[0], 0.001f, 0.0f, 1024.0f);
                        ImGui::DragFloat2("Size Max", &emitter.particleProperties.sizeMax[0], 0.001f, 0.0f, 1024.0f);
                        break;
                    
                    case CPUParticleEmitter::SizeMode::RandomLerp:
                        ImGui::DragFloat2("Size Min", &emitter.particleProperties.sizeMin[0], 0.001f, 0.0f, 1024.0f);
                        ImGui::DragFloat2("Size Max", &emitter.particleProperties.sizeMax[0], 0.001f, 0.0f, 1024.0f);
                        break;
                    
                    case CPUParticleEmitter::SizeMode::LerpOverLifetime:
                        ImGui::DragFloat2("Start Size", &emitter.particleProperties.startSize[0], 0.001f, 0.0f, 1024.0f);
                        ImGui::DragFloat2("End Size", &emitter.particleProperties.endSize[0], 0.001f, 0.0f, 1024.0f);
                        break;   
                }

                // Opacity properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Opacity Mode", opacitySelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(opacityOptions); n++)
                    {
                        bool is_selected = (opacitySelection == opacityOptions[n]);
                        if (ImGui::Selectable(opacityOptions[n], is_selected))
                        {
                            emitter.particleProperties.opacityMode = (CPUParticleEmitter::OpacityMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                switch (emitter.particleProperties.opacityMode)
                {
                    case CPUParticleEmitter::OpacityMode::Constant:
                        ImGui::DragFloat("Opacity", &emitter.particleProperties.opacity, 0.001f, 0.0f, 1.0f);
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::RandomMinMax:
                        ImGui::DragFloat("Opacity Min", &emitter.particleProperties.opacityMin, 0.001f, 0.0f, 1.0f);
                        ImGui::DragFloat("Opacity Max", &emitter.particleProperties.opacityMax, 0.001f, 0.0f, 1.0f);
                        break;
                    
                    case CPUParticleEmitter::OpacityMode::LerpOverLifetime:
                        ImGui::DragFloat("Start Opacity", &emitter.particleProperties.startOpacity, 0.001f, 0.0f, 1.0f);
                        ImGui::DragFloat("End Opacity", &emitter.particleProperties.endOpacity, 0.001f, 0.0f, 1.0f);
                        break;
                }

                // Lifespan properties
                ImGui::Separator();
                if (ImGui::BeginCombo("Lifespan Mode", lifespanSelection))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(lifespanOptions); n++)
                    {
                        bool is_selected = (lifespanSelection == lifespanOptions[n]);
                        if (ImGui::Selectable(lifespanOptions[n], is_selected))
                        {
                            emitter.particleProperties.lifespanMode = (CPUParticleEmitter::LifespanMode)n;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (emitter.particleProperties.lifespanMode == CPUParticleEmitter::LifespanMode::Constant)
                {
                    ImGui::DragFloat("Lifespan", &emitter.particleProperties.lifespan, 0.001f, 0.0f, 65'536.0f);
                }
                else if (emitter.particleProperties.lifespanMode == CPUParticleEmitter::LifespanMode::RandomMinMax)
                {
                    ImGui::DragFloat("Lifespan Min", &emitter.particleProperties.lifespanMin, 0.001f, 0.0f, 65'536.0f);
                    ImGui::DragFloat("Lifespan Max", &emitter.particleProperties.lifespanMax, 0.001f, 0.0f, 65'536.0f);
                }

                // Display all affectors

                // Basic affectors
                ImGui::SeparatorText("Affectors");
                ImGui::Checkbox("Add Velocity", &emitter.affectorProperties.addVelocity);
                ImGui::Checkbox("Gravity", &emitter.affectorProperties.gravityEnabled);

                // Attractors
                ImGui::SeparatorText("Attractors");
                if (ImGui::Button("Add"))
                {
                    emitter.Attractors().emplace_back();
                }
                for (int i = 0; i < emitter.attractors.size(); ++i)
                {
                    // Grab attractor reference
                    auto& a = emitter.attractors[i];
                    bool keepAttractor = true;

                    ImGui::PushID(&a);
                    if (ImGui::CollapsingHeader(("Attractor " + std::to_string(i) + "###").c_str(), &keepAttractor, ImGuiTreeNodeFlags_None))
                    {
                        ImGui::Checkbox("Relative to Transform", &a.relativeToTransform);
                        ImGui::DragFloat3("Position", &a.position.x, 0.001f, 0.0f, 0.0f);
                        ImGui::DragFloat("Radius", &a.radius, 0.001f, 0.0f, 16'384.0f);
                        ImGui::DragFloat("Strength", &a.strength, 0.001f, 0.0f, 0.0f);
                    }

                    // Delete the attractor if requested
                    if (!keepAttractor)
                    {
                        emitter.Attractors().erase(emitter.Attractors().begin() + i);
                        i--;
                        keepAttractor = true;
                    }

                    // Pop the attractor ID from the stack
                    ImGui::PopID();
                }
            }

            // Delete the emitter if requested
            if (!keepEmitter)
            {
                currentEffect->loadedEmitters.erase(currentEffect->loadedEmitters.begin() + i);
                i--;
                keepEmitter = true;
            }

            // Pop the emitter ID from the stack
            ImGui::PopID();
        }
    }

    ImGui::End();
}