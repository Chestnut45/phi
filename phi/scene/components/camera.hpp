#pragma once

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include <phi/core/input.hpp>
#include <phi/core/math/shapes.hpp>
#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // Forward declare scene class
    class Scene;

    // Provides an interface to manipulate and update a camera used for rendering a scene
    class Camera : public BaseComponent
    {
        // Public interface
        public:

            // Different control modes for the camera
            enum class Mode : int
            {
                FirstPerson,
                Target,
                Cutscene
            };

            Camera();
            Camera(int width, int height);
            ~Camera();

            // Delete copy constructor/assignment
            Camera(const Camera&) = delete;
            Camera& operator=(const Camera&) = delete;

            // Delete move constructor/assignment
            Camera(Camera&& other) = delete;
            void operator=(Camera&& other) = delete;

            // Control mode manipulation
            inline void SetMode(Mode mode) { this->mode = mode; viewDirty = true; };

            // Movement
            void SetPosition(const glm::vec3& position);
            void Translate(const glm::vec3& offset);

            // View manipulation
            void LookAt(const glm::vec3& position);
            void Rotate(float yawOffset, float pitchOffset);
            void Zoom(float amount);

            // Processes input according to the current control mode
            void Update(float delta);
            
            // Updates the camera's resolution
            // NOTE: Automatically called by scene on active camera when Scene::SetViewport() is used
            void SetResolution(int width, int height);

            // Returns a normalized ray from the camera's position to the screen coordinate given
            Ray GenerateRay(double x, double y);

            // Vector accessors
            inline const glm::vec3& GetDirection() const { return forward; };
            inline const glm::vec3& GetPosition() const { return position; };
            inline const glm::vec3& GetRight() const { return right; };

            // Matrix accessors (recalculate if data has been changed)
            inline const glm::mat4& GetView() const { if (viewDirty) UpdateView(); return view; };
            inline const glm::mat4& GetProj() const { if (projDirty) UpdateProjection(); return proj; };

            // Other accessors
            inline int GetWidth() const { return width; };
            inline int GetHeight() const { return height; };
            inline Mode GetMode() const { return mode; };
            inline float GetFov() const { return fov; };
            Frustum GetViewFrustum() const;

        private:

            // Pointer stability guarantee for components
            static constexpr auto in_place_delete = true;

            // Default camera control mode
            Mode mode = Mode::FirstPerson;

            // Camera properties
            glm::vec3 position;

            // Directional info
            mutable glm::vec3 forward;
            mutable glm::vec3 up;
            mutable glm::vec3 right;

            // Matrices
            mutable glm::mat4 view;
            mutable glm::mat4 proj;

            // Flags for when properties need to be recalculated
            mutable bool viewDirty = false;
            mutable bool projDirty = false;

            // View properties (Sensible defaults)
            int width = 1280;
            int height = 720;
            float aspect = 1.0f;
            float near = 0.1f;
            float far = 1000.0f;
            float fov = 60.0f;

            // View angles
            float yaw = -90.0f;
            float pitch = 0.0f;

            // TODO: Move controls out of this class

            // Control variables
            float walkSpeed = 4.0f;
            float runMultiplier = 16.0f;
            float lookSensitivity = 0.045f;

            // Input handling
            Input input;
            
            // Internal helper functions
            void UpdateView() const;
            void UpdateProjection() const;
        
        // Friends
        private:

            // Scene requires access to set / get some data
            friend class Scene;

            // Our currently active scene, if any
            // NOTE: This exists solely to ensure deleting a camera component
            // will properly update the scene's active camera to nullptr
            Scene* activeScene = nullptr;
    };
}