#pragma once

#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions

#include <phi/graphics/texture_2d.hpp>

namespace Phi
{
    // RAII Wrapper for an FBO
    class Framebuffer
    {
        // Interface
        public:

            Framebuffer();
            ~Framebuffer();

            // Delete copy constructor/assignment
            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            // Delete move constructor/assignment
            Framebuffer(Framebuffer&& other) = delete;
            void operator=(Framebuffer&& other) = delete;

            // Attaches a texture to the given attachment point
            void AttachTexture(const Texture2D* const texture, GLenum attachment);

            // Bind the framebuffer (defaults to both read and draw)
            void Bind(GLenum target = GL_FRAMEBUFFER) const;

            // Binds the default framebuffer
            void Unbind(GLenum target = GL_FRAMEBUFFER) const { glBindFramebuffer(target, 0); };

            // Checks if the framebuffer is complete
            bool CheckCompleteness() const;

            // Accessors

            // Returns the OpenGL handle for this FBO
            GLuint GetID() const { return fbo; }

        // Data / implementation
        private:
        
            // OpenGL objects
            GLuint fbo;

            // State
            int width = 0;
            int height = 0;
    };
}