#include "scene.hpp"

#include <fstream>
#include <sstream>
#include <imgui/imgui.h>

#include <phi/core/math/constants.hpp>
#include <phi/core/file.hpp>
#include <phi/scene/node.hpp>
#include <phi/scene/components/collision/bounding_sphere.hpp>
#include <phi/scene/components/particles/cpu_particle_effect.hpp>
#include <phi/scene/components/lighting/point_light.hpp>

// DEBUG
#include <phi/core/debug.hpp>

namespace Phi
{
    Scene::Scene(int width, int height)
        : renderWidth(width), renderHeight(height)
    {
        // Add the default material
        // Guaranteed to have the ID 0
        RegisterMaterial("default", PBRMaterial());

        // Set all global light pointers to nullptr
        for (int i = 0; i < (int)DirectionalLight::Slot::NUM_SLOTS; ++i)
        {
            globalLights[i] = nullptr;
        }

        // Enable programs
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Back face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // Initialize resources

        // Query UBO alignment
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UBO_ALIGNMENT);

        // Create global light buffer
        int gbSize = (MAX_USER_DIRECTIONAL_LIGHTS + 2) * (sizeof(glm::vec4) * 2);
        int alignedSize = ( UBO_ALIGNMENT == 0 ) ? gbSize : ceil( (float)gbSize / UBO_ALIGNMENT ) * UBO_ALIGNMENT;
        globalLightBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, alignedSize);

        // Empty vertex attributes object for attributeless rendering
        glGenVertexArrays(1, &dummyVAO);

        // Load shaders

        // Global lighting shaders

        // PBR
        globalLightPBRShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/global_light.vs");
        globalLightPBRShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pbr.fs");
        globalLightPBRShader.Link();

        // PBR with SSAO
        globalLightPBRSSAOShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/global_light.vs");
        globalLightPBRSSAOShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pbr_ssao.fs");
        globalLightPBRSSAOShader.Link();

        // SSAO
        ssaoShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/ssao_pass.vs");
        ssaoShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/ssao_pass.fs");
        ssaoShader.Link();

        // Light scattering shaders
        lightScatteringShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/light_scatter.vs");
        lightScatteringShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/light_scatter.fs");
        lightScatteringShader.Link();
        lightTransferShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/light_scatter.vs");
        lightTransferShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/light_transfer.fs");
        lightTransferShader.Link();

        // Initialize SSAO data

        // Generate kernel data
        RNG rng;
        GLfloat kernelData[SSAO_SAMPLE_SIZE * 4];
        for (int i = 0; i < SSAO_SAMPLE_SIZE * 4; i += 4)
        {
            // Generate the sample
            glm::vec3 sample;
            sample.x = rng.NextFloat(-1.0f, 1.0f);
            sample.y = rng.NextFloat(-1.0f, 1.0f);
            sample.z = rng.NextFloat(0.0f, 1.0f);

            // Normalize, distribute, and scale the sample
            sample = glm::normalize(sample);
            float scale = (float)i / SSAO_SAMPLE_SIZE;
            scale = glm::mix(0.1f, 1.0f, scale * scale);
            sample *= scale;

            // Write the data to the buffer
            kernelData[i] = sample.x;
            kernelData[i + 1] = sample.y;
            kernelData[i + 2] = sample.z;
            kernelData[i + 3] = 1.0f;
        }
        
        // Create static buffer
        ssaoKernelUBO = new GPUBuffer(BufferType::Static, sizeof(GLfloat) * 4 * SSAO_SAMPLE_SIZE, kernelData);

        // Generate kernel rotation vector texture
        GLfloat rotationTextureData[32];
        for (int i = 0; i < 32; i += 2)
        {
            rotationTextureData[i] = rng.NextFloat(-1.0f, 1.0f);
            rotationTextureData[i + 1] = rng.NextFloat(-1.0f, 1.0f);
        }

        // Create the texture
        ssaoRotationTexture = new Texture2D(4, 4, GL_RG16F, GL_RG, GL_FLOAT, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST, false, rotationTextureData);

        // Initialize the framebuffers
        RegenerateFramebuffers();
    }

    Scene::~Scene()
    {
        // Ensure camera does not have a dangling pointer to us
        RemoveCamera();

        // Destroy all components / nodes
        registry.clear();

        // Render targets
        delete renderTarget;
        delete rTexColor;
        delete rTexDepthStencil;

        // Geometry buffer / textures
        delete gBuffer;
        delete gTexNormal;
        delete gTexMaterial;
        delete gTexDepthStencil;

        // SSAO resources
        delete ssaoKernelUBO;
        delete ssaoRotationTexture;
        delete ssaoScreenTexture;
        delete ssaoFBO;
    }

    Node* Scene::CreateNode()
    {
        // Create the internal entity
        NodeID id = registry.create();

        // Increase counter
        nodeCount++;

        // Construct the node and return a pointer
        return &registry.emplace<Node>(id, this, id);
    }

    Node* Scene::CreateNode3D()
    {
        // Create the internal entity
        NodeID id = registry.create();

        // Increase counter
        nodeCount++;

        // Construct the node
        Node* node = &registry.emplace<Node>(id, this, id);

        // Add a transform
        node->AddComponent<Transform>();

        // Then return
        return node;
    }

    Node* Scene::Get(NodeID id)
    {
        return registry.try_get<Node>(id);
    }

    void Scene::Delete(NodeID id)
    {
        // Destroy the node and all of its components
        Node* node = Get(id);
        if (node)
        {
            // Traverse hierarchy and delete all child nodes first
            // NOTE: Done in reverse order since the vector is
            // modified immediately on deletion
            for (int i = node->children.size() - 1; i >= 0; i--)
            {
                Delete(node->children[i]->GetID());
            }

            // Remove any dangling references from the hierarchy
            Node* parent = node->GetParent();
            if (parent)
            {
                parent->RemoveChild(node);
            }

            // Destroy the internal entity and all components
            registry.destroy(id);

            // Update counter
            nodeCount--;
        }
    }

    void Scene::Update(float delta)
    {
        // Update active components
        if (activeCamera) activeCamera->Update(delta);
        if (activeEnvironment) activeEnvironment->Update(delta);
        if (activeVoxelMap) activeVoxelMap->Update(delta);

        // Update all particle effects
        for (auto&&[id, effect] : registry.view<CPUParticleEffect>().each())
        {
            effect.Update(delta);
        }

        // Perform frustum culling if enabled
        if (activeCamera && cullingEnabled)
        {
            // Grab the camera's view frustum
            Frustum viewFrustum = activeCamera->GetViewFrustum();

            if (cullWithQuadtree)
            {
                // Rebuild quadtree every frame
                if (dynamicQuadtree) BuildQuadtree();

                // Query quadtree with frustum
                auto found = quadtree.FindElements(viewFrustum);
                for (int i : found)
                {
                    // Grab the bounding sphere and intersection test the actual volume
                    BoundingSphere* s = quadtree.Get(i);
                    if (s->Intersects(viewFrustum)) basicMeshRenderQueue.push_back(s->GetNode()->Get<BasicMesh>());
                }
            }
            else
            {
                // Naively cull every mesh with a bounding volume
                for (auto&&[id, mesh] : registry.view<BasicMesh>().each())
                {
                    BoundingSphere* sphere = mesh.GetNode()->Get<BoundingSphere>();
                    if (sphere && sphere->IsCullingEnabled())
                    {
                        // If the volume exists, only add to render queue if it intersects the view frustum
                        if (sphere->Intersects(viewFrustum)) basicMeshRenderQueue.push_back(&mesh);
                    }
                    else
                    {
                        // If no bounding volume exists, add to render queue
                        basicMeshRenderQueue.push_back(&mesh);
                    }
                }

                // TODO: Cull voxel objects
            }
        }
        else
        {
            // Culling is disabled, draw all meshes
            for (auto&&[id, mesh] : registry.view<BasicMesh>().each())
            {
                basicMeshRenderQueue.push_back(&mesh);
            }
            for (auto&&[id, mesh] : registry.view<VoxelMesh>().each())
            {
                voxelMeshRenderQueue.push_back(&mesh);
            }
        }

        if (debugDrawing)
        {
            for (auto&&[id, object] : registry.view<VoxelObject>().each())
            {
                Debug::Instance().DrawAABB(object.GetAABB(), glm::vec3(1.0f));
            }
        }

        // Update timing
        totalElapsedTime += delta;
    }

    void Scene::Render()
    {
        // Only render if we have an active camera
        if (!activeCamera) return;

        // Grab the framebuffer ID (might not be 0)
        GLint currentFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

        // Set initial rendering state
        glViewport(0, 0, renderWidth, renderHeight);
        glDisable(GL_BLEND);

        // Update the camera buffer right before rendering
        activeCamera->UpdateUBO();
        activeCamera->GetUBO()->BindSectionRange(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::Camera);

        // Pass flags
        bool pbrPass = basicMeshRenderQueue.size() > 0 || voxelMeshRenderQueue.size() > 0;

        // Geometry passes

        // Bind the geometry buffer and clear it
        gBuffer->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (pbrPass)
        {
            // Bind the material buffer
            pbrMaterialBuffer.BindBase(GL_SHADER_STORAGE_BUFFER, (int)ShaderStorageBindingIndex::PBRMaterial);

            // Setup stencil state
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            // PBR material pass
            glStencilFunc(GL_ALWAYS, (int)StencilValue::PBRMaterial, 0xff);

            // Render all basic meshes
            for (BasicMesh* mesh : basicMeshRenderQueue)
            {
                mesh->Render();
            }
            BasicMesh::FlushRenderQueue();

            // Render all voxel meshes
            for (VoxelMesh* mesh : voxelMeshRenderQueue)
            {
                mesh->Render();
            }
            VoxelMesh::FlushRenderQueue();
        }

        // Bind the main render target to draw to
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO);

        // Lighting passes
        
        // Update global light buffer
        globalLightBuffer->Sync();
        globalLightBuffer->BindRange(GL_UNIFORM_BUFFER, (GLuint)UniformBindingIndex::GlobalLights, globalLightBuffer->GetCurrentSection() * globalLightBuffer->GetSize(), globalLightBuffer->GetSize());

        // Write all active global lights to the buffer
        int activeLights = 0;
        for (int i = 0; i < (int)DirectionalLight::Slot::NUM_SLOTS; i++)
        {
            DirectionalLight* light = globalLights[i];
            if (light)
            {
                globalLightBuffer->Write(glm::vec4(light->color, 1.0f));
                globalLightBuffer->Write(glm::vec4(light->direction, light->ambient));
                activeLights++;
            }
        }

        // Write the sun if it's active
        if (activeEnvironment && activeEnvironment->renderSun)
        {
            globalLightBuffer->Write(glm::vec4(activeEnvironment->sunColor, 1.0f));
            globalLightBuffer->Write(glm::vec4(glm::normalize(-activeEnvironment->sunPos), activeEnvironment->sunAmbient));
            activeLights++;
        }

        // Write number of lights and base ambient light value
        globalLightBuffer->SetOffset((sizeof(glm::vec4) * 2) * (MAX_USER_DIRECTIONAL_LIGHTS + 1));
        globalLightBuffer->Write(glm::ivec4(activeLights));
        globalLightBuffer->Write(ambientLight);

        // Blit the geometry buffer's depth and stencil textures to the main render target
        // TODO: Is a blit fastest here? Profile...
        switch (renderMode)
        {
            case RenderMode::MatchInternalResolution:
                glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
                break;
        }

        // Disable depth testing / writing
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_FALSE);

        // Bind the empty VAO for attributeless rendering
        glBindVertexArray(dummyVAO);

        // Ensure no stencil values are updated
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        // Bind the geometry buffer textures
        gTexNormal->Bind(0);
        gTexMaterial->Bind(1);
        gTexDepthStencil->Bind(2);

        // SSAO pass (only runs if there are any viable rendered components)
        if (ssao && pbrPass)
        {
            // Bind resources
            ssaoFBO->Bind(GL_DRAW_FRAMEBUFFER);
            ssaoKernelUBO->BindBase(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::SSAO);
            ssaoRotationTexture->Bind(3);
            ssaoShader.Use();

            // Draw fullscreen triangle
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // Unbind (back to default FBO for lighting)
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO);

            // Bind the SSAO texture for the lighting pass
            ssaoScreenTexture->Bind(3);
        }

        // PBR Materials global lighting pass
        if (pbrPass)
        {
            // Use correct shader based on SSAO state
            ssao ? globalLightPBRSSAOShader.Use() : globalLightPBRShader.Use();
            glStencilFunc(GL_EQUAL, (GLint)StencilValue::PBRMaterial, 0xff);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Lock global light buffer
        globalLightBuffer->Lock();
        globalLightBuffer->SwapSections();

        // Point light pass
        for (auto&&[id, light] : registry.view<PointLight>().each())
        {
            light.Render();
        }
        PointLight::FlushRenderQueue(pbrPass);

        // Disable stencil testing
        glDisable(GL_STENCIL_TEST);

        // Environment pass

        if (activeEnvironment) 
        {
            // Render the skybox texture
            activeEnvironment->RenderSkybox();

            if (activeEnvironment->renderSun)
            {
                // Bind the proper FBO and render the sun
                sunlightFBO->Bind(GL_DRAW_FRAMEBUFFER);
                glClear(GL_COLOR_BUFFER_BIT);
                activeEnvironment->RenderSun();

                // Apply light scattering post-process effect
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO);
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);
                sunlightTexture->Bind(4);
                
                if (activeEnvironment->godRays)
                {
                    // Upload uniforms
                    lightScatteringShader.Use();
                    glm::vec4 sunPosScreen = activeCamera->proj * activeCamera->view * glm::vec4(activeEnvironment->sunPos + activeCamera->position, 1.0f);
                    glm::vec2 lightPos = glm::vec2(sunPosScreen) / sunPosScreen.w * 0.5f + 0.5f;
                    lightScatteringShader.SetUniform("lightPos", lightPos);
                    lightScatteringShader.SetUniform("exposureDecayDensityWeight", glm::vec4(activeEnvironment->exposure, activeEnvironment->decay, activeEnvironment->density, activeEnvironment->weight));
                }
                else
                {
                    lightTransferShader.Use();
                }

                // Bind and draw
                glBindVertexArray(dummyVAO);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);
                glDisable(GL_BLEND);
            }
        }

        // Particle pass

        // Render all particle effects
        for (auto&&[id, effect] : registry.view<CPUParticleEffect>().each())
        {
            effect.Render();
        }
        CPUParticleEffect::FlushRenderQueue();

        // Re-enable depth writing
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Wireframe pass

        // Debug visualizations
        Debug::Instance().FlushShapes();

        // Final blit to default framebuffer
        // TODO: Optimize this!
        // NOTES: Potential pipeline stall here... can't blit until rendering has finished
        // switch (renderMode)
        // {
        //     case RenderMode::MatchInternalResolution:
        //         glBlitNamedFramebuffer(renderTarget->GetID(), 0, 0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        //         break;
        // }
        // renderTarget->Unbind();

        // Lock the camera buffer and move to the next section
        activeCamera->GetUBO()->Lock();
        activeCamera->GetUBO()->SwapSections();

        // Clear render queues
        basicMeshRenderQueue.clear();
        voxelMeshRenderQueue.clear();
    }

    void Scene::SetRenderMode(RenderMode mode)
    {
        renderMode = mode;
    }

    void Scene::SetResolution(int width, int height)
    {
        if (width < 1 || height < 1)
        {
            Phi::Error("Scene width and height must both be positive");
            return;
        }

        this->renderWidth = width;
        this->renderHeight = height;

        // Update camera viewport
        if (activeCamera) activeCamera->SetResolution(width, height);
        RegenerateFramebuffers();
    }

    int Scene::RegisterMaterial(const std::string& name, const PBRMaterial& material)
    {
        // Find if the material exists
        const auto& it = pbrMaterialIDs.find(name);

        if (it != pbrMaterialIDs.end())
        {
            // Material with provided name exists, replace it
            pbrMaterials[it->second] = material;

            // Write the new material data to the gpu buffer
            pbrMaterialBuffer.SetOffset(sizeof(glm::vec4) * 2 * it->second);
            pbrMaterialBuffer.Write(glm::vec4{material.color, 1.0f});
            pbrMaterialBuffer.Write(glm::vec4{material.metallic, material.roughness, 1.0f, 1.0f});

            return it->second;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            int id = pbrMaterials.size();
            pbrMaterialIDs[name] = id;
            pbrMaterials.push_back(material);

            // Write the new material data to the gpu buffer
            pbrMaterialBuffer.SetOffset(sizeof(glm::vec4) * 2 * id);
            pbrMaterialBuffer.Write(glm::vec4{material.color, 1.0f});
            pbrMaterialBuffer.Write(glm::vec4{material.metallic, material.roughness, 1.0f, 1.0f});

            return id;
        }
    }

    const PBRMaterial& Scene::GetPBRMaterial(int id)
    {
        if (id < 0 || id >= pbrMaterials.size()) return pbrMaterials[0];
        return pbrMaterials[id];
    }

    int Scene::GetPBRMaterialID(const std::string& name)
    {
        // Find if the material exists
        const auto& it = pbrMaterialIDs.find(name);

        if (it != pbrMaterialIDs.end())
        {
            // Material with provided name exists
            return it->second;
        }

        // Does not exist, return default value
        return 0;
    }

    void Scene::LoadMaterials(const std::string& path)
    {
        try
        {
            // Load YAML file
            YAML::Node node = YAML::LoadFile(File::GlobalizePath(path));

            // Process pbr materials
            if (node["pbr_materials"])
            {
                const auto& mats = node["pbr_materials"];
                for (int i = 0; i < mats.size(); ++i)
                {
                    // Grab the specific material node
                    const auto& mat = mats[i];

                    // Create the material
                    PBRMaterial m{};

                    // Load properties from node
                    std::string name = mat["name"] ? mat["name"].as<std::string>() : "Unnamed PBR Material";
                    m.color.r = mat["color"]["r"] ? mat["color"]["r"].as<float>() : m.color.r;
                    m.color.g = mat["color"]["g"] ? mat["color"]["g"].as<float>() : m.color.g;
                    m.color.b = mat["color"]["b"] ? mat["color"]["b"].as<float>() : m.color.b;
                    m.metallic = mat["metallic"] ? mat["metallic"].as<float>() : m.metallic;
                    m.roughness = mat["roughness"] ? mat["roughness"].as<float>() : m.roughness;

                    // Add the material to the scene
                    RegisterMaterial(name, m);
                }
            }
        }
        catch (YAML::Exception e)
        {
            Error("YAML Parser Error: ", path);
        }
    }

    void Scene::SetActiveCamera(Camera& camera)
    {
        // Validate the component
        if (camera.GetNode()->GetScene() == this)
        {
            RemoveCamera();
            activeCamera = &camera;
        }
    }

    void Scene::RemoveCamera()
    {
        activeCamera = nullptr;
    }

    void Scene::SetActiveEnvironment(Environment& environment)
    {
        // Validate the component
        if (environment.GetNode()->GetScene() == this)
        {
            RemoveEnvironment();
            activeEnvironment = &environment;
        }
    }

    void Scene::RemoveEnvironment()
    {
        activeEnvironment = nullptr;
    }

    void Scene::SetActiveVoxelMap(VoxelMap& map)
    {
        // Validate the component
        if (map.GetNode()->GetScene() == this)
        {
            RemoveVoxelMap();
            activeVoxelMap = &map;
        }
    }

    void Scene::RemoveVoxelMap()
    {
        activeVoxelMap = nullptr;
    }

    void Scene::ShowDebug()
    {
        ImGui::SetNextWindowPos(ImVec2(renderWidth - 364, renderHeight - 454));
        ImGui::SetNextWindowSize(ImVec2(360, 450));
        ImGui::Begin("Scene Debug", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        ImGui::SeparatorText("Graphics Settings");
        ImGui::Checkbox("SSAO", &ssao);
        ImGui::Checkbox("Debug Drawing", &debugDrawing);

        ImGui::SeparatorText("Environment");
        ImGui::ColorEdit3("Ambient Light", &ambientLight.x);

        if (activeEnvironment)
        {
            ImGui::Text("Timing");
            ImGui::Separator();
            if (ImGui::Button("Sunrise")) activeEnvironment->SetTime(Environment::SUNRISE); ImGui::SameLine();
            if (ImGui::Button("Noon")) activeEnvironment->SetTime(Environment::NOON); ImGui::SameLine();
            if (ImGui::Button("Sunset")) activeEnvironment->SetTime(Environment::SUNSET); ImGui::SameLine();
            if (ImGui::Button("Midnight")) activeEnvironment->SetTime(Environment::MIDNIGHT);
            ImGui::Checkbox("Advance Time", &activeEnvironment->advanceTime);
            ImGui::DragFloat("Time", &activeEnvironment->timeOfDay, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Day Time", &activeEnvironment->dayLength, 1.0f, 0.0f, INT32_MAX);
            ImGui::DragFloat("Night Time", &activeEnvironment->nightLength, 1.0f, 0.0f, INT32_MAX);

            ImGui::Text("Sun:");
            ImGui::Separator();
            ImGui::Checkbox("Render Sun", &activeEnvironment->renderSun);
            if (activeEnvironment->renderSun)
            {
                ImGui::DragFloat("Size", &activeEnvironment->sunSize, 0.1f, 0.0f, 16'384.0f);
                ImGui::DragFloat("Distance", &activeEnvironment->sunDistance, 0.1f, 0.0f, 16'384.0f);
                ImGui::DragFloat("Rotation", &activeEnvironment->sunRotation, 0.001f, 0.0f, TAU);
                ImGui::DragFloat("Ambience", &activeEnvironment->sunAmbient, 0.001f, 0.0f, 1.0f);
                ImGui::ColorEdit3("Color", &activeEnvironment->sunColor.r);
                ImGui::Checkbox("God Rays", &activeEnvironment->godRays);
                if (activeEnvironment->godRays)
                {
                    ImGui::DragFloat("Exposure", &activeEnvironment->exposure, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("Decay", &activeEnvironment->decay, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("Density", &activeEnvironment->density, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("Weight", &activeEnvironment->weight, 0.001f, 0.0f, 1.0f);
                }
            }
        }

        ImGui::SeparatorText("Camera");
        if (activeCamera)
        {
            const glm::vec3& pos = activeCamera->GetPosition();
            const glm::vec3& dir = activeCamera->GetDirection();
            ImGui::Text("Position: (%.1f %.1f, %.1f)", pos.x, pos.y, pos.z);
            ImGui::Text("Direction: (%.1f %.1f, %.1f)", dir.x, dir.y, dir.z);
            ImGui::Text("FOV: %.0f", activeCamera->GetFov());
        }
        else
        {
            ImGui::Text("Null");
        }

        ImGui::End();
    }
    
    void Scene::RegenerateFramebuffers()
    {
        if (gBuffer)
        {
            // Delete framebuffers and textures if they exist
            delete rTexColor;
            delete rTexDepthStencil;
            delete renderTarget;

            delete gBuffer;
            delete gTexNormal;
            delete gTexMaterial;
            delete gTexDepthStencil;

            delete ssaoFBO;
            delete ssaoScreenTexture;

            delete sunlightFBO;
            delete sunlightTexture;
        }

        // Create render target textures
        // rTexColor = new Texture2D(renderWidth, renderHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        // rTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create render target FBO and attach textures
        // renderTarget = new Framebuffer();
        // renderTarget->Bind();
        // renderTarget->AttachTexture(rTexColor, GL_COLOR_ATTACHMENT0);
        // renderTarget->AttachTexture(rTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        // renderTarget->CheckCompleteness();

        // Create geometry buffer textures
        gTexNormal = new Texture2D(renderWidth, renderHeight, GL_RGB16_SNORM, GL_RGB, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexMaterial = new Texture2D(renderWidth, renderHeight, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create geometry buffer and attach textures
        gBuffer = new Framebuffer();
        gBuffer->Bind();
        gBuffer->AttachTexture(gTexNormal, GL_COLOR_ATTACHMENT0);
        gBuffer->AttachTexture(gTexMaterial, GL_COLOR_ATTACHMENT1);
        gBuffer->AttachTexture(gTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        gBuffer->CheckCompleteness();

        // Set draw buffers for geometry buffer
        GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, drawBuffers);

        // Create SSAO texture
        ssaoScreenTexture = new Texture2D(renderWidth, renderHeight, GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create SSAO FBO
        ssaoFBO = new Framebuffer();
        ssaoFBO->Bind();
        ssaoFBO->AttachTexture(ssaoScreenTexture, GL_COLOR_ATTACHMENT0);
        ssaoFBO->CheckCompleteness();

        // Set draw buffer
        glDrawBuffers(1, drawBuffers);

        // Create sun texture
        sunlightTexture = new Texture2D(renderWidth, renderHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create light scattering FBO
        sunlightFBO = new Framebuffer();
        sunlightFBO->Bind();
        sunlightFBO->AttachTexture(sunlightTexture, GL_COLOR_ATTACHMENT0);
        sunlightFBO->AttachTexture(gTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        sunlightFBO->CheckCompleteness();

        // Set draw buffer
        glDrawBuffers(1, drawBuffers);
    }

    void Scene::BuildQuadtree()
    {
        // Remove all elements from the quadtree
        quadtree.Clear();

        // Do one cleanup per frame
        quadtree.Cleanup();

        // Only insert nodes with the proper components to the quadtree
        for (auto&&[id, sphere] : registry.view<BoundingSphere>().each())
        {
            // Don't add bounding spheres that aren't for culling
            if (!sphere.IsCullingEnabled()) continue;

            // Grab the global position and radius of the node
            const glm::vec3& pos = sphere.GetNode()->Get<Transform>()->GetGlobalPosition();
            float r = sphere.GetVolume().radius;

            // Create the bounding rectangle
            Rectangle rect(pos.x - r, pos.z + r, pos.x + r, pos.z - r);

            // Insert into the quadtree
            quadtree.Insert(&sphere, rect);
        }
    }
}