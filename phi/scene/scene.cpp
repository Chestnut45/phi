#include "scene.hpp"

#include <fstream>
#include <sstream>
#include <imgui/imgui.h>

#include <phi/core/file.hpp>
#include <phi/scene/node.hpp>
#include <phi/scene/components/collision/bounding_sphere.hpp>
#include <phi/scene/components/renderable/cpu_particle_effect.hpp>
#include <phi/scene/components/lighting/point_light.hpp>

namespace Phi
{
    Scene::Scene()
    {
        // Add the default materials
        // Guaranteed to have the ID 0
        AddMaterial("default", BasicMaterial());
        AddMaterial("default", VoxelMaterial());

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

        // Empty vertex attributes object for attributeless rendering
        glGenVertexArrays(1, &dummyVAO);

        // Load shaders

        // Global lighting shaders

        // Basic material
        globalLightBasicShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/global_light_pass.vs");
        globalLightBasicShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pass_basic.fs");
        globalLightBasicShader.Link();

        // Voxel material
        globalLightVoxelShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/global_light_pass.vs");
        globalLightVoxelShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/global_light_pass_voxel.fs");
        globalLightVoxelShader.Link();

        // Wireframes
        wireframeShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/wireframe.vs");
        wireframeShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/wireframe.fs");
        wireframeShader.Link();

        // SSAO
        ssaoShader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/ssao_pass.vs");
        ssaoShader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/ssao_pass.fs");
        ssaoShader.Link();

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
        ssaoRotationTexture = new Texture2D(4, 4, GL_RG32F, GL_RG, GL_FLOAT, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST, false, rotationTextureData);

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
        // Only update if we have an active camera
        if (!activeCamera) return;
        
        // Update the camera
        activeCamera->Update(delta);

        // Update all particle effects
        for (auto&&[id, effect] : registry.view<CPUParticleEffect>().each())
        {
            effect.Update(delta);
        }

        // Perform frustum culling if enabled
        if (cullingEnabled)
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

            for (auto&&[id, voxelObject] : registry.view<VoxelObject>().each())
            {
                voxelObjectRenderQueue.push_back(&voxelObject);
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
        UpdateCameraBuffer();
        cameraBuffer.BindRange(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::Camera, cameraBuffer.GetCurrentSection() * cameraBuffer.GetSize(), cameraBuffer.GetSize());

        // Flags
        bool basicPass = basicMeshRenderQueue.size() > 0;
        bool voxelPass = voxelObjectRenderQueue.size() > 0;

        if (basicPass || voxelPass)
        {
            // Geometry passes

            // Bind the material buffers
            basicMaterialBuffer.BindBase(GL_SHADER_STORAGE_BUFFER, (int)ShaderStorageBindingIndex::BasicMaterial);
            voxelMaterialBuffer.BindBase(GL_SHADER_STORAGE_BUFFER, (int)ShaderStorageBindingIndex::VoxelMaterial);

            // Bind the geometry buffer and clear it
            gBuffer->Bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // Setup stencil state
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        }

        // Basic material pass

        // Render all basic meshes
        glStencilFunc(GL_ALWAYS, (int)StencilValue::BasicMaterial, 0xff);
        for (BasicMesh* mesh : basicMeshRenderQueue)
        {
            mesh->Render();
        }
        BasicMesh::FlushRenderQueue();

        // Voxel material pass

        // Render all voxel objects
        glStencilFunc(GL_ALWAYS, (int)StencilValue::VoxelMaterial, 0xff);
        for (VoxelObject* voxelObject : voxelObjectRenderQueue)
        {
            voxelObject->Render();
        }
        VoxelObject::FlushRenderQueue();

        // TODO: PBR material pass

        // Bind the main render target to draw to
        gBuffer->Unbind(GL_DRAW_FRAMEBUFFER);

        // Lighting passes
        
        // Lighting pass setup
        if (basicPass || voxelPass)
        {
            // Update global light buffer
            globalLightBuffer.Sync();
            globalLightBuffer.BindRange(GL_UNIFORM_BUFFER, (GLuint)UniformBindingIndex::GlobalLights,
                globalLightBuffer.GetCurrentSection() * globalLightBuffer.GetSize(), globalLightBuffer.GetSize());

            // Write all active global lights to the buffer
            int activeLights = 0;
            for (int i = 0; i < (int)DirectionalLight::Slot::NUM_SLOTS; i++)
            {
                DirectionalLight* light = globalLights[i];
                if (light)
                {
                    globalLightBuffer.Write(glm::vec4(light->color, 1.0f));
                    globalLightBuffer.Write(glm::vec4(light->direction, light->ambient));
                    activeLights++;
                }
            }
            globalLightBuffer.SetOffset((sizeof(glm::vec4) * 2) * MAX_DIRECTIONAL_LIGHTS);
            globalLightBuffer.Write(activeLights);

            // Blit the geometry buffer's depth and stencil textures to the main render target
            switch (renderMode)
            {
                case RenderMode::MatchInternalResolution:
                    glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                    glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
                    break;
                
                case RenderMode::CustomViewport:
                    glBlitFramebuffer(0, 0, renderWidth, renderHeight, viewportX, viewportY, viewportX + viewportWidth, viewportY + viewportHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                    glBlitFramebuffer(0, 0, renderWidth, renderHeight, viewportX, viewportY, viewportX + viewportWidth, viewportY + viewportHeight, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
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
            if (ssao)
            {
                // Bind resources
                ssaoFBO->Bind(GL_DRAW_FRAMEBUFFER);
                ssaoKernelUBO->BindBase(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::SSAO);
                ssaoRotationTexture->Bind(3);
                ssaoShader.Use();

                // Issue draw call
                glDrawArrays(GL_TRIANGLES, 0, 3);

                // Unbind (back to default FBO for lighting)
                ssaoFBO->Unbind(GL_DRAW_FRAMEBUFFER);

                // Bind the SSAO texture for the lighting pass
                ssaoScreenTexture->Bind(3);
            }
        }

        // Clear the render target (not needed if using viewport / blit method)
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Use custom viewport instead of renderTarget
        if (renderMode == RenderMode::CustomViewport) glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

        // Basic materials
        if (basicPass)
        {
            globalLightBasicShader.Use();
            glStencilFunc(GL_EQUAL, (GLint)StencilValue::BasicMaterial, 0xff);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Voxel materials
        if (voxelPass)
        {
            globalLightVoxelShader.Use();
            glStencilFunc(GL_EQUAL, (GLint)StencilValue::VoxelMaterial, 0xff);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Lock global light buffer
        globalLightBuffer.Lock();
        globalLightBuffer.SwapSections();

        // Point light pass
        for (auto&&[id, light] : registry.view<PointLight>().each())
        {
            light.Render();
        }
        PointLight::FlushRenderQueue(basicPass, voxelPass);

        // Disable stencil testing
        glDisable(GL_STENCIL_TEST);

        // Environment pass

        // Draw the skybox
        if (activeSkybox) activeSkybox->Render();

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

        // Quadtree bounds
        if (showQuadtree)
        {
            wireframeVAO.Bind();
            wireframeShader.Use();
            glDrawArrays(GL_LINES, 0, wireframeVerts.size());
        }

        // Final blit to default framebuffer
        // TODO: Optimize this!
        // NOTES: Potential pipeline stall here... can't blit until rendering has finished
        // switch (renderMode)
        // {
        //     case RenderMode::MatchInternalResolution:
        //         glBlitNamedFramebuffer(renderTarget->GetID(), 0, 0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        //         break;
            
        //     case RenderMode::CustomViewport:
        //         glBlitNamedFramebuffer(renderTarget->GetID(), 0, 0, 0, renderWidth, renderHeight, viewportX, viewportY, viewportX + viewportWidth, viewportY + viewportHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        //         break;
        // }
        // renderTarget->Unbind();

        // Lock the camera buffer and move to the next section
        cameraBuffer.Lock();
        cameraBuffer.SwapSections();

        // Clear render queues
        basicMeshRenderQueue.clear();
        voxelObjectRenderQueue.clear();
    }

    void Scene::ShowDebug()
    {
        ImGui::Begin("Scene");

        ImGui::SeparatorText("Statistics");
        ImGui::Text("Nodes: %lu", nodeCount);
        ImGui::Text("Voxel objects rendered: %lu", voxelObjectRenderQueue.size());
        ImGui::NewLine();

        ImGui::SeparatorText("Active Camera");
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
        ImGui::NewLine();

        ImGui::SeparatorText("Graphics Settings");
        ImGui::Checkbox("SSAO", &ssao);
        ImGui::NewLine();

        ImGui::SeparatorText("Experimental Settings");
        ImGui::Checkbox("Frustum culling", &cullingEnabled);
        if (cullingEnabled)
        {
            ImGui::Checkbox("Use quadtree for culling", &cullWithQuadtree);
            if (cullWithQuadtree)
            {
                ImGui::Checkbox("Dynamic quadtree", &dynamicQuadtree);
                ImGui::Checkbox("Show quadtree", &showQuadtree);
            }
        }

        ImGui::End();
    }

    int Scene::AddMaterial(const std::string& name, const BasicMaterial& material)
    {
        // Find if the material exists
        const auto& it = basicMaterialIDs.find(name);

        if (it != basicMaterialIDs.end())
        {
            // Material with provided name exists, replace it
            basicMaterials[it->second] = material;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            basicMaterialIDs[name] = basicMaterials.size();
            basicMaterials.push_back(material);

            // Write the new material to the buffer
            basicMaterialBuffer.Sync();
            basicMaterialBuffer.Write(glm::vec4{glm::vec3(material.color), material.shininess});
        }

        return basicMaterialIDs[name];
    }

    int Scene::AddMaterial(const std::string& name, const VoxelMaterial& material)
    {
        // Find if the material exists
        const auto& it = voxelMaterialIDs.find(name);

        if (it != voxelMaterialIDs.end())
        {
            // Material with provided name exists, replace it
            voxelMaterials[it->second] = material;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            voxelMaterialIDs[name] = voxelMaterials.size();
            voxelMaterials.push_back(material);

            // Write the new material to the buffer
            voxelMaterialBuffer.Sync();
            voxelMaterialBuffer.Write(glm::vec4{glm::vec3(material.color), material.shininess});
        }

        return voxelMaterialIDs[name];
    }

    int Scene::GetBasicMaterialID(const std::string& name)
    {
        // Find if the material exists
        const auto& it = basicMaterialIDs.find(name);

        if (it != basicMaterialIDs.end())
        {
            // Material with provided name exists
            return it->second;
        }

        // Does not exist, return default value
        return 0;
    }

    int Scene::GetVoxelMaterialID(const std::string& name)
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

            // Process basic materials
            if (node["basic_materials"])
            {
                const auto& mats = node["basic_materials"];
                for (int i = 0; i < mats.size(); ++i)
                {
                    // Grab the specific material
                    const auto& mat = mats[i];

                    // Create the basic material
                    BasicMaterial m{};

                    // Load properties from node
                    std::string name = mat["name"] ? mat["name"].as<std::string>() : "Unnamed Basic Material";
                    m.color.r = mat["color"]["r"] ? mat["color"]["r"].as<float>() : m.color.r;
                    m.color.g = mat["color"]["g"] ? mat["color"]["g"].as<float>() : m.color.g;
                    m.color.b = mat["color"]["b"] ? mat["color"]["b"].as<float>() : m.color.b;
                    m.shininess = mat["shininess"] ? mat["shininess"].as<float>() : m.shininess;

                    // Add the material to the scene
                    AddMaterial(name, m);
                }
            }

            // Process voxel materials
            if (node["voxel_materials"])
            {
                const auto& mats = node["voxel_materials"];
                for (int i = 0; i < mats.size(); ++i)
                {
                    // Grab the specific material
                    const auto& mat = mats[i];

                    // Create the basic material
                    VoxelMaterial m{};

                    // Load properties from node
                    std::string name = mat["name"] ? mat["name"].as<std::string>() : "Unnamed Voxel Material";
                    m.color.r = mat["color"]["r"] ? mat["color"]["r"].as<float>() : m.color.r;
                    m.color.g = mat["color"]["g"] ? mat["color"]["g"].as<float>() : m.color.g;
                    m.color.b = mat["color"]["b"] ? mat["color"]["b"].as<float>() : m.color.b;
                    m.shininess = mat["shininess"] ? mat["shininess"].as<float>() : m.shininess;

                    // Add the material to the scene
                    AddMaterial(name, m);
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
        // Check if the camera is already attached to a scene
        if (camera.activeScene)
        {
            // If the camera's active scene is this, early out
            if (camera.activeScene == this) return;

            // Remove the camera from the other scene first
            camera.activeScene->RemoveCamera();
        }

        // Detatch current camera if any is attached
        RemoveCamera();

        // Make the camera this scene's active camera
        activeCamera = &camera;
        camera.activeScene = this;
    }

    void Scene::RemoveCamera()
    {
        if (activeCamera)
        {
            activeCamera->activeScene = nullptr;
            activeCamera = nullptr;
        }
    }

    void Scene::SetActiveSkybox(Skybox& skybox)
    {
        // Check if the skybox is already attached to a scene
        if (skybox.activeScene)
        {
            // If the skybox's active scene is this, early out
            if (skybox.activeScene == this) return;

            // Remove the skybox from the other scene first
            skybox.activeScene->RemoveSkybox();
        }

        // Detatch current skybox if any is attached
        RemoveSkybox();

        // Make the skybox this scene's active skybox
        activeSkybox = &skybox;
        skybox.activeScene = this;
    }

    void Scene::RemoveSkybox()
    {
        if (activeSkybox)
        {
            activeSkybox->activeScene = nullptr;
            activeSkybox = nullptr;
        }
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

        // Update camera viewport if render mode matches
        if (activeCamera) activeCamera->SetResolution(width, height);
        RegenerateFramebuffers();
    }

    void Scene::SetViewport(int x, int y, int width, int height)
    {
        if (width < 1 || height < 1)
        {
            Phi::Error("Scene custom viewport width and height must both be positive");
            return;
        }

        viewportX = x;
        viewportY = y;
        viewportWidth = std::clamp(width, 1, MAX_RESOLUTION.x);
        viewportHeight = std::clamp(height, 1, MAX_RESOLUTION.y);
    }

    void Scene::SetRenderMode(RenderMode mode)
    {
        renderMode = mode;
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

        // Create SSAO texture
        ssaoScreenTexture = new Texture2D(renderWidth, renderHeight, GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create SSAO FBO
        ssaoFBO = new Framebuffer();
        ssaoFBO->Bind();
        ssaoFBO->AttachTexture(ssaoScreenTexture, GL_COLOR_ATTACHMENT0);
        ssaoFBO->CheckCompleteness();

        // Set draw buffer
        GLenum buf[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buf);

        // Create geometry buffer textures
        gTexNormal = new Texture2D(renderWidth, renderHeight, GL_RGB16F, GL_RGB, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexMaterial = new Texture2D(renderWidth, renderHeight, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
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

        // Unbind before returning
        gBuffer->Unbind();
    }

    void Scene::UpdateCameraBuffer()
    {
        // Grab camera values
        const glm::mat4& view = activeCamera->GetView();
        const glm::mat4& proj = activeCamera->GetProj();
        glm::mat4 viewProj = proj * view;

        // Ensure we don't write when commands are reading
        cameraBuffer.Sync();

        // Write camera matrix data to UBO
        // Doing this once here on the CPU is a much easier price to pay than per-vertex
        cameraBuffer.Write(viewProj);
        cameraBuffer.Write(glm::inverse(viewProj));
        cameraBuffer.Write(view);
        cameraBuffer.Write(glm::inverse(view));
        cameraBuffer.Write(proj);
        cameraBuffer.Write(glm::inverse(proj));
        cameraBuffer.Write(glm::vec4(activeCamera->GetPosition(), 1.0f));
        cameraBuffer.Write(glm::vec4(viewportX, viewportY, activeCamera->GetWidth() * 0.5f, activeCamera->GetHeight() * 0.5f));
        cameraBuffer.Write(glm::vec4(activeCamera->near, activeCamera->far, 0.0f, 0.0f));
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

        // Rebuild wireframe vertices
        wireframeVerts.clear();
        for (const Rectangle& rect : quadtree.GetRects())
        {
            // Rendered as a list of lines
            wireframeVerts.push_back({rect.left, 0.0f, rect.top});
            wireframeVerts.push_back({rect.right, 0.0f, rect.top});
            wireframeVerts.push_back({rect.right, 0.0f, rect.top});
            wireframeVerts.push_back({rect.right, 0.0f, rect.bottom});
            wireframeVerts.push_back({rect.right, 0.0f, rect.bottom});
            wireframeVerts.push_back({rect.left, 0.0f, rect.bottom});
            wireframeVerts.push_back({rect.left, 0.0f, rect.bottom});
            wireframeVerts.push_back({rect.left, 0.0f, rect.top});
        }

        // Update the buffer
        wireframeBuffer.Overwrite(wireframeVerts.data(), wireframeVerts.size() * sizeof(VertexPos));
    }
}