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
    {
        // Generate globalized paths
        std::string dayPath = File::GlobalizePath(dayMapPath);
        std::string nightPath = File::GlobalizePath(nightMapPath);

        // Initialize day and night cubemaps
        dayMap = new Cubemap({  dayPath + "/right.png",
                                dayPath + "/left.png",
                                dayPath + "/top.png",
                                dayPath + "/bottom.png",
                                dayPath + "/front.png",
                                dayPath + "/back.png"});
        
        nightMap = new Cubemap({nightPath + "/right.png",
                                nightPath + "/left.png",
                                nightPath + "/top.png",
                                nightPath + "/bottom.png",
                                nightPath + "/front.png",
                                nightPath + "/back.png"});

        // If first instance, initialize static resources
        if (refCount == 0)
        {
            // Create VAO / VBO / Shader for skyboxes
            skyboxVBO = new Phi::GPUBuffer(Phi::BufferType::Static, sizeof(SKYBOX_VERTICES), SKYBOX_VERTICES);
            skyboxVAO = new Phi::VertexAttributes(Phi::VertexFormat::POS, skyboxVBO);
            skyboxShader = new Phi::Shader();

            // TODO: Use local paths hardcoded so that global paths can be generated at runtime
            skyboxShader->LoadShaderSource(GL_VERTEX_SHADER, "phi/graphics/shaders/skybox.vs");
            skyboxShader->LoadShaderSource(GL_FRAGMENT_SHADER, "phi/graphics/shaders/skybox.fs");
            skyboxShader->Link();
        }

        refCount++;
    }

    Skybox::~Skybox()
    {
        refCount--;

        // Delete cubemaps
        delete dayMap;
        delete nightMap;

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
        dayMap->Bind(0);
        nightMap->Bind(1);

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