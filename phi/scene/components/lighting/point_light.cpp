#include "point_light.hpp"

#include <phi/graphics/geometry.hpp>
#include <phi/scene/node.hpp>

namespace Phi
{
    PointLight::PointLight()
    {
        if (refCount == 0)
        {
            // Initialize static resources

            // Generate icosphere vertex data
            // TODO: Geometry should have generation methods (pull out of BasicMesh)
            for (const VertexPos& v : Icosphere::UNIT_ICOSPHERE_VERTICES)
            {
                sphereVerts.push_back(v);
            }
            for (const GLuint& i : Icosphere::UNIT_ICOSPHERE_INDICES)
            {
                sphereInds.push_back(i);
            }

            // Init shaders
            basicShader = new Shader();
            basicShader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/point_light.vs");
            basicShader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/point_light_basic.fs");
            basicShader->Link();

            voxelShader = new Shader();
            voxelShader->LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/point_light.vs");
            voxelShader->LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/point_light_voxel.fs");
            voxelShader->Link();

            // Initialize buffers
            vertexBuffer = new GPUBuffer(BufferType::Static, sizeof(VertexPos) * sphereVerts.size(), sphereVerts.data());
            indexBuffer = new GPUBuffer(BufferType::Static, sizeof(GLuint) * sphereInds.size(), sphereInds.data());
            instanceBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, (sizeof(glm::vec4) + sizeof(glm::vec3)) * MAX_POINT_LIGHTS);

            // Initialize vertex attributes
            vao = new VertexAttributes(VertexFormat::POS, vertexBuffer, indexBuffer);

            // Source instanced attributes from the correct buffer
            instanceBuffer->Bind(GL_ARRAY_BUFFER);
            vao->Bind();
            vao->AddAttribute(3, GL_FLOAT, 1, sizeof(glm::vec4) + sizeof(glm::vec3), 0);
            vao->AddAttribute(4, GL_FLOAT, 1, sizeof(glm::vec4) + sizeof(glm::vec3), sizeof(glm::vec3));
        }

        refCount++;
    }

    PointLight::~PointLight()
    {
        refCount--;

        if (refCount == 0)
        {
            delete basicShader;
            delete voxelShader;
            delete vertexBuffer;
            delete indexBuffer;
            delete instanceBuffer;
            delete vao;
        }
    }

    void PointLight::Render()
    {
        // Flush if full
        if (queuedLights == MAX_POINT_LIGHTS) PointLight::FlushRenderQueue();

        // Sync if first drawn this frame
        if (queuedLights == 0) instanceBuffer->Sync();

        // Grab transform
        Transform* transform = GetNode()->Get<Transform>();

        // Write position, color, and radius to instance data buffer
        instanceBuffer->Write(transform ? glm::vec3(transform->GetGlobalMatrix() * glm::vec4(position, 1.0f)) : position);
        instanceBuffer->Write(glm::vec4(color.r, color.g, color.b, radius));

        // Increase counter
        queuedLights++;
    }

    void PointLight::FlushRenderQueue(bool doBasicPass, bool doVoxelPass)
    {
        if (queuedLights == 0) return;

        vao->Bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glFrontFace(GL_CW);

        // Shade basic materials
        if (doBasicPass)
        {
            basicShader->Use();
            glStencilFunc(GL_EQUAL, (GLint)Scene::StencilValue::BasicMaterial, 0xff);
            glDrawElementsInstancedBaseInstance(GL_TRIANGLES, sphereInds.size(), GL_UNSIGNED_INT, 0, queuedLights, MAX_POINT_LIGHTS * instanceBuffer->GetCurrentSection());
        }

        // Shade voxel materials
        if (doVoxelPass)
        {
            voxelShader->Use();
            glStencilFunc(GL_EQUAL, (GLint)Scene::StencilValue::VoxelMaterial, 0xff);
            glDrawElementsInstancedBaseInstance(GL_TRIANGLES, sphereInds.size(), GL_UNSIGNED_INT, 0, queuedLights, MAX_POINT_LIGHTS * instanceBuffer->GetCurrentSection());
        }
        
        // Unbind resources and swap buffers
        glFrontFace(GL_CCW);
        glDisable(GL_BLEND);
        vao->Unbind();
        instanceBuffer->Lock();
        instanceBuffer->SwapSections();

        // Reset counters
        queuedLights = 0;
    }
}