#include "skybox.hpp"

#include <phi/core/file.hpp>
#include <phi/scene/scene.hpp>

namespace Phi
{
    static const Phi::VertexPos SKYBOX_VERTICES[] =
    {
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        {-1.0f,  1.0f, -1.0f},
        {1.0f,  1.0f, -1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {1.0f, -1.0f,  1.0f}
    };

    Skybox::Skybox(const std::string& dayMapPath, const std::string& nightMapPath)
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
            // Create VAO / VBO / Shader for skyboxes
            skyboxVBO = new Phi::GPUBuffer(Phi::BufferType::Static, sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES);
            skyboxVAO = new Phi::VertexAttributes(Phi::VertexFormat::POS, skyboxVBO);
            skyboxShader = new Phi::Shader();

            // TODO: Use local paths hardcoded so that global paths can be generated at runtime
            skyboxShader->LoadSource(GL_VERTEX_SHADER, "phi/graphics/shaders/skybox.vs");
            skyboxShader->LoadSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/skybox.fs");
            skyboxShader->Link();
        }

        refCount++;
    }

    Skybox::~Skybox()
    {
        refCount--;

        // Safely remove ourselves from any active scene
        if (activeScene) activeScene->RemoveSkybox();

        // If last instance, cleanup static resources
        if (refCount == 0)
        {
            delete skyboxVBO;
            delete skyboxVAO;
            delete skyboxShader;
        }
    }

    void Skybox::Render()
    {
        // Bind resources
        skyboxShader->Use();
        skyboxVAO->Bind();
        dayMap.Bind(0);
        nightMap.Bind(1);

        // Write normalized time uniform to interpolate skyboxes
        skyboxShader->SetUniform("timeOfDay", timeOfDay);

        // Change depth function so max distance still renders
        glDepthFunc(GL_LEQUAL);
        
        // Draw, unbind, and reset depth function
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }
}