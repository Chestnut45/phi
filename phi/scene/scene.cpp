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
        // Add the default materials
        // Guaranteed to have the ID 0
        RegisterMaterial("default", PBRMaterial());
        RegisterMaterial("default", VoxelMaterial());

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
        int alignedSize = ( UBO_ALIGNMENT == 0 ) ? gbSize : ceil( (float)gbSize / (float)UBO_ALIGNMENT ) * UBO_ALIGNMENT;
        globalLightBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, alignedSize);

        // Empty vertex attributes object for attributeless rendering
        glGenVertexArrays(1, &dummyVAO);

        // Load shaders

        // Global lighting shaders

        // PBR
        globalLightPBRShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        globalLightPBRShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pbr.fs");
        globalLightPBRShader.Link();

        // PBR with SSAO
        globalLightPBRSSAOShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        globalLightPBRSSAOShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pbr_ssao.fs");
        globalLightPBRSSAOShader.Link();

        // SSAO
        ssaoShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        ssaoShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/ssao_pass.fs");
        ssaoShader.Link();

        // Light scattering shaders
        lightScatteringShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        lightScatteringShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/light_scatter.fs");
        lightScatteringShader.Link();
        lightTransferShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        lightTransferShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/light_transfer.fs");
        lightTransferShader.Link();

        // Tone map shader
        toneMapShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        toneMapShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/tone_map.fs");
        toneMapShader.Link();

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
        delete rTexFinal;

        // Geometry buffer / textures
        delete gBuffer;
        delete gTexNormal;
        delete gTexAlbedo;
        delete gTexEmissive;
        delete gTexMetallicRoughness;
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
        
        // Generate name
        std::string name = "Node " + std::to_string(nodeCount++);

        // Construct the node and return a pointer
        return &registry.emplace<Node>(id, *this, id, name);
    }

    Node* Scene::CreateNode3D()
    {
        // Construct the node
        Node* node = CreateNode();

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
        for (auto&&[_, effect] : registry.view<CPUParticleEffect>().each())
        {
            effect.Update(delta);
        }

        // Update all voxel objects
        for (auto&&[_, voxelObject] : registry.view<VoxelObject>().each())
        {
            voxelObject.Update(delta);
            
            // Draw aabbs if requested
            if (debugDrawing) Debug::Instance().DrawAABB(voxelObject.GetAABB());
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

        // Update timing
        totalElapsedTime += delta;
    }

    void Scene::Render()
    {
        // Only render if we have an active camera
        if (!activeCamera) return;

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
        renderTarget->Bind(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);

        // Disable depth testing / writing
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_FALSE);

        // Bind the empty VAO for attributeless rendering
        glBindVertexArray(dummyVAO);

        // Ensure no stencil values are updated
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        // Bind the geometry buffer textures
        gTexNormal->Bind(0);
        gTexAlbedo->Bind(1);
        gTexEmissive->Bind(2);
        gTexMetallicRoughness->Bind(3);
        gTexDepthStencil->Bind(4);

        // SSAO pass (only runs if there are any viable rendered components)
        if (ssao && pbrPass)
        {
            // Bind resources
            ssaoFBO->Bind(GL_DRAW_FRAMEBUFFER);
            ssaoKernelUBO->BindBase(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::SSAO);
            ssaoRotationTexture->Bind(5);
            ssaoShader.Use();

            // Draw fullscreen triangle
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // Unbind
            renderTarget->Bind(GL_DRAW_FRAMEBUFFER);

            // Bind the SSAO texture for the lighting pass
            ssaoScreenTexture->Bind(5);
        }

        // PBR Materials global lighting pass
        if (pbrPass)
        {
            // Global light pass
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
            activeEnvironment->Render(activeCamera);

            if (activeEnvironment->renderSun)
            {
                // Bind the proper FBO and render the sun
                sunlightFBO->Bind(GL_DRAW_FRAMEBUFFER);
                glClear(GL_COLOR_BUFFER_BIT);
                activeEnvironment->RenderSun();

                // Apply light scattering post-process effect
                renderTarget->Bind(GL_DRAW_FRAMEBUFFER);
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);
                sunlightTexture->Bind(5);
                
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
            }
        }

        // Particle pass

        // Render all particle effects
        for (auto&&[id, effect] : registry.view<CPUParticleEffect>().each())
        {
            effect.Render();
        }
        CPUParticleEffect::FlushRenderQueue();

        // Make sure blending state is disabled
        glDisable(GL_BLEND);

        // Debug visualizations
        Debug::Instance().FlushShapes();

        // Barrier to ensure all texture writes have finished
        glTextureBarrier();
        
        // Final Pass: Tone mapping
        if (renderMode == RenderMode::DefaultFBO) renderTarget->Unbind(GL_DRAW_FRAMEBUFFER);
        toneMapShader.Use();
        rTexColor->Bind(6);
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Re-enable depth writing / testing
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Lock the camera buffer and move to the next section
        activeCamera->GetUBO()->Lock();
        activeCamera->GetUBO()->SwapSections();

        // Clear render queues
        basicMeshRenderQueue.clear();
        voxelMeshRenderQueue.clear();

        // Unbind render target
        renderTarget->Unbind();
    }

    void Scene::SetRenderMode(RenderMode mode)
    {
        renderMode = mode;

        // TODO: This should be cleaner (Ubershaders is the most likely solution...)
        // Set the proper output location for the tonemap shader
        glBindFragDataLocation(toneMapShader.GetProgramID(), (renderMode == RenderMode::DefaultFBO) ? 0 : 1, "finalColor");
        toneMapShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/fullscreen_tri.vs");
        toneMapShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/tone_map.fs");
        toneMapShader.Link();
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

        int id = 0;
        if (it != pbrMaterialIDs.end())
        {
            // Material with provided name exists, replace it
            id = it->second;
            pbrMaterials[id] = material;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            id = pbrMaterials.size();
            pbrMaterialIDs[name] = id;
            pbrMaterials.push_back(material);
        }

        // Write the new material data to the gpu buffer
        pbrMaterialBuffer.SetOffset(sizeof(glm::vec4) * 3 * id);
        pbrMaterialBuffer.Write(glm::vec4{material.color.r, material.color.g, material.color.b, material.color.a});
        pbrMaterialBuffer.Write(glm::vec4{material.emissive.r, material.emissive.g, material.emissive.b, material.emissive.a});
        pbrMaterialBuffer.Write(glm::vec4{material.metallic, material.roughness, 0.0f, 0.0f});

        return id;
    }

    int Scene::RegisterMaterial(const std::string& name, const VoxelMaterial& material)
    {
        // Find if the material exists
        const auto& it = voxelMaterialIDs.find(name);

        if (it != voxelMaterialIDs.end())
        {
            // Material with provided name exists, replace it
            voxelMaterials[it->second] = material;
            return it->second;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            int id = voxelMaterials.size();
            voxelMaterialIDs[name] = id;
            voxelMaterials.push_back(material);
            return id;
        }
    }

    const PBRMaterial& Scene::GetPBRMaterial(int id) const
    {
        if (id < 0 || id >= pbrMaterials.size()) return pbrMaterials[0];
        return pbrMaterials[id];
    }

    const VoxelMaterial& Scene::GetVoxelMaterial(int id) const
    {
        if (id < 0 || id >= voxelMaterials.size()) return voxelMaterials[0];
        return voxelMaterials[id];
    }

    int Scene::GetPBRMaterialID(const std::string& name) const
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

    int Scene::GetVoxelMaterialID(const std::string& name) const
    {
        // Find if the material exists
        const auto& it = voxelMaterialIDs.find(name);

        if (it != voxelMaterialIDs.end())
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

            // Process PBR materials
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
                    std::string name = mat["name"] ? mat["name"].as<std::string>() : "new_material";
                    m.color.r = mat["color"]["r"] ? mat["color"]["r"].as<float>() : m.color.r;
                    m.color.g = mat["color"]["g"] ? mat["color"]["g"].as<float>() : m.color.g;
                    m.color.b = mat["color"]["b"] ? mat["color"]["b"].as<float>() : m.color.b;
                    m.color.a = mat["color"]["a"] ? mat["color"]["a"].as<float>() : m.color.a;
                    m.emissive.r = mat["emissive"]["r"] ? mat["emissive"]["r"].as<float>() : m.emissive.r;
                    m.emissive.g = mat["emissive"]["g"] ? mat["emissive"]["g"].as<float>() : m.emissive.g;
                    m.emissive.b = mat["emissive"]["b"] ? mat["emissive"]["b"].as<float>() : m.emissive.b;
                    m.emissive.a = mat["emissive"]["a"] ? mat["emissive"]["a"].as<float>() : m.emissive.a;
                    m.metallic = mat["metallic"] ? mat["metallic"].as<float>() : m.metallic;
                    m.roughness = mat["roughness"] ? mat["roughness"].as<float>() : m.roughness;

                    // Add the material to the scene
                    RegisterMaterial(name, m);
                }
            }

            // Process voxel materials
            if (node["voxel_materials"])
            {
                const auto& mats = node["voxel_materials"];
                for (int i = 0; i < mats.size(); ++i)
                {
                    // Grab the specific material node
                    const auto& mat = mats[i];

                    // Create the material
                    VoxelMaterial m{};

                    // Load properties from node
                    m.name = mat["name"] ? mat["name"].as<std::string>() : "new_material";
                    m.flammability = mat["flammability"] ? mat["flammability"].as<float>() : m.flammability;

                    // Material flag parsing
                    // TODO: This can be faster
                    for (auto& flag : mat["flags"])
                    {
                        std::string flagName = flag.as<std::string>();
                        if (flagName == "liquid") m.flags |= VoxelMaterial::Flags::Liquid;
                        if (flagName == "fire") m.flags |= VoxelMaterial::Flags::Fire;
                    }

                    // Live load the current ID for the named PBR material
                    m.pbrID = mat["pbr_name"] ? GetPBRMaterialID(mat["pbr_name"].as<std::string>()) : m.pbrID;

                    // Add the material to the scene
                    RegisterMaterial(m.name, m);
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
        if (&camera.GetNode()->GetScene() == this)
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
        if (&environment.GetNode()->GetScene() == this)
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
        if (&map.GetNode()->GetScene() == this)
        {
            RemoveVoxelMap();
            activeVoxelMap = &map;
        }
    }

    void Scene::RemoveVoxelMap()
    {
        activeVoxelMap = nullptr;
    }

    void Scene::ShowDebug(int x, int y, int width, int height)
    {
        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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
                static const auto colorFlags = ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float;
                ImGui::ColorEdit3("Color", &activeEnvironment->sunColor.r, colorFlags);
                ImGui::DragFloat("Ambience", &activeEnvironment->sunAmbient, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Size", &activeEnvironment->sunSize, 0.1f, 0.0f, 16'384.0f);
                ImGui::DragFloat("Distance", &activeEnvironment->sunDistance, 0.1f, 0.0f, 16'384.0f);
                ImGui::DragFloat("Rotation", &activeEnvironment->sunRotation, 0.001f, 0.0f, TAU);
                
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
            delete rTexFinal;
            delete renderTarget;

            delete gBuffer;
            delete gTexNormal;
            delete gTexAlbedo;
            delete gTexEmissive;
            delete gTexMetallicRoughness;
            delete gTexDepthStencil;

            delete ssaoFBO;
            delete ssaoScreenTexture;

            delete sunlightFBO;
            delete sunlightTexture;
        }

        // Draw buffer order
        GLenum drawBuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

        // Create render target textures
        rTexColor = new Texture2D(renderWidth, renderHeight, GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        rTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        rTexFinal = new Texture2D(renderWidth, renderHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create render target FBO and attach textures
        renderTarget = new Framebuffer();
        renderTarget->Bind();
        renderTarget->AttachTexture(rTexColor, GL_COLOR_ATTACHMENT0);
        renderTarget->AttachTexture(rTexFinal, GL_COLOR_ATTACHMENT1);
        renderTarget->AttachTexture(rTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        renderTarget->CheckCompleteness();

        // Set the draw buffers for the render target
        glDrawBuffers(2, drawBuffers);

        // Create geometry buffer textures
        gTexNormal = new Texture2D(renderWidth, renderHeight, GL_RGB16_SNORM, GL_RGB, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexAlbedo = new Texture2D(renderWidth, renderHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexEmissive = new Texture2D(renderWidth, renderHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexMetallicRoughness = new Texture2D(renderWidth, renderHeight, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create geometry buffer and attach textures
        gBuffer = new Framebuffer();
        gBuffer->Bind();
        gBuffer->AttachTexture(gTexNormal, GL_COLOR_ATTACHMENT0);
        gBuffer->AttachTexture(gTexAlbedo, GL_COLOR_ATTACHMENT1);
        gBuffer->AttachTexture(gTexEmissive, GL_COLOR_ATTACHMENT2);
        gBuffer->AttachTexture(gTexMetallicRoughness, GL_COLOR_ATTACHMENT3);
        gBuffer->AttachTexture(gTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        gBuffer->CheckCompleteness();

        // Set draw buffers for geometry buffer
        glDrawBuffers(4, drawBuffers);

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