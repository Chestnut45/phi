#include "sky.hpp"

#include <phi/core/math/constants.hpp>
#include <phi/core/file.hpp>
#include <phi/scene/scene.hpp>

namespace Phi
{
    Sky::Sky(const std::string& dayMapPath, const std::string& nightMapPath)
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
            // Load shader
            skyboxShader = new Phi::Shader();
            skyboxShader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/skybox.vs");
            skyboxShader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/skybox.fs");
            skyboxShader->Link();

            // Generate dummy VAO for attributeless rendering
            glGenVertexArrays(1, &dummyVAO);
        }

        refCount++;
    }

    Sky::~Sky()
    {
        refCount--;

        // Safely remove ourselves from any active scene
        if (activeScene) activeScene->RemoveSkybox();

        // If last instance, cleanup static resources
        if (refCount == 0)
        {
            // Delete the shader
            delete skyboxShader;

            // Delete dummy vao
            glDeleteVertexArrays(1, &dummyVAO);
        }
    }

    void Sky::RenderSkybox()
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
}