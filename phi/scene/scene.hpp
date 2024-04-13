#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <entt.hpp>

#include <phi/core/structures/quadtree.hpp>
#include <phi/graphics/materials.hpp>
#include <phi/graphics/framebuffer.hpp>
#include <phi/graphics/texture_2d.hpp>
#include <phi/scene/components/renderable/basic_mesh.hpp>
#include <phi/scene/components/collision/bounding_sphere.hpp>
#include <phi/scene/components/camera.hpp>
#include <phi/scene/components/renderable/skybox.hpp>
#include <phi/scene/components/transform.hpp>
#include <phi/scene/components/lighting/directional_light.hpp>
#include <phi/scene/components/renderable/voxel_mesh.hpp>

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

            // Valid slots for active global directional lights in a scene
            enum class LightSlot : int
            {
                SLOT_0 = 0,
                SLOT_1 = 1,
                SLOT_2 = 2,
                SLOT_3 = 3,
                
                NUM_SLOTS = 4
            };

            // Valid render modes
            enum class RenderMode
            {
                // Renders the final image to the viewport (0, 0, renderWidth, renderHeight) of the default framebuffer
                MatchInternalResolution,

                // Renders the final image to the custom viewport set by Scene::SetViewport(...)
                CustomViewport,
            };

            // Reserved internal uniform buffer binding indices
            enum class UniformBindingIndex
            {
                Camera = 0,
                GlobalLights = 1,
            };

            // Reserved internal shader storage buffer binding indices
            enum class ShaderStorageBindingIndex
            {
                InstanceData = 0,
                BasicMaterial = 1,
                VoxelMaterial = 2,
            };

            // Creates an empty scene with the default resolution
            Scene();

            // Creates an empty scene with the given resolution
            Scene(int width, int height);

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

            // Returns a pointer to the given node, or nullptr if id is invalid
            Node* Get(NodeID id);

            // Deletes the given node and all of their components / children from the scene
            void Delete(NodeID id);

            // Clears the entire scene, deleting all nodes / components
            void Clear();

            // Simulation / rendering

            // Updates all components in the scene according to simulation settings
            void Update(float delta);

            // Renders all renderable components in the scene according to render settings
            void Render();

            // Shows debug statistics in an ImGui window
            void ShowDebug();

            // Material management

            // Adds a single basic material to the internal registry
            // If a material already exists with the given name,
            // the new material data replaces the old, but keeps the same ID
            // Returns the ID of the material
            int AddMaterial(const std::string& name, const BasicMaterial& material);
            
            // Returns the ID for the given named material, if it exists
            // Returns 0 (the default material) otherwise
            int GetMaterialID(const std::string& name);

            // Loads the materials from a .pmat format data file
            void LoadMaterials(const std::string& filepath);

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

            // Sets the active skybox to the given skybox
            void SetActiveSkybox(Skybox& skybox);

            // Removes the currently active skybox
            void RemoveSkybox();

            // Global lighting management

            // Attaches a light to one of the global slots
            void AttachLight(DirectionalLight& light, LightSlot slot);

            // Removes any existing light at slot and replaces with nullptr
            void RemoveLight(LightSlot slot);

            // Resolution management

            // Updates the resolution the scene is rendered at
            // Affects the camera's viewport and internal framebuffer texture resolutions
            // NOTE: Nonpositive (negative or zero) values are rejected (no behaviour)
            // TODO: Add auto setting that updates when the window is resized
            // RATIONALE: Tying the scene's resolution to a window should be opt-in, not opt-out
            void SetResolution(int width, int height);

            // Updates the viewport to render to in window-relative coordinates (Bottom-left is (0, 0))
            // NOTE: Only affects output when using RenderMode::CustomViewport
            void SetViewport(int x, int y, int width, int height);

            // Changes the rendering mode
            void SetRenderMode(RenderMode mode);

            // TODO: Interface for iterating components / nodes directly

        // Data / implementation
        private:

            // Internal registry for access to nodes and their components
            entt::basic_registry<NodeID> registry;

            // Currently active camera
            Camera* activeCamera = nullptr;

            // Currently active skybox
            Skybox* activeSkybox = nullptr;

            // Render data

            // Mode
            RenderMode renderMode{RenderMode::MatchInternalResolution};

            // Limits
            static inline glm::ivec2 MAX_RESOLUTION = glm::ivec2(4096, 2160);

            // Internal rendering resolution
            int renderWidth = 1280;
            int renderHeight = 720;

            // Custom viewport data
            int viewportX = 0;
            int viewportY = 0;
            int viewportWidth = renderWidth;
            int viewportHeight = renderHeight;

            // Camera UBO
            GPUBuffer cameraBuffer{BufferType::DynamicDoubleBuffer, sizeof(glm::mat4) * 5 + sizeof(glm::vec4) * 2};

            // Main render target
            Framebuffer* renderTarget = nullptr;
            Texture2D* rTexColor = nullptr;
            Texture2D* rTexDepthStencil = nullptr;

            // Geometry framebuffer and textures
            Framebuffer* gBuffer = nullptr;
            Texture2D* gTexPosition = nullptr;
            Texture2D* gTexNormal = nullptr;
            Texture2D* gTexMaterial = nullptr;
            Texture2D* gTexDepthStencil = nullptr;

            // Material data
            std::vector<BasicMaterial> materials;
            std::unordered_map<std::string, int> materialIDs;

            // Material buffers
            GPUBuffer basicMaterialBuffer{BufferType::Dynamic, 1024 * sizeof(glm::vec4)};

            // Render queues and lists

            // Queue of basic meshes to be rendered this frame
            std::vector<BasicMesh*> basicMeshRenderQueue;

            // Queue of voxel meshes to be rendered this frame
            std::vector<VoxelMesh*> voxelMeshRenderQueue;

            // Wireframe resources
            std::vector<VertexPos> wireframeVerts;
            GPUBuffer wireframeBuffer{BufferType::Static};
            VertexAttributes wireframeVAO{VertexFormat::POS, &wireframeBuffer};
            Shader wireframeShader;

            // Lighting data

            // Slots for global directional lights in the scene
            DirectionalLight* globalLights[(int)LightSlot::NUM_SLOTS];

            // Global lighting resources
            GPUBuffer globalLightBuffer{BufferType::Dynamic, 4 * (sizeof(glm::vec4) * 2)};
            Shader globalLightShader;

            // A dummy VAO used for attributeless rendering
            GLuint dummyVAO = 0;

            // Internal statistics
            float totalElapsedTime = 0.0f;
            size_t nodeCount = 0;

            // Settings
            bool cullingEnabled = false;
            bool cullWithQuadtree = false;
            bool showQuadtree = false;
            bool dynamicQuadtree = false;

            // Helper functions
            void UpdateCameraBuffer();
            void RegenerateFramebuffers();
        
        // Friends
        private:

            // Necessary for cameras to be able to remove themselves from a scene on destruction
            friend class Camera;

            // Necessary for nodes to be able to add components to the scene
            friend class Node;
        
        // Experimental features
        private:

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