#include "camera.hpp"

#include <utility>
#include <phi/scene/scene.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{

    Camera::Camera(int width, int height) : position(0), forward(0, 0, -1), up(0, 1, 0), right(1, 0, 0)
    {
        // Ensure our matrices are in a valid state
        SetResolution(width, height);
        UpdateView();

        // Create uniform buffer

        // Query UBO alignment
        GLint UBO_ALIGNMENT;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UBO_ALIGNMENT);

        // Align wanted buffer size to multiple of alignment
        int cameraBufferSize = sizeof(glm::mat4) * 6 + sizeof(glm::vec4) * 3;
        int alignedSize = ( UBO_ALIGNMENT == 0 ) ? cameraBufferSize : ceil( (float)cameraBufferSize / UBO_ALIGNMENT ) * UBO_ALIGNMENT;

        // Create the buffer
        ubo = new GPUBuffer(BufferType::DynamicDoubleBuffer, alignedSize);
    }

    Camera::~Camera()
    {
        // Ensure we are removed from any active scene on destruction
        Scene& scene = GetNode()->GetScene();
        if (scene.GetActiveCamera() == this)
        {
            scene.RemoveCamera();
        }

        // Delete uniform buffer
        delete ubo;
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
        // DEBUG: View manipulation
        if (input.IsMouseCaptured())
        {
            // Keyboard movement
            float movementSpeed = walkSpeed * delta * (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) ? runMultiplier : 1.0f);
            if (input.IsKeyDown(GLFW_KEY_W)) Translate(forward * movementSpeed);
            if (input.IsKeyDown(GLFW_KEY_S)) Translate(-forward * movementSpeed);
            if (input.IsKeyDown(GLFW_KEY_A)) Translate(-right * movementSpeed);
            if (input.IsKeyDown(GLFW_KEY_D)) Translate(right * movementSpeed);
            
            // Calculate mouse movement
            glm::vec2 mouseOffset = input.GetMouseDelta() * lookSensitivity;

            // Rotate the camera according to mouse movement
            Rotate(mouseOffset.x, -mouseOffset.y);

            // Zoom the camera according to mouse scroll
            Zoom(input.GetMouseScroll().y);
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

    Ray Camera::GenerateRay(double x, double y)
    {
        if (orthographic)
        {
            float xOfs = (2.0f * x / (float)width - 1.0f) * (0.5f * (float)orthoWidth);
            float yOfs = 1.0f - (2.0f * y / (float)height) * (0.5f * (float)orthoHeight);
            glm::vec3 localUp = glm::normalize(glm::cross(right, forward));
            return std::move(Ray(position + right * xOfs + localUp * yOfs, forward));
        }
        else
        {
            glm::vec4 ndc = glm::vec4((2.0f * x / width) - 1.0f, 1.0f - (2.0f * y / height), 0.0f, 1.0f);
            glm::vec4 eye = glm::inverse(proj) * ndc;
            glm::vec4 world = glm::inverse(view) * glm::vec4(eye.x, eye.y, eye.z, 0.0f);
            return std::move(Ray(position, glm::normalize(world)));
        }
    }

    void Camera::UpdateView() const
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

        // Update view
        view = glm::lookAt(position, position + forward, up);
        viewDirty = false;
    }

    void Camera::UpdateProjection() const
    {
        if (orthographic)
        {
            float hx = 0.5f * 256;
            float hy = 0.5f * 256;
            proj = glm::ortho(-hx, hx, -hy, hy, near, far);
        }
        else
        {
            proj = glm::perspective(glm::radians(fov), aspect, near, far);
        }
        
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

    void Camera::UpdateUBO() const
    {
        // Grab camera values
        const glm::mat4& view = GetView();
        const glm::mat4& proj = GetProj();
        glm::mat4 viewProj = proj * view;

        // Ensure we don't write when commands are reading
        ubo->Sync();

        // Write camera matrix data to UBO
        // Doing this once here on the CPU is a much easier price to pay than per-vertex
        ubo->Write(viewProj);
        ubo->Write(glm::inverse(viewProj));
        ubo->Write(view);
        ubo->Write(glm::inverse(view));
        ubo->Write(proj);
        ubo->Write(glm::inverse(proj));
        ubo->Write(glm::vec4(position, 1.0f));
        ubo->Write(glm::vec4(0, 0, width * 0.5f, height * 0.5f));
        ubo->Write(glm::vec4(near, far, 0.0f, 0.0f));
    }
}