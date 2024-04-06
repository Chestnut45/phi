#include "transform.hpp"

#include <phi/scene/node.hpp>

namespace Phi
{
    Transform::Transform()
    {

    }

    Transform::~Transform()
    {
        
    }

    void Transform::SetPosition(const glm::vec3& newPosition)
    {
        position = newPosition;
        matrixDirty = true;
    }

    void Transform::Translate(const glm::vec3& offset)
    {
        position += offset;
        matrixDirty = true;
    }

    void Transform::SetRotation(const glm::quat& newRotation)
    {
        rotation = newRotation;
        matrixDirty = true;
    }

    void Transform::Rotate(const glm::quat& rotation)
    {
        this->rotation = rotation * this->rotation; // order matters here
        matrixDirty = true;
    }

    void Transform::SetScale(const glm::vec3& newScale)
    {
        scale = newScale;
        matrixDirty = true;
    }

    void Transform::Scale(const glm::vec3& scale)
    {
        this->scale *= scale;
        matrixDirty = true;
    }

    glm::vec3 Transform::GetGlobalPosition() const
    {
        Node* parent = GetNode()->GetParent();
        if (parent)
        {
            Transform* parentTransform = parent->GetComponent<Transform>();
            if (parentTransform)
            {
                return parentTransform->GetGlobalMatrix() * glm::vec4(position, 1.0f);
            }
        }
        return position;
    }

    glm::quat Transform::GetGlobalRotation() const
    {
        Node* parent = GetNode()->GetParent();
        if (parent)
        {
            Transform* parentTransform = parent->GetComponent<Transform>();
            if (parentTransform)
            {
                return parentTransform->GetGlobalRotation() * rotation;
            }
        }
        return rotation;
    }

    glm::vec3 Transform::GetGlobalScale() const
    {
        Node* parent = GetNode()->GetParent();
        if (parent)
        {
            Transform* parentTransform = parent->GetComponent<Transform>();
            if (parentTransform)
            {
                return parentTransform->GetGlobalScale() * scale;
            }
        }
        return scale;
    }

    glm::mat4 Transform::GetGlobalMatrix() const
    {
        Node* parent = GetNode()->GetParent();
        if (parent)
        {
            Transform* parentTransform = parent->GetComponent<Transform>();
            if (parentTransform)
            {
                return parentTransform->GetGlobalMatrix() * GetLocalMatrix();
            }
        }
        return GetLocalMatrix();
    }
}