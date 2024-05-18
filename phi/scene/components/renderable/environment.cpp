#include "environment.hpp"

#include <phi/core/math/constants.hpp>
#include <phi/core/file.hpp>
#include <phi/scene/scene.hpp>
#include <phi/core/resource_manager.hpp>

namespace Phi
{
    Environment::Environment(const std::string& dayMapPath, const std::string& nightMapPath)
        :
        dayMap({dayMapPath + "/right.png",
                dayMapPath + "/left.png",
                dayMapPath + "/top.png",
                dayMapPath + "/bottom.png",
                dayMapPath + "/front.png",
                dayMapPath + "/back.png"}),
        
        nightMap({nightMapPath + "/right.png",
                nightMapPath + "/left.png",
                nightMapPath + "/top.png",
                nightMapPath + "/bottom.png",
                nightMapPath + "/front.png",
                nightMapPath + "/back.png"})
    {
        // If first instance, initialize static resources
        if (refCount == 0)
        {
            // Load shaders
            skyboxShader = new Phi::Shader();
            skyboxShader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/skybox.vs");
            skyboxShader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/skybox.fs");
            skyboxShader->Link();

            sunShader = new Shader();
            sunShader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/sun.vs");
            sunShader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/sun.fs");
            sunShader->Link();

            // Generate dummy VAO for attributeless rendering
            glGenVertexArrays(1, &dummyVAO);

            // Load sun texture
            sunTexture = new Texture2D("data://textures/particles/shapes/circle_6.png", GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
        }

        refCount++;
    }

    Environment::~Environment()
    {
        refCount--;

        // Safely remove ourselves from any active scene
        if (activeScene) activeScene->RemoveEnvironment();

        // If last instance, cleanup static resources
        if (refCount == 0)
        {
            // Delete the shader and texture
            delete skyboxShader;
            delete sunTexture;

            // Delete dummy vao
            glDeleteVertexArrays(1, &dummyVAO);
        }
    }

    void Environment::Update(float delta)
    {
        if (advanceTime)
        {
            // Update time
            float weightedDelta = (timeOfDay < 0.5f) ? delta / dayLength : delta / nightLength;
            weightedDelta *= 0.5f;
            timeOfDay += weightedDelta;
            if (timeOfDay > 1.0f) timeOfDay -= 1.0f;
        }

        // Calculate sun position
        sunPos.x = 0.0f;
        sunPos.y = glm::sin(TAU * timeOfDay) * sunDistance;
        sunPos.z = -glm::cos(TAU * timeOfDay) * sunDistance;

        // Rotate
        sunPos = glm::quat(glm::vec3(0.0f, sunRotation, 0.0f)) * sunPos;
    }

    void Environment::RenderSkybox()
    {
        // Bind resources
        skyboxShader->Use();
        dayMap.Bind(0);
        nightMap.Bind(1);

        // Write blend factor to mix day and night skyboxes
        float blendFactor = (1 - sin(timeOfDay * TAU)) / 2.0f;
        skyboxShader->SetUniform("blendFactor", blendFactor);

        // Draw and unbind
        glDepthFunc(GL_LEQUAL);
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    void Environment::RenderSun()
    {
        // Bind, draw, and unbind
        sunTexture->Bind(5);
        sunShader->Use();
        sunShader->SetUniform("sunColor", sunColor);
        sunShader->SetUniform("sunPos", sunPos);
        sunShader->SetUniform("sunSize", sunSize);
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
}