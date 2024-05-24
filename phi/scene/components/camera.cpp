#include "camera.hpp"

#include <utility>
#include <phi/scene/scene.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{

    Camera::Camera() : position(0, 4, 16), forward(0, 0, -1), up(0, 1, 0), right(1, 0, 0)
    {
        // Ensure our matrices are in a valid state
        UpdateView();
        SetResolution(width, height);
    }

    Camera::Camera(int width, int height) : position(0, 4, 16), forward(0, 0, -1), up(0, 1, 0), right(1, 0, 0)
    {
        // Ensure our matrices are in a valid state
        UpdateView();
        SetResolution(width, height);
    }

    Camera::~Camera()
    {
        // Ensure we are removed from any active scene on destruction
        if (activeScene) activeScene->RemoveCamera();
    }

    void Camera::SetPosition(const glm::vec3& position)
    {
        this->position = position;
        viewDirty = true;
    }

    void Camera::Translate(const glm::vec3& offset)
    {
        this->position += offset;
        viewDirty = true;
    }

    void Camera::LookAt(const glm::vec3& position)
    {
        forward = glm::normalize(position - this->position);
        right = glm::normalize(glm::cross(forward, up));
        viewDirty = true;
    }

    void Camera::Rotate(float yawOffset, float pitchOffset)
    {
        yaw += yawOffset;
        pitch += pitchOffset;
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        viewDirty = true;
    }

    void Camera::Zoom(float amount)
    {
        fov -= amount;
        fov = glm::clamp(fov, 1.0f, 120.0f);
        projDirty = true;
    }

    void Camera::Update(float delta)
    {
        float movementSpeed;
        glm::vec2 mouseOffset;
        switch (mode)
        {
            case Mode::FirstPerson:

                // DEBUG: Don't bother updating unless mouse is captured
                if (!input.IsMouseCaptured()) break;

                // Keyboard movement
                movementSpeed = walkSpeed * delta * (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) ? runMultiplier : 1.0f);
                if (input.IsKeyDown(GLFW_KEY_W)) Translate(forward * movementSpeed);
                if (input.IsKeyDown(GLFW_KEY_S)) Translate(-forward * movementSpeed);
                if (input.IsKeyDown(GLFW_KEY_A)) Translate(-right * movementSpeed);
                if (input.IsKeyDown(GLFW_KEY_D)) Translate(right * movementSpeed);
                
                // Calculate mouse movement
                mouseOffset = input.GetMouseDelta() * lookSensitivity;

                // Rotate the camera according to mouse movement
                Rotate(mouseOffset.x, -mouseOffset.y);

                // Zoom the camera according to mouse scroll
                Zoom(input.GetMouseScroll().y);

                break;
            
            case Mode::Target:

                // Process user input

                break;
            
            case Mode::Cutscene:

                // Advance cutscene / scripted events

                break;
        }

        // DEBUG: Sync with any existing transform
        Transform* transform = GetNode()->Get<Transform>();
        if (transform) transform->SetPosition(position);
    };

    void Camera::SetResolution(int width, int height)
    {
        this->width = width;
        this->height = height;
        aspect = (float)width / (float)height;
        projDirty = true;
    }

    Ray Camera::CastRay(double x, double y)
    {
        glm::vec3 ndc = glm::vec3((2.0f * x) / width - 1.0f, 1.0f - (2.0f * y) / height, 1.0f);
        glm::vec4 clip = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);
        glm::vec4 eye = glm::inverse(proj) * clip;
        eye = glm::vec4(eye.x, eye.y, -1.0f, 0.0f);
        glm::vec3 world = glm::normalize(glm::vec3(glm::inverse(view) * eye));
        return Ray(position, world);
    }

    void Camera::UpdateView() const
    {
        if (mode == Mode::FirstPerson)
        {
            // Calculate direction from yaw and pitch
            glm::vec3 dir;
            float cosPitch = cos(glm::radians(pitch));
            dir.x = cos(glm::radians(yaw)) * cosPitch;
            dir.y = sin(glm::radians(pitch));
            dir.z = sin(glm::radians(yaw)) * cosPitch;

            // Calculate normalized forward and right axes
            forward = glm::normalize(dir);
            right = glm::normalize(glm::cross(forward, up));
        }

        // Update view matrix
        view = glm::lookAt(position, position + forward, up);
        viewDirty = false;
    }

    void Camera::UpdateProjection() const
    {
        proj = glm::perspective(glm::radians(fov), aspect, near, far);
        projDirty = false;
    }

    Frustum Camera::GetViewFrustum() const
    {
        // Frustum to return
        Frustum viewFrustum;

        // Get the combined view projection matrix
        glm::mat4 m = GetProj() * GetView();
        
        // Extract the planes from the combined matrix (Gribb / Hartmann method)
        // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
        viewFrustum.near = Plane(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
        viewFrustum.far = Plane(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);
        viewFrustum.top = Plane(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
        viewFrustum.bottom = Plane(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
        viewFrustum.left = Plane(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
        viewFrustum.right = Plane(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);

        // Normalize the planes so that distance calculations are accurate
        viewFrustum.near.Normalize();
        viewFrustum.far.Normalize();
        viewFrustum.top.Normalize();
        viewFrustum.bottom.Normalize();
        viewFrustum.left.Normalize();
        viewFrustum.right.Normalize();

        return std::move(viewFrustum);
    }
}