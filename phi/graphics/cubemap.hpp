#pragma once

#include <iostream>
#include <vector>
#include <string>

#define GLEW_NO_GLU
#include <GL/glew.h> // OpenGL types / functions
#include <stb_image.h>

namespace Phi
{
    // Cubemap data
    class Cubemap
    {
        // Interface
        public:
            
            // Constructor
            // faces should contain 6 file paths to image files
            // in the order: right, left, top, bottom, front, back
            // Accepts local paths like data:// and user://
            Cubemap(const std::vector<std::string>& faces);
            ~Cubemap();

            // Delete copy constructor/assignment
            Cubemap(const Cubemap&) = delete;
            Cubemap& operator=(const Cubemap&) = delete;

            // Delete move constructor/assignment
            Cubemap(Cubemap&& other) = delete;
            void operator=(Cubemap&& other) = delete;

            // Bind this cubemap's texture to GL_TEXTURE_CUBE_MAP on a given texture unit
            void Bind(int texUnit = 0) const;
        
        private:
        
            // OpenGL objects
            GLuint textureID;
    };
}