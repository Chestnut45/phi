#include "directional_light.hpp"

#include <phi/scene/node.hpp>

namespace Phi
{
    DirectionalLight::DirectionalLight()
    {
    }

    DirectionalLight::~DirectionalLight()
    {
        if (active) Deactivate();
    }

    void DirectionalLight::Activate(Slot slot)
    {
        // Deactivate first if necessary
        if (active) Deactivate();

        // Grab scene
        Scene* scene = GetNode()->GetScene();

        // Deactivate any existing light in the slot
        auto current = scene->globalLights[(int)slot];
        if (current) current->Deactivate();

        // Set ourselves to be active in the scene
        scene->globalLights[(int)slot] = this;
        this->active = true;
        this->slot = slot;
    }

    void DirectionalLight::Deactivate()
    {
        if (!active) return;

        // Get scene
        Scene* scene = GetNode()->GetScene();

        // Remove ourselves from the slot
        scene->globalLights[(int)slot] = nullptr;
        active = false;
    }
}