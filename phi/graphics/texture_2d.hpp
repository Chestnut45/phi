#pragma once

#include <iostream>
#include <string>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions
#include <stb_image.h>

namespace Phi
{
    // 2D texture RAII wrapper
    class Texture2D
    {
        // Interface
        public:

            // Valid filter modes
            enum class FilterMode
            {
                Nearest,
                Linear
            };

            // Creates an empty texture with the given formats and parameters
            Texture2D(int width, int height,
                      GLint internalFormat, GLint format, GLenum type,
                      GLint wrapU, GLint wrapV,
                      GLenum minFilter, GLenum magFilter,
                      bool mipmap = false,
                      void* data = nullptr);
            
            // Loads a texture from disk with the given formats and parameters
            // Accepts local paths like data:// and user://
            Texture2D(const std::string& path,
                      GLint wrapU, GLint wrapV,
                      GLenum minFilter, GLenum magFilter,
                      bool mipmap = false);
            
            ~Texture2D();

            // Delete copy constructor/assignment
            Texture2D(const Texture2D&) = delete;
            Texture2D& operator=(const Texture2D&) = delete;

            // Delete move constructor/assignment
            Texture2D(Texture2D&& other) = delete;
            void operator=(Texture2D&& other) = delete;

            // Binds the texture to the given unit
            void Bind(int texUnit = 0) const;

            // Debug rendering methods

            // Blits the texture to the given screen coordinates of the default framebuffer (bottom left origin)
            // Width and height default to the texture's width and height
            void BlitToScreen(int x, int y, int width = -1, int height = -1) const;

            // Accessors
            inline GLuint GetID() const { return textureID; };
            inline int GetWidth() const { return width; };
            inline int GetHeight() const { return height; };
        
        // Data / implementation
        private:
        
            GLuint textureID;
            int width = 0;
            int height = 0;
    };
}