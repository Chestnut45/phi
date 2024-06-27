#pragma once

// System
#include <string>
#include <unordered_map>
#include <vector>

// Third party
#include <entt.hpp>

// Core systems
#include <phi/core/structures/quadtree.hpp>

// Graphics
#include <phi/graphics/materials.hpp>
#include <phi/graphics/framebuffer.hpp>
#include <phi/graphics/texture_2d.hpp>

// Components
#include <phi/scene/components/camera.hpp>
#include <phi/scene/components/transform.hpp>
#include <phi/scene/components/collision/bounding_sphere.hpp>
#include <phi/scene/components/lighting/directional_light.hpp>
#include <phi/scene/components/renderable/basic_mesh.hpp>
#include <phi/scene/components/renderable/environment.hpp>
#include <phi/scene/components/renderable/voxel_mesh.hpp>
#include <phi/scene/components/simulation/voxel_map.hpp>
#include <phi/scene/components/simulation/voxel_material.hpp>

namespace Phi
{
    // Forward declare node class
    class Node;

    // Unique identifier for every node in a scene
    typedef entt::entity NodeID;

    // A collection of nodes that contain various components / resources
    // Used to perform simulations on components and handle rendering logic
    class Scene
    {
        // Interface
        public:

            // Valid render modes
            enum class RenderMode
            {
                // Renders the final image to the default framebuffer with viewport (0, 0, renderWidth, renderHeight)
                DefaultFBO,

                // Renders the final image to a texture with the dimensions (0, 0, renderWidth, renderHeight)
                // Texture is accessible via Scene::GetTexture()
                Texture
            };

            // Reserved internal uniform buffer binding indices
            enum class UniformBindingIndex
            {
                Camera = 0,
                GlobalLights = 1,
                SSAO = 2,
            };

            // Reserved internal shader storage buffer binding indices
            enum class ShaderStorageBindingIndex
            {
                InstanceData = 0,
                PBRMaterial = 1,
            };

            // Special stencil values (mostly for readability)
            enum class StencilValue
            {
                None = 0,
                PBRMaterial = 1,
            };

            // Creates an empty scene with the given resolution
            Scene(int width = 1280, int height = 720);

            ~Scene();

            // Delete copy constructor/assignment
            Scene(const Scene&) = delete;
            Scene& operator=(const Scene&) = delete;

            // Delete move constructor/assignment
            Scene(Scene&& other) = delete;
            Scene& operator=(Scene&& other) = delete;

            // Node management
            //
            // NOTE: For all CreateNode*() functions, the pointer
            // returned is valid until one of the following occurs:
            // 1. The node is deleted via Scene::Delete(NodeID)
            // 2. The scene is cleared (deleting all nodes)
            // 3. The scene is destroyed (destructor is called)

            // Creates and registers a new empty node into the scene
            Node* CreateNode();

            // Creates and registers a new node with a transform component into the scene
            Node* CreateNode3D();

            // TODO: Register & create template nodes / factories
            // e.g. RegisterNodeTemplate(string templateName, Node* node), CreateNode(string templateName)

            // Returns a pointer to the given node, or nullptr if id is invalid
            Node* Get(NodeID id);

            // Deletes the given node and all of their components / children from the scene
            void Delete(NodeID id);

            // Clears the entire scene, deleting all nodes / components
            void Clear();

            // Simulation / rendering

            // Updates all components in the scene according to simulation settings
            void Update(float delta);

            // Renders all renderable components in the scene according to
            // render settings and the currently active camera
            void Render();

            // Changes the rendering mode
            void SetRenderMode(RenderMode mode);

            // Updates the resolution the scene is rendered at
            // Updates the current active camera's viewport and internal texture resolutions
            // NOTE: Nonpositive (negative or zero) values are rejected (no behaviour)
            // TODO: Add auto setting that updates when the window is resized
            // RATIONALE: Tying the scene's resolution to a window should be opt-in, not opt-out
            void SetResolution(int width, int height);

            // Access to the scene's last rendered frame texture, or nullptr if none exists
            // NOTE: Only valid when using RenderMode::Texture
            Texture2D* GetTexture() { return rTexColor; }

            // Material management

            // Adds a single material to the internal registry
            // If a material already exists with the given name,
            // the new material data replaces the old, and keeps the same ID
            // Returns the ID of the material (IDs are per material type!)
            int RegisterMaterial(const std::string& name, const PBRMaterial& material);
            int RegisterMaterial(const std::string& name, const VoxelMaterial& material);

            // Returns a constant reference to the internal material data
            // for the given ID. If ID is invalid, the default material will be returned instead
            const PBRMaterial& GetPBRMaterial(int id) const;
            const VoxelMaterial& GetVoxelMaterial(int id) const;
            
            // Returns the ID for the given material name, if it exists
            // Returns 0 (the default material) otherwise
            int GetPBRMaterialID(const std::string& name) const;
            int GetVoxelMaterialID(const std::string& name) const;

            // Loads materials from a YAML file and adds them to the scene
            // NOTE: Currently works with PBRMaterial and VoxelMaterial
            void LoadMaterials(const std::string& path);

            // Const access to internal material lists
            inline const std::vector<PBRMaterial>& GetPBRMaterials() const { return pbrMaterials; }
            inline const std::vector<VoxelMaterial>& GetVoxelMaterials() const { return voxelMaterials; }

            // Camera management

            // Returns the currently active camera for the scene
            // Guaranteed to either be a valid camera or nullptr
            Camera* GetActiveCamera() { return activeCamera; };

            // Sets the active camera to the given camera
            // Removes current active camera first
            void SetActiveCamera(Camera& camera);

            // Removes the currently active camera
            // NOTE: Does not delete the camera component or node
            void RemoveCamera();

            // Environment management

            // Returns the currently active environment for the scene
            // Guaranteed to either be a valid environment or nullptr
            Environment* GetActiveEnvironment() { return activeEnvironment; }

            // Sets the active environment instance to use
            void SetActiveEnvironment(Environment& environment);

            // Removes the currently active environment
            // NOTE: Does not delete the component!
            void RemoveEnvironment();

            // Voxel map management

            // Returns the currently active voxel map in the scene
            // Guaranteed to either be a valid voxel map or nullptr
            VoxelMap* GetActiveVoxelMap() { return activeVoxelMap; }

            // Sets the voxel map component for the scene to update
            void SetActiveVoxelMap(VoxelMap& map);

            // Removes the currently active voxel map
            // NOTE: Does not delete the component!
            void RemoveVoxelMap();

            // Lighting

            // Sets the base ambient light in the scene
            void SetAmbientLight(const glm::vec3& ambient) { ambientLight = ambient; }

            // Gets the base ambient light in the scene
            const glm::vec3& GetAmbientLight() const { return ambientLight; }

            // Shows debug statistics in an ImGui window
            void ShowDebug(int x, int y, int width, int height);

            // TODO: Interface for iterating components / nodes directly

            // Limits
            static inline glm::ivec2 MAX_RESOLUTION = glm::ivec2(4096, 2160);
            static const int MAX_BASIC_MATERIALS = 1024;
            static const int MAX_VOXEL_MATERIALS = 1024;
            static const int MAX_USER_DIRECTIONAL_LIGHTS = 4;
            static const int SSAO_SAMPLE_SIZE = 32;

            // Minimum UBO alignment, calculated at scene construction
            GLint UBO_ALIGNMENT = 1;

        // Data / implementation
        private:

            // Internal registry for access to nodes and their components
            entt::basic_registry<NodeID> registry;

            // Active components
            Camera* activeCamera = nullptr;
            Environment* activeEnvironment = nullptr;
            VoxelMap* activeVoxelMap = nullptr;

            // Render data

            // Mode
            RenderMode renderMode{RenderMode::DefaultFBO};

            // Internal rendering resolution
            int renderWidth;
            int renderHeight;

            // Render queues and lists
            std::vector<BasicMesh*> basicMeshRenderQueue;
            std::vector<VoxelMesh*> voxelMeshRenderQueue;

            // Main render target
            Framebuffer* renderTarget = nullptr;
            Texture2D* rTexColor = nullptr;
            Texture2D* rTexDepthStencil = nullptr;

            // Geometry framebuffer and textures
            Framebuffer* gBuffer = nullptr;
            Texture2D* gTexNormal = nullptr;
            Texture2D* gTexAlbedo = nullptr;
            Texture2D* gTexEmissive = nullptr;
            Texture2D* gTexMetallicRoughness = nullptr;
            Texture2D* gTexDepthStencil = nullptr;

            // SSAO data
            GPUBuffer* ssaoKernelUBO = nullptr;
            Texture2D* ssaoRotationTexture = nullptr;
            Texture2D* ssaoScreenTexture = nullptr;
            Framebuffer* ssaoFBO = nullptr;
            Shader ssaoShader;

            // Light scattering / god rays (sun / sky)
            Framebuffer* sunlightFBO = nullptr;
            Texture2D* sunlightTexture = nullptr;
            Shader lightScatteringShader;
            Shader lightTransferShader;

            // A dummy VAO used for attributeless rendering
            GLuint dummyVAO = 0;

            // Material data

            // PBR Materials
            std::vector<PBRMaterial> pbrMaterials;
            std::unordered_map<std::string, int> pbrMaterialIDs;
            GPUBuffer pbrMaterialBuffer{BufferType::Dynamic, MAX_BASIC_MATERIALS * sizeof(glm::vec4) * 3};

            // Voxel materials
            std::vector<VoxelMaterial> voxelMaterials;
            std::unordered_map<std::string, int> voxelMaterialIDs;

            // TODO: Voxel Materials

            // Lighting data

            // The ambient light level in the scene
            glm::vec3 ambientLight = glm::vec3(0.0f);

            // Slots for global directional lights in the scene
            DirectionalLight* globalLights[(int)DirectionalLight::Slot::NUM_SLOTS];

            // Global lighting resources
            GPUBuffer* globalLightBuffer = nullptr;
            Shader globalLightPBRShader;
            Shader globalLightPBRSSAOShader;

            // Settings
            bool ssao = true;
            bool debugDrawing = true;
            bool depthPrePass = false;

            // Internal statistics
            float totalElapsedTime = 0.0f;
            size_t nodeCount = 0;

            // Helper functions
            void RegenerateFramebuffers();
        
        // Friends
        private:

            // Necessary for nodes to be able to add components to the scene
            friend class Node;

            // Necessary for directional lights to be able to activate / deactivate
            friend class DirectionalLight;
        
        // Experimental features
        private:

            // Settings
            bool cullingEnabled = false;
            bool cullWithQuadtree = false;
            bool dynamicQuadtree = false;
            
            // Builds up a quadtree containing every node that satisfies the following:
            // 1. Has a bounding volume component
            // 2. Has some renderable component (i.e. BasicMesh)
            // This quadtree (when built) is used for accelerated
            // frustum culling during the Update() method
            void BuildQuadtree();

            // Quadtree for all nodes with bounding volumes
            Quadtree<BoundingSphere*> quadtree{-150, 150, 150, -150};
    };
}