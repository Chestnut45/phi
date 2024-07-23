#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // Represents an arbitrary 3D transformation
    class Transform : public BaseComponent
    {
        // Interface
        public:

            Transform();
            ~Transform();

            // DEBUG: Testing BaseComponent interfaces
            void InspectorGUI() {
                if (ImGui::DragFloat3("Position", &position.x))
                {
                    matrixDirty = true;
                }
            };

            // Default copy constructor/assignment
            Transform(const Transform&) = default;
            Transform& operator=(const Transform&) = default;

            // Default move constructor/assignment
            Transform(Transform&& other) = default;
            Transform& operator=(Transform&& other) = default;

            // Position manipulation
            void SetPosition(const glm::vec3& newPosition);
            void SetPositionXYZ(float x, float y, float z) { SetPosition(glm::vec3(x, y, z)); };
            void Translate(const glm::vec3& offset);
            void TranslateXYZ(float x, float y, float z) { Translate(glm::vec3(x, y, z)); }

            // Rotations
            void SetRotation(const glm::quat& newRotation);
            void SetRotationXYZ(float x, float y, float z) { SetRotation(glm::quat(glm::vec3(x, y, z))); };
            void SetRotationXYZDeg(float x, float y, float z) { SetRotation(glm::quat(glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z)))); };
            void Rotate(const glm::quat& rotation);
            void RotateXYZ(float x, float y, float z) { Rotate(glm::quat(glm::vec3(x, y, z))); };
            void RotateXYZDeg(float x, float y, float z) { Rotate(glm::quat(glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z)))); };

            // Scaling
            void SetScale(const glm::vec3& newScale);
            void SetScaleXYZ(float x, float y, float z) { SetScale(glm::vec3(x, y, z)); };
            void Scale(const glm::vec3& scale);
            void ScaleXYZ(float x, float y, float z) { Scale(glm::vec3(x, y, z)); };

            // Local Accessors
            const glm::vec3& GetLocalPosition() const { return position; }
            const glm::quat& GetLocalRotation() const { return rotation; }
            const glm::vec3& GetLocalScale() const { return scale; }
            const glm::mat4& GetLocalMatrix() const
            {
                if (matrixDirty)
                {
                    // Apply translation, rotation, and scale
                    matrix = glm::mat4(1.0f);
                    matrix = glm::translate(matrix, position);
                    matrix *= glm::mat4_cast(rotation);
                    matrix = glm::scale(matrix, scale);
                    
                    // Reset the flag
                    matrixDirty = false;
                }

                return matrix;
            }

            // Global Accessors
            glm::vec3 GetGlobalPosition() const;
            glm::quat GetGlobalRotation() const;
            glm::vec3 GetGlobalScale() const;
            glm::mat4 GetGlobalMatrix() const;

        // Data / implementation
        private:

            // Internal storage of transformation components
            glm::vec3 position{0.0f};
            glm::quat rotation{glm::vec3(0.0f)};
            glm::vec3 scale{1.0f};

            // Combined transformation matrix
            mutable glm::mat4 matrix{1.0f};

            // Flags
            mutable bool matrixDirty = false;
    };
}