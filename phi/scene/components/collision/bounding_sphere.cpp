#include "bounding_sphere.hpp"

#include <phi/scene/node.hpp>
#include <phi/scene/components/transform.hpp>

namespace Phi
{
    BoundingSphere::BoundingSphere()
    {
    }
    
    BoundingSphere::BoundingSphere(float x, float y, float z, float radius)
        : volume(x, y, z, radius)
    {
    }

    BoundingSphere::BoundingSphere(const glm::vec3& position, float radius)
        : volume(position, radius)
    {
    }

    BoundingSphere::~BoundingSphere()
    {
    }

    void BoundingSphere::EncompassChildNodes()
    {
        // TODO
    }

    bool BoundingSphere::Intersects(const glm::vec3& point)
    {
        bool result;
        if (relativeToTransform)
        {
            Transform* transform = GetNode()->GetComponent<Transform>();
            if (transform)
            {
                glm::vec3 oldPosition = volume.position;

                // Calculate world space position
                volume.position = glm::vec3(transform->GetGlobalMatrix() * glm::vec4(volume.position, 1.0f));

                if (autoScale)
                {
                    float oldRadius = volume.radius;

                    // Calculate scale to guarantee coverage
                    glm::vec3 scale = transform->GetGlobalScale();
                    float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
                    volume.radius *= maxScale;

                    // Calculate intersection
                    bool result = volume.Intersects(point);

                    // Return to local space
                    volume.position = oldPosition;
                    volume.radius = oldRadius;
                    return result;
                }
                else
                {
                    // Calculate intersection
                    bool result = volume.Intersects(point);

                    // Return to local space
                    volume.position = oldPosition;
                    return result;
                }
            }
        }

        return volume.Intersects(point);
    }

    bool BoundingSphere::Intersects(const Plane& plane)
    {
        if (relativeToTransform)
        {
            Transform* transform = GetNode()->GetComponent<Transform>();
            if (transform)
            {
                glm::vec3 oldPosition = volume.position;

                // Calculate world space position
                volume.position = glm::vec3(transform->GetGlobalMatrix() * glm::vec4(volume.position, 1.0f));

                if (autoScale)
                {
                    float oldRadius = volume.radius;

                    // Calculate scale to guarantee coverage
                    glm::vec3 scale = transform->GetGlobalScale();
                    float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
                    volume.radius *= maxScale;

                    // Calculate intersection
                    bool result = volume.Intersects(plane);

                    // Return to local space
                    volume.position = oldPosition;
                    volume.radius = oldRadius;
                    return result;
                }
                else
                {
                    // Calculate intersection
                    bool result = volume.Intersects(plane);

                    // Return to local space
                    volume.position = oldPosition;
                    return result;
                }
            }
        }

        return volume.Intersects(plane);
    }

    bool BoundingSphere::Intersects(const Frustum& frustum)
    {
        if (relativeToTransform)
        {
            Transform* transform = GetNode()->GetComponent<Transform>();
            if (transform)
            {
                glm::vec3 oldPosition = volume.position;

                // Calculate world space position
                volume.position = glm::vec3(transform->GetGlobalMatrix() * glm::vec4(volume.position, 1.0f));

                if (autoScale)
                {
                    float oldRadius = volume.radius;

                    // Calculate scale to guarantee coverage
                    glm::vec3 scale = transform->GetGlobalScale();
                    float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
                    volume.radius *= maxScale;

                    // Calculate intersection
                    bool result = volume.Intersects(frustum);

                    // Return to local space
                    volume.position = oldPosition;
                    volume.radius = oldRadius;
                    return result;
                }
                else
                {
                    // Calculate intersection
                    bool result = volume.Intersects(frustum);

                    // Return to local space
                    volume.position = oldPosition;
                    return result;
                }
            }
        }

        return volume.Intersects(frustum);
    }
}