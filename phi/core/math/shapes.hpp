#pragma once

#include <glm/glm.hpp>

// windef.h defines empty macros called near and far so we can't use them as variable names
// Undefining should be safe, but if this causes any issues in the future, renaming may be required
#ifdef near
    #undef near
#endif
#ifdef far
    #undef far
#endif

namespace Phi
{
    // 2D Shapes

    // Represents a rectangle with floating point coordinates
    struct Rectangle
    {
        Rectangle(float left, float top, float right, float bottom);
        ~Rectangle();

        // Intersection tests
        bool Intersects(const Rectangle& rect) const;

        // Accessors

        // Returns the width of this rectangle
        inline float GetWidth() const { return right - left + 1; };

        // Returns the height of this rectangle
        inline float GetHeight() const { return top - bottom + 1; };

        // Data
        float left, top, right, bottom;
    };

    // Represents a rectangle with integer coordinates
    struct IRectangle
    {
        IRectangle(int left, int top, int right, int bottom);
        ~IRectangle();

        // Intersection tests
        bool Intersects(const IRectangle& rect) const;

        // Accessors

        // Returns the width of this rectangle
        inline int GetWidth() const { return right - left + 1; };

        // Returns the height of this rectangle
        inline int GetHeight() const { return top - bottom + 1; };

        // Data
        int left, top, right, bottom;
    };

    // 3D Shapes

    // Represents a 3D plane, the basis of many intersection tests
    struct Plane
    {
        Plane();
        Plane(float a, float b, float c, float d);
        Plane(const glm::vec3& normal, float distance);
        ~Plane();

        // Returns the signed minimum distance of the point to this plane
        inline float DistanceTo(const glm::vec3& point) const { return point.x * normal.x + point.y * normal.y + point.z * normal.z + distance; };
        
        // Normalizes the plane equation so that (a, b, c) is a unit vector and d is still proportional
        inline void Normalize()
        {
            float length = glm::length(normal);
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
            distance /= length;
        }

        // Data
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        float distance = 0;
    };

    // Represents a 3D frustum as 6 planes, supports point intersection tests
    struct Frustum
    {
        Frustum();
        Frustum(const Plane& near, const Plane& far,
                const Plane& top, const Plane& bottom,
                const Plane& left, const Plane& right);
        ~Frustum();

        // Intersection tests
        bool Intersects(const glm::vec3& point) const;

        // Data
        Plane near;
        Plane far;
        Plane left;
        Plane right;
        Plane top;
        Plane bottom;
    };

    // Represents an axis-aligned bounding box with floating point coordinates
    // Supports point and plane intersection tests
    struct AABB
    {
        // Creates an AABB with the given max and min coordinates
        AABB(const glm::vec3& min, const glm::vec3& max);

        // Creates an AABB from a rectangle by interpreting the
        // y axis of the rectangle as the z axis, and then
        // manually assigning a new max and min y coordinate
        AABB(Rectangle rect, float yMin = -10.0f, float yMax = 10.0f);

        ~AABB();

        // Intersection tests
        bool Intersects(const glm::vec3& point) const;
        bool Intersects(const Plane& plane) const;

        // NOTE: May give false positives!
        // Mostly used for culling since false positives can be corrected later
        bool IntersectsFast(const Frustum& frustum) const;

        // Accessors
        const glm::vec3& MinMax(bool minMax) const { return minMax ? max : min; };

        // Data
        glm::vec3 min;
        glm::vec3 max;
    };

    // Represents a sphere, supports point, plane, and frustum intersection tests
    struct Sphere
    {
        Sphere();
        Sphere(float x, float y, float z, float radius);
        Sphere(const glm::vec3& position, float radius);
        ~Sphere();

        // Intersection tests
        bool Intersects(const glm::vec3& point) const;
        bool Intersects(const Plane& plane) const;
        bool Intersects(const Frustum& frustum) const;

        // Data
        glm::vec3 position{0.0f};
        float radius = 1.0f;
    };
}