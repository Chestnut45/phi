#pragma once

#include <iostream>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions

#include <phi/graphics/gpu_buffer.hpp>
#include <phi/graphics/vertex.hpp>

namespace Phi
{
    // RAII VAO wrapper with automagical constructor for internal vertex types
    class VertexAttributes
    {
        // Interface
        public:

            VertexAttributes();
            VertexAttributes(VertexFormat format, const GPUBuffer* const vbo, const GPUBuffer* const ebo = nullptr);
            ~VertexAttributes();

            // Delete copy constructor/assignment
            VertexAttributes(const VertexAttributes&) = delete;
            VertexAttributes& operator=(const VertexAttributes&) = delete;

            // Delete move constructor/assignment
            VertexAttributes(VertexAttributes&& other) = delete;
            void operator=(VertexAttributes&& other) = delete;

            // Sets the stride for all subsequent calls to Add()
            inline void SetStride(GLuint stride) { this->stride = stride; };

            // Adds an attribute and associates the currently bound buffer with that attribute
            // NOTE: This object must be bound before any calls to Add(), else they are invalid (undefined behaviour)
            void AddAttribute(GLuint numComponents, GLenum type, GLuint divisor = 0, GLuint stride = 0, GLsizeiptr offset = -1);

            // Binding methods
            void Bind() const;
            void Unbind() const { glBindVertexArray(0); };

        // Data / implementation
        private:
        
            // OpenGL objects
            GLuint vao;

            // Counters
            GLuint attribCount = 0;
            GLsizeiptr currentOffset = 0;
            GLuint stride = 0;
    };
}