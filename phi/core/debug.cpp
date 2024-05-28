#include "debug.hpp"
#include "logging.hpp"

#define GLEW_NO_GLU
#include <GL/glew.h>

namespace Phi
{
    void GLErrorCheck(const std::string& label)
    {
        GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
            std::string error;
            switch (errorCode)
            {
                case GL_INVALID_ENUM: error = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE: error = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: error = "GL_INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW: error = "GL_STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW: error = "GL_STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY: error = "GL_OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            }
            Error("OpenGL: ", error, " @ ", label);
        }
    }

    Debug::Debug()
    {
        // Initialization

        // Load shader
        shader.LoadSource(GL_VERTEX_SHADER, "phi://graphics/shaders/wireframe.vs");
        shader.LoadSource(GL_FRAGMENT_SHADER, "phi://graphics/shaders/wireframe.fs");
        shader.Link();

        vertexBuffer = new GPUBuffer(BufferType::DynamicDoubleBuffer, sizeof(glm::vec3) * 2 * MAX_VERTICES);
        vertexBuffer->Bind(GL_ARRAY_BUFFER);

        vao = new VertexAttributes();
        vao->Bind();
        vao->AddAttribute(3, GL_FLOAT, 0, sizeof(glm::vec3) * 2);
        vao->AddAttribute(3, GL_FLOAT, 0, sizeof(glm::vec3) * 2);
    }

    Debug::~Debug()
    {
        // Cleanup
        delete vertexBuffer;
        delete vao;
    }

    void Debug::DrawAABB(const AABB& aabb, const glm::vec3& color)
    {
        // Don't draw if buffer full
        if (queuedVertices + AABB_VERTEX_COUNT >= MAX_VERTICES) FlushShapes();

        // Sync
        if (queuedVertices == 0) vertexBuffer->Sync();

        // Write vertex data
        glm::vec3 verts[AABB_VERTEX_COUNT];

        // Bottom square
        verts[0] = glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z);
        verts[1] = glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z);
        verts[2] = glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z);
        verts[3] = glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z);
        verts[4] = glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z);
        verts[5] = glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z);
        verts[6] = glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z);
        verts[7] = glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z);
        
        // Top square
        verts[8] = glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z);
        verts[9] = glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z);
        verts[10] = glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z);
        verts[11] = glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z);
        verts[12] = glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z);
        verts[13] = glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z);
        verts[14] = glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z);
        verts[15] = glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z);

        // Pillars
        verts[16] = glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z);
        verts[17] = glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z);
        verts[18] = glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z);
        verts[19] = glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z);
        verts[20] = glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z);
        verts[21] = glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z);
        verts[22] = glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z);
        verts[23] = glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z);

        for (int i = 0; i < AABB_VERTEX_COUNT; ++i)
        {
            vertexBuffer->Write(verts[i]);
            vertexBuffer->Write(color);
        }

        queuedVertices += AABB_VERTEX_COUNT;
    }

    void Debug::DrawRay(const Ray& ray, float length, const glm::vec3& color)
    {
        // Don't draw if buffer full
        if (queuedVertices + RAY_VERTEX_COUNT >= MAX_VERTICES) FlushShapes();

        // Sync
        if (queuedVertices == 0) vertexBuffer->Sync();

        // Write vertex data
        vertexBuffer->Write(ray.origin);
        vertexBuffer->Write(color);
        vertexBuffer->Write(ray.direction * length);
        vertexBuffer->Write(color);

        queuedVertices += RAY_VERTEX_COUNT;
    }

    void Debug::FlushShapes()
    {
        if (queuedVertices == 0) return;

        vao->Bind();
        shader.Use();
        glDrawArrays(GL_LINES, 0, queuedVertices);

        vertexBuffer->Lock();
        vertexBuffer->SwapSections();
        
        queuedVertices = 0;
    }
}