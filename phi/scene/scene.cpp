#include "scene.hpp"

#include <fstream>
#include <sstream>
#include <imgui/imgui.h>

#include <phi/core/file.hpp>
#include <phi/scene/node.hpp>
#include <phi/scene/components/collision/bounding_sphere.hpp>
#include <phi/scene/components/renderable/particles/cpu_particle_effect.hpp>

namespace Phi
{
    Scene::Scene()
    {
        // Add the default material
        // Guaranteed to have the ID 0
        AddMaterial("default", BasicMaterial());

        // Set all global light pointers to nullptr
        for (int i = 0; i < (int)LightSlot::NUM_SLOTS; ++i)
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

        // Global lighting (Blinn-Phong)
        globalLightShader.LoadSource(GL_VERTEX_SHADER, "phi/graphics/shaders/global_light_pass_blinn_phong.vs");
        globalLightShader.LoadSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/global_light_pass_blinn_phong.fs");
        globalLightShader.Link();

        // Wireframes
        wireframeShader.LoadSource(GL_VERTEX_SHADER, "phi/graphics/shaders/wireframe.vs");
        wireframeShader.LoadSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/wireframe.fs");
        wireframeShader.Link();

        // Initialize the framebuffers
        RegenerateFramebuffers();
    }

    Scene::~Scene()
    {
        // Ensure camera does not have a dangling pointer to us
        RemoveCamera();

        // Delete framebuffers and textures
        delete renderTarget;
        delete rTexColor;
        delete rTexDepthStencil;
        delete gBuffer;
        delete gTexPosition;
        delete gTexNormal;
        delete gTexMaterial;
        delete gTexDepthStencil;
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
        for (auto&&[node, effect] : registry.view<CPUParticleEffect>().each())
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
                    if (s->Intersects(viewFrustum)) basicMeshRenderQueue.push_back(s->GetNode()->GetComponent<BasicMesh>());
                }
            }
            else
            {
                // Naively cull every mesh with a bounding volume
                for (auto&&[node, mesh] : registry.view<BasicMesh>().each())
                {
                    BoundingSphere* sphere = mesh.GetNode()->GetComponent<BoundingSphere>();
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
            }
        }
        else
        {
            // Culling is disabled, draw all meshes

            for (auto&&[node, mesh] : registry.view<BasicMesh>().each())
            {
                basicMeshRenderQueue.push_back(&mesh);
            }

            for (auto&&[node, voxelmesh] : registry.view<VoxelMesh>().each())
            {
                voxelMeshRenderQueue.push_back(&voxelmesh);
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

        // Ensure camera and material buffers are bound
        cameraBuffer.BindRange(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::Camera, cameraBuffer.GetCurrentSection() * cameraBuffer.GetSize(), cameraBuffer.GetSize());
        basicMaterialBuffer.BindBase(GL_UNIFORM_BUFFER, (int)UniformBindingIndex::Materials);

        // Geometry passes

        // Bind the geometry buffer and clear it
        gBuffer->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Basic material pass

        // Render all basic meshes
        for (BasicMesh* mesh : basicMeshRenderQueue)
        {
            Transform* transform = mesh->GetNode()->GetComponent<Transform>();
            if (transform)
            {
                mesh->Render(transform->GetGlobalMatrix());
            }
            else
            {
                mesh->Render(); // Draw with no transform (default argument is identity matrix)
            }
        }
        BasicMesh::FlushRenderQueue();
        basicMeshRenderQueue.clear();

        // Render all voxel meshes
        for (VoxelMesh* mesh : voxelMeshRenderQueue)
        {
            Transform* transform = mesh->GetNode()->GetComponent<Transform>();
            if (transform)
            {
                mesh->Render(transform->GetGlobalMatrix(), glm::mat3_cast(transform->GetGlobalRotation()));
            }
            else
            {
                mesh->Render(); // Draw with no transform (default argument is identity matrix)
            }
        }
        VoxelMesh::FlushRenderQueue();
        voxelMeshRenderQueue.clear();

        // TODO: PBR material pass

        // Lighting passes

        // Bind the main render target to draw to
        gBuffer->Unbind(GL_DRAW_FRAMEBUFFER);

        // Clear the render target (not needed if using viewport / blit method)
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Use custom viewport instead of renderTarget
        if (renderMode == RenderMode::CustomViewport) glViewport(viewportX, viewportY, viewportWidth, viewportHeight);

        // Update global light buffer
        globalLightBuffer.BindBase(GL_UNIFORM_BUFFER, (GLuint)UniformBindingIndex::GlobalLights);

        // Write all active global lights to the buffer
        GLuint activeLights = 0;
        for (int i = 0; i < (int)LightSlot::NUM_SLOTS; i++)
        {
            DirectionalLight* light = globalLights[i];
            if (light)
            {
                globalLightBuffer.Write(light->color);
                globalLightBuffer.Write(glm::vec4(light->direction, light->ambient));
                activeLights++;
            }
        }
        
        // Bind shader and update light count uniform
        globalLightShader.Use();
        globalLightShader.SetUniform("globalLightCount", activeLights);

        // Bind the geometry buffer textures
        gTexPosition->Bind(0);
        gTexNormal->Bind(1);
        gTexMaterial->Bind(2);

        // Blit the geometry buffer's depth texture to the main render target
        switch (renderMode)
        {
            case RenderMode::MatchInternalResolution:
                glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                break;
            
            case RenderMode::CustomViewport:
                glBlitFramebuffer(0, 0, renderWidth, renderHeight, viewportX, viewportY, viewportX + viewportWidth, viewportY + viewportHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                break;
        }

        // Disable depth testing / writing
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_FALSE);

        // Draw fullscreen triangle to calculate global lighting on every pixel
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        // Lock global light buffer
        globalLightBuffer.Lock();
        globalLightBuffer.SwapSections();

        // TODO: Point light pass

        // Environment pass

        // Draw the skybox
        if (activeSkybox) activeSkybox->Render();

        // Particle pass

        // Render all particle effects
        // POTENTIAL: See if we can render particles to the geometry buffer instead
        // Probably not, but worth thinking about possibly interpolating particle data while blending
        // (stencil buffer could count number of times a pixel is blended and average all positions / normals at the end?)
        for (auto&&[node, effect] : registry.view<CPUParticleEffect>().each())
        {
            Transform* transform = effect.GetNode()->GetComponent<Transform>();
            if (transform)
            {
                effect.Render(transform->GetGlobalMatrix());
            }
            else
            {
                effect.Render(); // Draw with no transform (default argument is identity matrix)
            }
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
    }

    void Scene::ShowDebug()
    {
        ImGui::Begin("Scene");

        ImGui::Text("Statistics");
        ImGui::Separator();
        ImGui::Text("Nodes: %lu", nodeCount);
        ImGui::Text("Voxel meshes rendered: %lu", voxelMeshRenderQueue.size());
        ImGui::NewLine();

        ImGui::Text("Active Camera");
        ImGui::Separator();
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

        ImGui::Text("Settings");
        ImGui::Separator();
        
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
        const auto& it = materialIDs.find(name);

        if (it != materialIDs.end())
        {
            // Material with provided name exists, replace it
            materials[it->second] = material;
        }
        else
        {
            // Material does not exist, add it and add the ID to the map
            materialIDs[name] = materials.size();
            materials.push_back(material);

            // Write the new material to the buffer
            basicMaterialBuffer.Sync();
            basicMaterialBuffer.Write(glm::vec4{glm::vec3(material.color), material.shininess});
        }

        return materialIDs[name];
    }

    int Scene::GetMaterialID(const std::string& name)
    {
        // Find if the material exists
        const auto& it = materialIDs.find(name);

        if (it != materialIDs.end())
        {
            // Material with provided name exists
            return it->second;
        }

        // Does not exist, return default value
        return 0;
    }

    void Scene::LoadMaterials(const std::string& filepath)
    {
        try
        {
            // Load YAML file
            YAML::Node node = YAML::LoadFile(File::GlobalizePath(filepath));

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
                    std::string name = mat["name"] ? mat["name"].as<std::string>() : "Unnamed Material";
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
            Error("YAML Parser Error: ", filepath);
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

    void Scene::AttachLight(DirectionalLight& light, LightSlot slot)
    {
        RemoveLight(slot);
        globalLights[(int)slot] = &light;
    }

    void Scene::RemoveLight(LightSlot slot)
    {
        DirectionalLight* current = globalLights[(int)slot];
        if (current)
        {
            current->active = false;
            globalLights[(int)slot] = nullptr;
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

        // Perform any updates that need to happen when resolution changes
        if (activeCamera) activeCamera->UpdateViewport(width, height);
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
            delete gBuffer;
            delete gTexPosition;
            delete gTexNormal;
            delete gTexMaterial;
            delete gTexDepthStencil;
        }

        // Create textures
        rTexColor = new Texture2D(renderWidth, renderHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        rTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexPosition = new Texture2D(renderWidth, renderHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexNormal = new Texture2D(renderWidth, renderHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexMaterial = new Texture2D(renderWidth, renderHeight, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        gTexDepthStencil = new Texture2D(renderWidth, renderHeight, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

        // Create framebuffers and attach textures
        renderTarget = new Framebuffer();
        renderTarget->Bind();
        renderTarget->AttachTexture(rTexColor, GL_COLOR_ATTACHMENT0);
        renderTarget->AttachTexture(rTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);
        renderTarget->CheckCompleteness();

        gBuffer = new Framebuffer();
        gBuffer->Bind();
        gBuffer->AttachTexture(gTexPosition, GL_COLOR_ATTACHMENT0);
        gBuffer->AttachTexture(gTexNormal, GL_COLOR_ATTACHMENT1);
        gBuffer->AttachTexture(gTexMaterial, GL_COLOR_ATTACHMENT2);
        gBuffer->AttachTexture(gTexDepthStencil, GL_DEPTH_STENCIL_ATTACHMENT);

        // Set the draw buffers for the currently bound FBO
        GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, drawBuffers);

        // :)
        gBuffer->CheckCompleteness();
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
        cameraBuffer.Write(glm::vec4(activeCamera->GetPosition(), 1.0f));
        cameraBuffer.Write(glm::vec4(activeCamera->GetWidth() * 0.5f, activeCamera->GetHeight() * 0.5f, 0.0f, 0.0f));
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
            const glm::vec3& pos = sphere.GetNode()->GetComponent<Transform>()->GetGlobalPosition();
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