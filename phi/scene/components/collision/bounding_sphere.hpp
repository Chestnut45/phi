#pragma once

#include <glm/glm.hpp>

#include <phi/core/math/shapes.hpp>
#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // Represents a sphere bounding volume that can be used for collision or culling
    class BoundingSphere : public BaseComponent
    {
        // Interface
        public:

            BoundingSphere();
            BoundingSphere(float x, float y, float z, float radius);
            BoundingSphere(const glm::vec3& position, float radius);
            ~BoundingSphere();

            // DEBUG: Testing BaseComponent interfaces
            void InspectorGUI() {};

            // Default copy constructor/assignment
            BoundingSphere(const BoundingSphere&) = default;
            BoundingSphere& operator=(const BoundingSphere&) = default;

            // Default move constructor/assignment
            BoundingSphere(BoundingSphere&& other) = default;
            BoundingSphere& operator=(BoundingSphere&& other) = default;

            // Automatic generation
            
            // Adjusts the position and radius of the sphere to
            // just encompass all child nodes' bounding volumes
            void EncompassChildNodes();

            // Manual generation
            void SetPosition(const glm::vec3& position) { this->volume.position = position; };
            void SetRadius(float radius) { this->volume.radius = radius; };

            // Intersection tests
            // NOTE: Behaviour is dependant on relativeToTransform
            bool Intersects(const glm::vec3& point);
            bool Intersects(const Plane& plane);
            bool Intersects(const Frustum& frustum);

            // Settings
            void SetCullingEnabled(bool value) { useForCulling = value; };
            void SetRelativeToTransform(bool value) { relativeToTransform = value; };
            void SetAutoScale(bool value) { autoScale = value; };

            // Accessors
            const Sphere& GetVolume() const { return volume; };
            bool IsCullingEnabled() const { return useForCulling; };
            bool IsRelativeToTransform() const { return relativeToTransform; };
            bool IsAutoScaleEnabled() const { return autoScale; };

        // Data / implementation
        private:

            // Volume used for intersection tests
            Sphere volume{0.0f, 0.0f, 0.0f, 1.0f};

            // Flags

            // If enabled, any meshes attached to this node will only be
            // rendered if we intersect the active camera's view frustum
            bool useForCulling = false;

            // If enabled, intersection tests will be performed in world
            // space, using an existing sibling Transform component as
            // the transformation matrix to apply translation and rotation from
            bool relativeToTransform = true;

            // If enabled, in addition to the center of the sphere being
            // adjusted during intersection tests, the radius will also be
            // scaled by the largest component of the Transform's scale vector
            //
            // Requires relativeToTransform to be true in order to have any effect
            // 
            // NOTE: Only enable if you require the bounding sphere to
            // scale dynamically with a node's transform, since this
            // setting has a non-insignificant performance impact
            bool autoScale = false;
    };
}