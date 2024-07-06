#include "texture_2d.hpp"

#include <phi/core/file.hpp>
#include <phi/graphics/framebuffer.hpp>

namespace Phi
{
    // Generate texture constructor
    Texture2D::Texture2D(int width, int height,
                         GLint internalFormat, GLint format, GLenum type,
                         GLint wrapU, GLint wrapV,
                         GLenum minFilter, GLenum magFilter,
                         bool mipmap,
                         void* data)
    {
        // Generate the texture object
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Apply texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapU);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapV);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        // Specify the texture details
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
        if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        this->width = width;
        this->height = height;
    }

    // Load from file constructor
    Texture2D::Texture2D(const std::string& path,
                         GLint wrapU, GLint wrapV,
                         GLenum minFilter, GLenum magFilter,
                         bool mipmap)
    {
        // Generate the texture object
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Default default texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapU);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapV);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        // Load the image data
        int width, height, channelCount;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(File::GlobalizePath(path).c_str(), &width, &height, &channelCount, STBI_rgb_alpha);
        if (data)
        {
            // Send image data to texture target
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

            // Set width and height
            this->width = width;
            this->height = height;
        }
        else
        {
            std::cout << "ERROR: Couldn't load file: " << path.c_str() << std::endl;
        }

        // Free the image data
        stbi_image_free(data);

        // Unbind
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Destructor
    Texture2D::~Texture2D()
    {
        glDeleteTextures(1, &textureID);
    }

    // Bind this texture to GL_TEXTURE_2D on a given texture unit
    // NOTE: This leaves texUnit as the currently active texture unit on return
    void Texture2D::Bind(int texUnit) const
    {
        // Ensure valid texture unit is supplied
        // OpenGL 3.x requires a minimum of 16 units per stage
        if (texUnit < 0 || texUnit > 15)
        {
            std::cout << "ERROR: Invalid texture unit passed to Texture2D::Bind" << std::endl;
            return;
        }

        // Set active unit and bind texture
        glActiveTexture(GL_TEXTURE0 + texUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    void Texture2D::BlitToScreen(int x, int y, int width, int height) const
    {
        // Initialize a single static blit framebuffer to use for all texture blits
        static Framebuffer blitFBO;

        // Attach ourselves to the blit framebuffer
        blitFBO.Bind();
        blitFBO.AttachTexture(this, GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // Auto-detect width and height of the texture if nothing is passed
        width = width == -1 ? this->width : width;
        height = height == -1 ? this->height : height;

        // Perform the blit
        glBlitFramebuffer(0, 0, this->width, this->height, x, y, x + width, y + height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}