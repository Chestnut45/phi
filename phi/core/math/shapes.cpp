#include "shapes.hpp"

#include <cmath>

namespace Phi
{
    // Rectangle implementation

    Rectangle::Rectangle(float left, float top, float right, float bottom)
        : left(left), top(top), right(right), bottom(bottom)
    {
    }

    Rectangle::~Rectangle()
    {
    }

    bool Rectangle::Intersects(const Rectangle& rect) const
    {
        return (left < rect.right && right > rect.left && top > rect.bottom && bottom < rect.top);
    }

    // IRectangle implementation

    IRectangle::IRectangle(int left, int top, int right, int bottom)
        : left(left), top(top), right(right), bottom(bottom)
    {
    }

    IRectangle::~IRectangle()
    {
    }

    bool IRectangle::Intersects(const IRectangle& rect) const
    {
        return (left < rect.right && right > rect.left && top > rect.bottom && bottom < rect.top);
    }

    // Ray implementation

    Ray::Ray()
    {
    }

    Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
        : origin(origin), direction(direction)
    {
    }

    Ray::~Ray()
    {
    }

    glm::vec2 Ray::Slabs(const AABB& aabb)
    {
        glm::vec3 tMin = (aabb.min - origin) / direction;
        glm::vec3 tMax = (aabb.max - origin) / direction;
        glm::vec3 t1 = glm::min(tMin, tMax);
        glm::vec3 t2 = glm::max(tMin, tMax);
        float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
        float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
        return glm::vec2(tNear, tFar);
    }

    // Plane implementation

    Plane::Plane()
    {
    }
    
    Plane::Plane(float a, float b, float c, float d)
        : normal(a, b, c), distance(d)
    {
    }

    Plane::Plane(const glm::vec3& normal, float distance)
        : normal(normal), distance(distance)
    {
    }

    Plane::~Plane()
    {
    }

    // AABB implementation

    AABB::AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max)
    {
    }

    AABB::AABB(Rectangle rect, float yMin, float yMax)
        : min(rect.left, yMin, rect.bottom), max(rect.right, yMax, rect.top)
    {
    }

    AABB::~AABB()
    {
    }

    bool AABB::Intersects(const glm::vec3& point) const
    {
        return (
            point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z
        );
    }

    bool AABB::Intersects(const Plane& plane) const
    {
        // Convert to center / extents form
        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 extents = max - center;
        
        // Calculate projection interval radius
        float r = extents.x * abs(plane.normal.x) + extents.y * abs(plane.normal.y) + extents.z * abs(plane.normal.z);

        // Calculate distance of center from plane
        float dist = glm::dot(plane.normal, center) - plane.distance;

        // Intersection happens when distance falls within that radius
        return abs(dist) <= r;
    }

    bool AABB::IntersectsFast(const Frustum& frustum) const
    {
        const Plane planes[6] =
        {
            frustum.near, frustum.far, frustum.left, frustum.right, frustum.top, frustum.bottom
        };

        // Check all the planes of the frustum
        for (int i = 0; i < 6; i++)
        {
            const Plane& plane = planes[i];

            int nx = plane.normal.x > 0.0f;
            int ny = plane.normal.y > 0.0f;
            int nz = plane.normal.z > 0.0f;

            float dot = (plane.normal.x * MinMax(nx).x) + (plane.normal.y * MinMax(ny).y) + (plane.normal.z * MinMax(nz).z);

            if (dot < -plane.distance) return false;
        }

        return true;
    }

    // Frustum implementation

    Frustum::Frustum()
    {
    }

    Frustum::Frustum(const Plane& near, const Plane& far,
                     const Plane& top, const Plane& bottom,
                     const Plane& left, const Plane& right)
        : near(near), far(far), top(top), bottom(bottom), left(left), right(right)
    {
    }

    Frustum::~Frustum()
    {
    }

    bool Frustum::Intersects(const glm::vec3& point) const
    {
        // RATIONALE: Near plane is most likely to cause an early out,
        // far plane is least likely to have points tested since we
        // may not even load that far out
        if (near.DistanceTo(point) < 0) return false;
        if (top.DistanceTo(point) < 0) return false;
        if (bottom.DistanceTo(point) < 0) return false;
        if (left.DistanceTo(point) < 0) return false;
        if (right.DistanceTo(point) < 0) return false;
        if (far.DistanceTo(point) < 0) return false;
        return true;
    }

    // Sphere implementation

    Sphere::Sphere()
    {
    }

    Sphere::Sphere(float x, float y, float z, float radius)
        : position(x, y, z), radius(radius)
    {
    }

    Sphere::Sphere(const glm::vec3& position, float radius)
        : position(position), radius(radius)
    {
    }

    Sphere::~Sphere()
    {
    }

    bool Sphere::Intersects(const glm::vec3& point) const
    {
        return glm::distance(position, point) <= radius;
    }

    bool Sphere::Intersects(const Plane& plane) const
    {   
        return std::abs(plane.DistanceTo(position)) <= radius;
    }

    bool Sphere::Intersects(const Frustum& frustum) const
    {
        // RATIONALE: Near plane is most likely to cause an early out,
        // far plane is least likely to have points tested since we
        // may not even load that far out
        if (frustum.near.DistanceTo(position) < -radius) return false;
        if (frustum.left.DistanceTo(position) < -radius) return false;
        if (frustum.right.DistanceTo(position) < -radius) return false;
        if (frustum.top.DistanceTo(position) < -radius) return false;
        if (frustum.bottom.DistanceTo(position) < -radius) return false;
        if (frustum.far.DistanceTo(position) < -radius) return false;
        return true;
    }
}