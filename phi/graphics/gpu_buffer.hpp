#pragma once

#include <iostream>
#include <string.h> // For memcpy

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions

#include <phi/core/logging.hpp>
#include <phi/graphics/indirect.hpp>

namespace Phi
{
    // Different types of buffers for different usage patterns
    // Static is best for data you only update once at construction or fully overwrite occasionally
    // Dynamic is best for data you must update every frame but don't have the space for double/triple buffering
    // Double/triple buffers make use of SwapSection() to write to different sections of the buffer
    // NOTE: With double/triple buffering, Lock() only locks the current section of the buffer
    enum class BufferType
    {
        Static,
        Dynamic,
        DynamicDoubleBuffer,
        DynamicTripleBuffer
    };
    
    // Buffer object RAII wrapper
    class GPUBuffer
    {
        // Interface
        public:

            // Constructor: size is the usable size in bytes of a single section
            GPUBuffer(BufferType type, size_t size = 0, const void* const data = NULL);
            ~GPUBuffer();

            // Delete copy constructor/assignment
            GPUBuffer(const GPUBuffer&) = delete;
            GPUBuffer& operator=(const GPUBuffer&) = delete;

            // Delete move constructor/assignment
            GPUBuffer(GPUBuffer&& other) = delete;
            void operator=(GPUBuffer&& other) = delete;

            // Sets the internal buffer's current offset
            inline void SetOffset(GLuint offset) { pCurrent = offset < size ? (pData + currentSection * size + offset) : pCurrent; };

            // Write operations for dynamic buffers only
            // NOTE: All writes are performed at the current pointer offset (in bytes)
            // If the write succeeds, the offset will be increased by the size of the data written
            bool Write(int value);
            bool Write(float value);
            bool Write(const glm::vec2& value);
            bool Write(const glm::vec3& value);
            bool Write(const glm::vec4& value);
            bool Write(const glm::ivec4& value);
            bool Write(const glm::mat3& value);
            bool Write(const glm::mat4& value);
            bool Write(const DrawElementsCommand& cmd);
            bool Write(const DrawArraysCommand& cmd);
            bool Write(const void* const data, GLuint size);

            // Overwrites the entire buffer using glBufferData
            // NOTE: Only valid on static buffers, and will give the
            // GL_DYNAMIC_DRAW hint instead of the default GL_STATIC_DRAW
            void Overwrite(const void* const data, GLuint size);

            // Binding methods
            void Bind(GLenum target) const;
            void BindBase(GLenum target, GLuint index) const;
            void BindRange(GLenum target, GLuint index, GLintptr offset, GLsizeiptr size);

            // Synchronization
            void Lock(); // Insert a fence sync for all rendering commands
            void Sync(); // Wait until our sync object has been signaled
            void SwapSections(); // Increase the buffer section, wraps to [0, numSections)

            // Accessors
            inline GLuint GetName() const { return id; };
            inline BufferType GetType() const { return type; };
            inline GLuint GetCurrentSection() const { return currentSection; };
            inline GLuint GetOffset() const { return (pCurrent - (pData + currentSection * size)); };
            inline size_t GetSize() const { return size; };

            // Helper method to ensure buffer writes are safe
            inline bool CanWrite(GLuint bytes) const { return (pCurrent + bytes) <= (pData + currentSection * size + size); };

        // Data / implementation
        private:
            
            static const int MAX_SECTIONS = 3;
            
            // Buffer state / data
            BufferType type;
            size_t size;
            GLuint currentSection = 0;
            GLuint numSections = 1;

            // Pointer to persistently mapped buffer
            unsigned char* pData = nullptr;
            unsigned char* pCurrent = nullptr;

            // OpenGL object handles
            GLuint id = 0;
            GLsync syncObj[MAX_SECTIONS] = {0, 0, 0};
    };
}