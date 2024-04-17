#pragma once

#include <glm/glm.hpp>

#include <phi/scene/components/base_component.hpp>

namespace Phi
{
    // Represents a single directional light
    class DirectionalLight : public BaseComponent
    {
        // Interface
        public:

            DirectionalLight();
            ~DirectionalLight();

            // Delete copy constructor/assignment
            DirectionalLight(const DirectionalLight&) = delete;
            DirectionalLight& operator=(const DirectionalLight&) = delete;

            // Delete move constructor/assignment
            DirectionalLight(DirectionalLight&& other) = delete;
            DirectionalLight& operator=(DirectionalLight&& other) = delete;

            // Mutators

            // Sets the color directly in floating point format [0.0, 1.0] rgb
            inline void SetColor(const glm::vec3& color) { this->color = color; };

            // Sets the direction vector for this light (must be normalized!)
            inline void SetDirection(const glm::vec3& direction) { this->direction = direction; };

            // Sets the amount of ambient light
            inline void SetAmbient(float ambient) { this->ambient = ambient; };

            // Accessors
            inline const glm::vec3& GetColor() const { return color; };
            inline const glm::vec3& GetDirection() const { return direction; };
            inline float GetAmbient() const { return ambient; };
        
        // Data / implementation
        private:
            
            // Pointer stability guarantee for components
            static constexpr auto in_place_delete = true;
            
            // Needed so scene can store attachment info for safe deletion
            friend class Scene;

            // Light data
            glm::vec3 color{1.0f};
            glm::vec3 direction{0.0f, -1.0f, 0.0f};
            float ambient = 0.1f;

            // Flags
            bool active = false;
            int slot = 0;
    };
}