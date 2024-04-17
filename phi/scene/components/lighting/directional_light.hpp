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

            // Valid slots for active global directional lights in a scene
            enum class Slot : int
            {
                SLOT_0,
                SLOT_1,
                SLOT_2,
                SLOT_3,
                
                // Enum to count slots, not a valid slot itself
                NUM_SLOTS
            };

            DirectionalLight();
            ~DirectionalLight();

            // Delete copy constructor/assignment
            DirectionalLight(const DirectionalLight&) = delete;
            DirectionalLight& operator=(const DirectionalLight&) = delete;

            // Delete move constructor/assignment
            DirectionalLight(DirectionalLight&& other) = delete;
            DirectionalLight& operator=(DirectionalLight&& other) = delete;

            // Scene management

            // Activates this light in the given slot of the current scene
            // Will deactivate any existing light in that slot
            void Activate(Slot slot);
            
            // Deactivates this light (will not render)
            void Deactivate();

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

            // Light data
            glm::vec3 color{1.0f};
            glm::vec3 direction{0.0f, -1.0f, 0.0f};
            float ambient = 0.1f;

            // State
            bool active = false;
            Slot slot = Slot::SLOT_0;

            // Necessary for scenes to introspect our data to write to buffers
            friend class Scene;
    };
}