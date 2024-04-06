#include "directional_light.hpp"

#include <phi/scene/node.hpp>

namespace Phi
{
    DirectionalLight::DirectionalLight()
    {
    }

    DirectionalLight::~DirectionalLight()
    {
        if (active)
        {
            Scene* scene = GetNode()->GetScene();
            if (scene)
            {
                // Detach ourselves from the scene
                scene->RemoveLight((Phi::Scene::LightSlot)slot);
            }
        }
    }
}